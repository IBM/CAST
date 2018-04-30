#! /bin/bash
#================================================================================
#   
#    bmc_SEL_trap.sh
# 
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================ 
# Assumes xcat is installed.
# Add a traphandle to /etc/snmp/snmptrapd.conf
# For example traphandle  default /ugadmin/Scripts/snmp_traps/bmc_SEL_trap.sh
# TODO Find out about parallel execution on snmptrapd - John

# Debug tools.
set -x 
exec 2> sel.err
exec 5> sel.log
BASH_XTRACEFD="5"

# TODO MOVE TO CONFIG FILE? -John (5/3/16)
BIG_DATA_STORE_IP="10.4.2.32"
BIG_DATA_STORE_PORT="10518"

# To prevent race conditions.
MUTEX_SLEEP_TIME=.5
MAX_MUTEX_SLEEP_ITERATIONS=10

# Last log location.
LOG_RECORDS_DIR="/var/lib/bmc_sel/hosts"

# The target Logstash server.
LOG_WRITE_LOCATION="/dev/tcp/${BIG_DATA_STORE_IP}/${BIG_DATA_STORE_PORT}"

# Formats the date in the ISO 8601 format
# $1 Date
# $2 Time
function format_date {
   echo $1 $2 | sed -e 's/\([0-9]*\)\/\([0-9]*\)\/\([0-9]*\)[ ]*\([0-9]*\):\([0-9]*\):\([0-9]*\)/\3-\1-\2T\4:\5:\6/g' -e 's/^[ \t]*//' -e 's/[ \t]*$//'
}

# Read off the snmptrap details. TODO do we want more here?
read host
read ip
ip=$(echo $ip |sed  "s/[^ ]*:[ ]*\[\(.*\)\]:.*-.*/\1/g")

#ip=50.4.4.12
# Give the password, username and hostname their default values.
bmcpassword="ADMIN"
bmcusername="admin"
hostname=$ip

# FIXME This makes the code work properly.
lsdef all 2>&1 >/dev/null

# Extract the password, username and hostname from the xcat database.
while IFS="=" read -r key value
do
    if [[ $value ]]
    then
        # Strip out the blanks with parameter expansion.
        case ${key//[[:blank:]]/} in
            bmcpassword)
                bmcpassword=$value;;
            bmcusername)
                bmcusername=$value;;
            *)
                echo "failed $key"
        esac
    else
        hostname=$(echo $key | cut -d":" -f2 | sed -e 's/^[ \t]*//' -e 's/[ \t]*$//')
    fi
done < <(/opt/xcat/bin/lsdef -w bmc=$ip -i bmcpassword,bmcusername)

# DEBUG ECHO
#echo "bmcpassword=${bmcpassword}\nbmcusername=${bmcusername}\nhostname=${hostname}" 

# Mutex lock on a file to deal with race conditions, if the script is stuck here too long, ignore the mutex lock.
elapsed_sleep=0
while true
do
   # If we can make this file, then there shouldn't be another script responding to this host, break.
   if  mkdir /tmp/${ip}_BMC_SEL_TRAP_LOCK 2>/dev/null
   then
      break
   fi

   # Wait for another script to finish its work. Should only hit this one time max.
   echo "Waiting: " $elapsed_sleep
   sleep $MUTEX_SLEEP_TIME
   elapsed_sleep=$(( $elapsed_sleep + 1 ))
   
   # If we exceed or meet the maxiimum sleep iterations, then remove the old folder and continue the script.
   if [ $elapsed_sleep -ge $MAX_MUTEX_SLEEP_ITERATIONS ]
   then 
       rm -rf /tmp/${ip}_BMC_SEL_TRAP_LOCK
   fi
done

# remove lock directory when script exits.
trap 'rm -rf /tmp/${ip}_BMC_SEL_TRAP_LOCK' 0 

###################################################
# Begin processing log files
###################################################

# Variables for processing the logs.

# The event id and date last processed by this node.
# If this is the first event received by the node, these are expected to be empty.
saved_event_id=""
saved_date=""

# A flag for verifying the event entry.
# Set to true when the script grabs the last n sel entries.
verify_event_entry=false


