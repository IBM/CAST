#!/bin/bash
#================================================================================
#   
#    csm_ras_event_action_wrapper_archive_script.sh
# 
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

#================================================================================
#   usage:         Archive csm_ras_event_action table
#   version:       1.0
#   created:       06-01-2018
#   last modified: 06-20-2018
#================================================================================

#----------------------------------------------------------------
# Traps any interrupted or terminated sessions
# (see finish function below)
#----------------------------------------------------------------

trap finish SIGINT
trap finish SIGTERM

#------------------------------------------------
# Postgres and other error handling options
#------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'
OPTERR=0

#----------------------------------------------------------------
# Defined variables
#----------------------------------------------------------------
# Command line variables passed in:
# 1. Database Name,
# 2. Value of archive records to be processed
#----------------------------------------------------------------

DEFAULT_DB="csmdb"
logpath="/var/log/ibm/csm/db"
logname="csm_db_archive_script.log"
tmp_logname="$$_csm_db_archive_script.log"
cd "${BASH_SOURCE%/*}" || exit
dbname=$DEFAULT_DB
now=$(date '+%Y-%m-%d')
start_time=`date +%s%N`

source ./csm_db_utils.sh

line1_out="------------------------------------------------------------------------------------------------------------------------"
line2_log="------------------------------------------------------------------------------------"
line3_log="---------------------------------------------------------------------------------------------------------------------------"

#------------------------------------------------------------------------------------
script_name="csm_ras_event_action_wrapper_archive_script.sh"
#echo "------------------------------------------------------------------------------"
#echo "[Script name:   ]  $script_name"

#-------------------------------------------------------------------------------
# Current user connected
#-------------------------------------------------------------------------------

current_user=`id -u -n`
db_username="postgres"
now1=$(date '+%Y-%m-%d.%H:%M:%S')
pid=$BASHPID

#----------------------------------------------------------------
# These are the variables for the avg processing
#----------------------------------------------------------------

count=0
total=0
average="0"

#----------------------------------------------------------------
# Below checks the arguments passed in on the command line
#----------------------------------------------------------------

    if [ "$#" -ne 4 ]; then
        echo ${line1_out}
        echo "[Error  ] illegal # of import arguments"
        echo "[Info   ] Data_dir is where the archive files will be written"
        echo "[Example] [./csm_ras_event_action_wrapper_archive_script.sh] [dbname] [archive_counter] [table_name] [/data_dir/]"
        echo ${line1_out}
        exit 1
    fi

dbname=$1
archive_counter=$2
table_name1=$3
data_dir=$4
cur_path=$data_dir
logpath=$data_dir #<----- This file will live in "/var/log/ibm/csm/db"

#----------------------------------------------------------------
# Below makes the directory if it does not exist
# First checks if the "/" is specified at the end of the
# directory given path. If not then it is added
#----------------------------------------------------------------
    
    if [[ "${data_dir: -1}" != "/" ]]; then
        data_dir="${data_dir}/"
    fi 

    if [[ ! -e $data_dir ]]; then
        mkdir -p $data_dir 2>>/dev/null
        if [ $? -ne 0 ]; then
            echo "${line1_out}"
            echo "[Error  ] make directory failed for: $data_dir"
            echo "[Info   ] mkdir: cannot create directory ‘$data_dir’: Permission denied"
            echo "[Info   ] please provide a valid writable directory"
            echo "${line1_out}"
            exit 1
        else
            chown postgres:postgres $data_dir
            chmod 755 $data_dir
        fi
    elif [[ ! -d $data_dir ]]; then
        echo "${line1_out}"
        echo "$data_dir already exists but is not a directory" 1>&2
        echo "${line1_out}"
        exit 1
    fi

#-------------------------------------------------------------------------------
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files to /tmp directory
#-------------------------------------------------------------------------------

     if [ -d "$logpath" -a -w "$logpath" ]; then #<--- if logpath exist and u have permission
         logdir="$logpath"
     else
         logdir="/tmp"
     fi
     logfile="${logdir}/${tmp_logname}"

#-------------------------------------------------------------------------------
# Log Message
#-------------------------------------------------------------------------------

     function LogMsg () {
     LogTime=$(date '+%Y-%m-%d.%H:%M:%S')
     echo "$LogTime ($pid) ($current_user) $1" >> $logfile 2>&1
     }

     LogMsg "[Start ] Archiving Process:   |  $table_name1"
     LogMsg "${line2_log}"

#-------------------------------------------------------------------------------
# Log Message File Size
#-------------------------------------------------------------------------------