# If a log has been found already on this node.
if [ -e "$LOG_RECORDS_DIR/$ip" ]
then
    # Load the saved data.
    IFS=' ' read saved_event_id saved_date < $LOG_RECORDS_DIR/$ip
    last_SEL=$(/opt/xcat/bin/ipmitool-xcat -H $ip -U $bmcusername -P $bmcpassword sel list last 1) 
    
    # 
    OFS=$IFS
    IFS="|"    
    set -- $last_SEL    
    event_id=$( echo $1 | sed 's/[ ]*\([a-zA-Z0-9]*\)[ ]*/\1/g')    
    event_date=$(format_date $2 $3)
    IFS=$OFS

    # There are 9 possible states here.
    # The only instances that matter are when the date is > than the previous one
    # or when the id is greater and the date is equal (might not happen).
    # id\/date->| new date > | new date = | new date < |
    #-----------|------------|------------|------------|
    # new id >  |New entry   |Simultaneous|Something is|
    #           |added.      |entries?    |wrong.      |   
    #-----------|------------|------------|------------|
    # new id =  |An entry was|No new      |Something is|
    #           |removed and |entries.    |wrong.      |
    #           |added.      |            |            |
    #-----------|------------|------------|------------|
    # new id <  |Entries were|Entries were|Last entry  |
    #           |deleted and |deleted. No |was deleted.|
    #           |added.      |new entries.|            |
    #-----------|------------|------------|------------|
    # Difference from the last id and saved id.
    event_id_diff=$(( 0x${event_id} - 0x${saved_event_id} ))
   
    if [[ $event_date > $saved_date ]]   
    then
        echo "event is newer"
        # React depending on the difference between the saved id and last id.
        if [[ $event_id_diff -eq 1 ]]
        then
            # If the differnce was only one, don't go back to ipmitool, we're done.
            echo $last_SEL > /tmp/${ip}_logs

        elif [[ $event_id_diff -gt 1 ]]
        then
            # Pull an extra entry just in case there is another entry added to the execution.
            ((event_id_diff+=1))
            # Get the last $event_id_diff SEL entries.
            /opt/xcat/bin/ipmitool-xcat -H $ip -U $bmcusername -P $bmcpassword sel list last $event_id_diff > /tmp/${ip}_logs
            
            # There is a chance that this doesn't grab all of the entries.
            verify_event_entry=true
        else # new id = | new date > and new id < | new date >
            # If the id is equivalenti or less, but the date is greater that indicates some
            # kind of sel purge, we need to grab all of the data to ensure we get everything.
            /opt/xcat/bin/ipmitool-xcat -H $ip -U $bmcusername -P $bmcpassword sel list > /tmp/${ip}_logs
        fi
    elif [[ $event_date == $saved_date ]]
    then
        # FIXME not sure how to react to this scenario, just copied the event id > code. -John (5/3/16)
        echo "TODO newid > | new date ="
        if [[ $event_id_diff -eq 1 ]]
        then
            # If the differnce was only one, don't go back to ipmitool, we're done.
            echo $last_SEL > /tmp/${ip}_logs

        elif [[ $event_id_diff -gt 1 ]]
        then
            # Pull an extra entry just in case there is another entry added to the execution.
            ((event_id_diff+=1))
            # Get the last $event_id_diff SEL entries.
            /opt/xcat/bin/ipmitool-xcat -H $ip -U $bmcusername -P $bmcpassword sel list last $event_id_diff > /tmp/${ip}_logs
            
            # There is a chance that this doesn't grab all of the entries.
            verify_event_entry=true
        fi
    else
        # If the entry ID is identical exit, as there is nothing to be gained.
        echo "No new events have been detected."
        exit
    fi   
else
    # If there's no last record, get all of the records from ipmi sel list.
    /opt/xcat/bin/ipmitool-xcat -H $ip -U $bmcusername -P $bmcpassword sel list > /tmp/${ip}_logs
fi

# -1 - stop parsing, 1 - parse normally, 2+ - parse again with some filtering.
parse_iteration=1

# The last event parsed.
last_event_id=""
last_event_date=""

# The first event id/date from the previous iteration.
previous_first_event_id=""
previous_first_event_date=""

# Cache and change IFS
OFS=$IFS
IFS="|"

# Should only execute this code once under normal operation.
# If the SEL got new events while this script got the SEL this will execute at least twice.
# Don't let this execute 4 times, if 4 iterations are reached stop executing.
while (( $parse_iteration > 0 && $parse_iteration < 5 ))
do
    # Number of the entry being processed, used to ensure only the first
    # event_id and event_date are cached for error checking.
    entry_number=0

    # The first entry processed's id and date.
    first_event_id=""
    first_event_date=""

    # -u3 = read from file descriptor 3
    # The actual parsing of the retrieved SEL.
    while read -r -u3 line
    do
        # Set to extract the fields.
        set -- $line
        
        # Event ID and description are going to be 1:1
        event_id=$( echo $1 | sed -e 's/^[ \t]*//' -e 's/[ \t]*$//' )
        description=$( echo $4"-"$5"-"$6 | sed -e 's/^[ \t]*//' -e 's/[ \t]*$//' )
        
        # Convert the date to the iso-8661 standard for logstash parsing.
        event_date=$(format_date $2 $3)
                
        # Increment the entry number.
        ((entry_number+=1))
        
        # On the first iteration, cache the first entry id 
        # and saved date to do a final check at the end of the execution.
        # XXX This might not be ideal, but I want to avoid duplicating the parse
        # code since that is likely slower than just a branch. Additionally, 
        # under expected circumstances this should only be hit once - John (5/3/16)
        if [[ $entry_number -eq 1 ]]
        then
            first_event_id=$event_id
        fi
        
        # TODO Find out if two events can happen coterminally. - John (5/3/16)
        # Only save if the date is newer than the last one
        if [[ $event_date > $saved_date ]]
        then
            # Break if this is an additional iteration and the previous leading event is hit.
            if [[ $parse_iteration -gt 1 ]]
            then 
                # XXX should we check on the date?
                event_id_diff=$(( 0x${previous_first_event_id} - 0x${event_id} ))
                if [[ $event_id_diff -ge 0 ]]
                then
                    break
                fi
            fi
            
            last_event_id=$event_id
            last_event_date=$event_date
            
            echo ${event_date}:${hostname}:${ip}:${event_id}:${description} > $LOG_WRITE_LOCATION
        fi
    done 3< /tmp/${ip}_logs

    # Check to verify that the first event grabbed is either identical 
    # to the last event saved, or is one higher in the id chain.
    # If the event warrants being verified, verify it.
    if [ "$verify_event_entry" = true ]
    then 
        # If the event id difference is greater than 0 we have a problem.
        first_event_id_diff=$(( 0x${first_event_id} - 0x${saved_event_id} ))
	total_event_id_diff=$(( 0x${last_event_id} - 0x${saved_event_id} ))
        
        # XXX should we check the date here? -John (5/3/16)
        if [[ $first_event_id_diff -gt 0 ]]
        then            
            # Save the this iteration's earliest 
            previous_first_event_id=$first_event_id
            previous_first_event_date=$first_event_date
            
            # Increase the perform_parse variable so the parse loop knows to filter the log.
            (( parse_iteration+=1 ))
            
            total_event_id_diff=$(( ${total_event_id_diff} + ${first_event_id_diff} * ${parse_iteration} ))           

            # Grab even more of the sel this time.
            /opt/xcat/bin/ipmitool-xcat -H $ip -U $bmcusername -P $bmcpassword sel list last $total_event_id_diff  > /tmp/${ip}_logs
        else
            # Set the perform_parse to -1, this will kick out of the parse loop 
            parse_iteration=-1
        fi
    else
            # Set the perform_parse to -1, this will kick out of the parse loop 
            parse_iteration=-1
    fi
done
# Reset the IFS.
IFS=$OFS

# The last chronological entry processed by this script, gets written to  
# a directory of flat files.
echo "${last_event_id} ${last_event_date}" > $LOG_RECORDS_DIR/$ip