function filesize () {
touch ${data_dir}$logname
MaxFileSize=1000000000
now2=$(date '+%Y-%m-%d.%H.%M.%S')

     cat ${data_dir}$tmp_logname >> ${data_dir}$logname

    #--------------------
    #Get size in bytes
    #--------------------
    file_size=`du -b ${data_dir}$logname | tr -s '\t' ' ' | cut -d' ' -f1`
    if [ $file_size -gt $MaxFileSize ];then
        mv ${data_dir}$logname ${data_dir}$logname.$now2
        touch ${data_dir}$logname
    fi
}

#-------------------------------------------------------------------------------
# Error Log Message
#-------------------------------------------------------------------------------

    function finish () {

    echo   "${line1_out}"
    echo   "[Info   ] Archiving process for $table_name1 has been interrupted or terminated."
    echo   "[Info   ] Please see log file for more details"
    LogMsg "[Info  ] Arch process for:    |  $table_name1 has been interrupted or terminated."
    LogMsg "[Info  ] Exiting:             |  csm_ras_event_action_wrapper_archive_script.sh."
    LogMsg "${line2_log}"
    LogMsg "[End   ] Archiving Process:   |  $table_name1"
    echo   "${line1_out}"
    echo "${line3_log}" >> $logfile


    filesize
    cat ${data_dir}$tmp_logname >> ${data_dir}$logname
    wait
    
    #-------------------------------------------------
    # Clean up any failed archiving runs
    #-------------------------------------------------
    
    rm -rf ${data_dir}$tmp_logname
    
    if [[ ! -f ${data_dir}${pid}_${table_name1}_archive_results* ]]; then
        rm -f ${data_dir}${pid}_${table_name1}_archive_results*
    fi
    
    if [[ ! -f ${data_dir}${pid}_${table_name1}.count* ]]; then
        rm -f ${data_dir}${pid}_${table_name1}.count*
    fi
    exit $?
}

#----------------------------------------------------------------
# Check if postgresql exists already
#----------------------------------------------------------------

string1="$now1 ($pid) ($current_user) [Info  ] DB Names:            |"
psql -l 2>>/dev/null $logfile

if [ $? -ne 127 ]; then       #<------------This is the error return code
db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
\set ON_ERROR_STOP true
select string_agg(datname,', ') from pg_database;
EOF`
    echo "$string1 $db_query" | sed "s/.\{40\},/&\n$string1 /g" >> $logfile 2>&1
    LogMsg "[Info  ] DB install check:    |  PostgreSQL is installed"
else
    echo "${line1_out}"
    echo "[Error ] PostgreSQL may not be installed. Please check configuration settings"
    LogMsg "[Error ] PostgreSQL:          |  Might not be installed."
    LogMsg "[Info  ] Additional Message:  |  Please check configuration settings"
    finish
    exit 1
fi

        LogMsg "[Info  ] Script name:         |  csm_ras_event_action_wrapper_archive_script.sh"

#----------------------------------------------------------------
# Check if database exists
#----------------------------------------------------------------

    db_exists="no"
    psql -lqt | cut -d \| -f 1 | grep -qw $dbname
        if [ $? -eq 0 ]; then
        db_exists="yes"
        fi

#----------------------------------------------------------------
# End it if the input argument requires an existing database
# (Database does not exist)
#----------------------------------------------------------------

    if [ $db_exists == "no" ]; then

        echo "${line1_out}"
        echo "[Error   ] Cannot perform: $dbname database does not exist"
        echo "[Info    ] Please provide a valid DB that exists on the system (hint: psql -l)."
        LogMsg "[Error ] Cannot perform:      |  $dbname database does not exist"
        LogMsg "[Info  ] Please provide:      |  A valid DB that exists on the system (hint: psql -l)."
        finish
        exit 1
    fi

#-----------------------------------------------
# csm_table_name & csm_table_data_array
#-----------------------------------------------

declare -A avg_data

#----------------------------------------------------------------
# This should be in the order of each of the child history scripts
#----------------------------------------------------------------

    table_name=()
    table_name+=(${3}                  )

#----------------------------------------------------------------
# All the raw combined timing results before trimming
#----------------------------------------------------------------

    all_results="$data_dir/${pid}_${table_name1}_archive_results.$now.timings"

#----------------------------------------------------------------
# These are the individual history tables being archived
#----------------------------------------------------------------

start_timer
./csm_ras_event_action_table_archive.sh $dbname $archive_counter $table_name1 $data_dir >>"$all_results" 2>&1
awk '/trace/ {print $0}
/^ERROR:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d.%H:%M:%S') ($pid) ($current_user) [Error ] DB Message:          |  "'"$0}' ${all_results}  >>"${logfile}"
end_timer "template time" 2>> ${logfile}

#runtime="$(($(date +%s%N)-$start_time))"
#sec="$((runtime/1000000000))"
#min="$((runtime/1000000))"
#
#t_time=`printf "%02d:%02d:%02d:%02d.%03d\n" "$((sec/86400))" "$((sec/3600%24))" "$((sec/60%60))" "$((sec%60))" "${min}"`

#-------------------------------------------------------------------------------------------------------------------
# Waits for the process to finish before calculating and trimming the results
#-------------------------------------------------------------------------------------------------------------------

    wait        

#----------------------------------------------------------------
# Create the archive count array from external file
#----------------------------------------------------------------
# 1. create array
# 2. read a line in
# 3. create an index on table name and append line to the array
# 4. increment table name (index for each pass)
#----------------------------------------------------------------

z=0
d=0

declare -A archive_array

if [ -f "$data_dir/${pid}_$table_name1.count" ]; then
    for file in $( ls -1 $data_dir/${pid}_$table_name1.count)
        do
            archive_array[${table_name[z]}]=$(cat $file)
            ((z++))
        done
else
    echo "${line1_out}"
    echo "[Error: ] The directory ($data_dir$table_name1) was invalid."
    LogMsg "[Error ] The directory:       |  ($data_dir$table_name1) was invalid."
    echo "[Error: ] Or the table: $table_name1 is not a valid archiving table."
    LogMsg "[Error ] Or the table:        |  $table_name1 is not a valid archiving table."
    echo "[Info:  ] Please check the log file: $data_dir$logname for detailed info."
    rm ${all_results}
        if [[ -f ${data_dir}${table_name1}.archive.${now}.json ]]; then
            rm ${data_dir}${table_name1}.archive.${now}.json
        fi
    finish
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 0
fi

#-------------------------------------------------------------------------------------------------------------------
# This calculates and trims the results to a csv file
#-------------------------------------------------------------------------------------------------------------------

    j=0
    echo "${line1_out}"

    for i in $(grep real $all_results | awk '{ print substr($2,3,5) }'); do
        total=$(echo $total+$i | bc | awk '{printf "%.3f\n", $0}')
        avg_data[${table_name[j]}]="$i"
        ((count++))
        ((j++))
    done

    average=$(echo "scale=6; $total / $count" | bc | awk '{printf "%.3f\n", $0}')

#------------------------------------------------------------------------------------------
# Archiving results output
#------------------------------------------------------------------------------------------

echo "  Table                        |       Time         |  Archive Count (DB Actual)"
echo "-------------------------------|--------------------|--------------------------------"

e_time=`printf " Total Time (Cleanup)          |  %02d:%02d:%02d:%02d.%03d\n" "$((sec/86400))" "$((sec/3600%24))" "$((sec/60%60))" "$((sec%60))" "${min}"`

for ((j=0; j<${#table_name[*]}; j++));
do
    table=${table_name[j]}
    table_avg=${avg_data[$table]}
    archive_count=${archive_array[$table]}
    printf '%-0s %-29s %-20s %0s\n' "" "$table" "|  $t_time" "|   $archive_count"
    LogMsg "[Info  ] Table name:          |  $table"
    LogMsg "[Info  ] Table time:          |  $t_time"
    LogMsg "[Info  ] User Count(cmd-line):|  $archive_counter"
    LogMsg "[Info  ] Actual DB Arc count: |  $archive_count"
done

    echo "${line1_out}"
    echo " Date/Time:                    |  $now"
    echo " DB Name:                      |  $dbname"
    echo " DB User:                      |  $current_user"
    echo " User Count (cmd-line):        |  $archive_counter"
    LogMsg "[Info  ] Complete:            |  $table_name1 archive process"

#----------------------------------------------------------------
# This removes all .timing files left over
#----------------------------------------------------------------

    rm ${all_results}
    rm ${data_dir}${pid}_$table_name1.count
#----------------------------------------------------------------

runtime="$(($(date +%s%N)-$start_time))"
sec="$((runtime/1000000000))"
min="$((runtime/1000000))"

#----------------------------------------------------------------
# Total script time calculated
#----------------------------------------------------------------

e_time=`printf "%02d:%02d:%02d:%02d.%03d\n" "$((sec/86400))" "$((sec/3600%24))" "$((sec/60%60))" "$((sec%60))" "${min}"`
printf " Total Time (Cleanup)          |  %02d:%02d:%02d:%02d.%03d\n" "$((sec/86400))" "$((sec/3600%24))" "$((sec/60%60))" "$((sec%60))" "${min}"
echo "${line1_out}"
LogMsg "[Info  ] Total Script Time:   |  $e_time"
LogMsg "${line2_log}"
LogMsg "[End   ] Archiving Process:   |  $table_name1"
echo "${line3_log}" >> $logfile

#----------------------------------------------------------------
# Temp file to master log file and clean up
#----------------------------------------------------------------

start_timer
filesize
end_timer "filesize run" 2>> ${logfile}
#cat ${data_dir}$tmp_logname >> ${data_dir}$logname
wait
rm ${data_dir}$tmp_logname
