#!/bin/bash
#================================================================================
#   
#    csm_history_table_delete_template.sh
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
#   usage:         run ./csm_db_history_combo_wrapper_delete_script.sh
#   version:       1.0
#   create:        04-10-2017
#   last modified: 03-15-2018
#================================================================================

#----------------------------------------------------------------
# Defined variables
#----------------------------------------------------------------
# Command line variables passed in:
# 1. Database Name,
# 2. Value of archive records to be deleted
#----------------------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'
OPTERR=0

DEFAULT_DB="csmdb"
logpath="/var/log/ibm/csm/db"
logname="csm_db_delete_script.log"
cd "${BASH_SOURCE%/*}" || exit
dbname=$DEFAULT_DB
now=$(date '+%Y-%m-%d.%H.%M.%S')

#-------------------------------------------------------------------------------
# Current user connected
#-------------------------------------------------------------------------------

current_user=`id -u -n`
db_username="postgres"

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
        echo "-------------------------------------------------------------------------------------------------------------"
        echo "[Error  ] illegal # of import arguments"
        echo "[Example] [./csm_history_table_delete_template.sh] [dbname] [archive_counter] [history_table_name] [data_dir]"
        echo "-------------------------------------------------------------------------------------------------------------"
        exit 1
    fi

    dbname=$1
    interval_time=$2
    i=$interval_time  #<-------variable passed into the SQL statement
    table_name1=$3
    data_dir=$4
    cur_path=$data_dir
    logpath=$data_dir

#----------------------------------------------------------------
# Below makes the directory if it does not exist
#----------------------------------------------------------------

    if [[ ! -e $data_dir ]]; then
        mkdir -p $data_dir
        if [ $? -ne 0 ]; then
            echo "make directory failed for: $data_dir"
            exit 1
        else
            chown postgres:root $data_dir
            chmod 755 $data_dir
        fi
    elif [[ ! -d $data_dir ]]; then
        echo "$data_dir already exists but is not a directory" 1>&2
        exit 1
    fi

#-------------------------------------------------------------------------------
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files to /tmp directory
#-------------------------------------------------------------------------------

    if [ -d "$logpath" -a -w "$logpath" ]; then #<--- if logpath exist and u have permission
#    if [ -d "$logpath" ]; then
        logdir="$logpath"
    else
        logdir="/tmp"
    fi
    logfile="${logdir}/${logname}"

#-------------------------------------------------------------------------------
# Log Message
#-------------------------------------------------------------------------------

    function LogMsg () {
    LogTime=$(date '+%Y-%m-%d.%H.%M.%S')
    echo "$LogTime ($current_user) ($table_name1.del ) $1" >> $logfile
    }

#--------------------------------------------------------------------------------
# Check if postgresql exists already (not used when combined with wrapper script)
#--------------------------------------------------------------------------------
#
#    psql -l 2>&1>/dev/null
#    if [ $? -eq 0  ]; then
#        LogMsg "------------------------------------------------------------------------------"
#        echo "[Info    ] PostgreSQL is installed"
#        LogMsg "[Info    ] PostgreSQL is installed"
#        LogMsg "------------------------------------------------------------------------------"
#    else
#        LogMsg "------------------------------------------------------------------------------"
#        echo "[Error   ] PostgreSQL may not be installed. Please check configuration settings" #<------add check from wrapper script
#        LogMsg "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
#        LogMsg "---------------------------------------------------------------------------------------"
#        exit 1
#    fi
#
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

        LogMsg "[Start ] Database does not exist."
#       echo "------------------------------------------------------------------------------"
        echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
#       echo "------------------------------------------------------------------------------"
        LogMsg "[Error ] Cannot perform action because the $dbname database does not exist. Exiting."
        LogMsg "[End   ] Database does not exist."
#       LogMsg "---------------------------------------------------------------------------------------"
        exit 1
    fi

#----------------------------------------------------------------
# All the raw combined timing results before trimming
# Along with csm allocation history archive results
#----------------------------------------------------------------

    time="$(time ( ls ) 2>&1 1>/dev/null )"
#   all_results="csm_delete_archive_all_results.$now.timings"
    tmp_delete_count="$data_dir/tmp_delete_count"

#-------------------------------------------------------------------------------
# psql history archive query execution
# variables were created and nested queries to track delete count totals
#-------------------------------------------------------------------------------

delete_count=`time psql -q -tA -U $db_username -d $dbname "ON_ERROR_STOP=1" << THE_END
    WITH delete_1 AS(
    DELETE FROM $table_name1 WHERE history_time < (NOW() - INTERVAL '$i MIN') AND archive_history_time IS NOT NULL RETURNING *)
    select count(*) from delete_1;
THE_END`
    echo "$delete_count" > "$tmp_delete_count.count"

#   echo "$time" >> $all_results
#   LogMsg "[Info    ] Database archive complete."
    LogMsg "[Info  ] Database process complete for deletion script."
#   LogMsg "---------------------------------------------------------------------------------------"

#-------------------------------------------------------------------------------------------------------------------
# Waits for the process to finish before calculating and trimming the results
#-------------------------------------------------------------------------------------------------------------------
#
#    wait
#
#-------------------------------------------------------------------------------------------------------------------
# This calculates and trims the results to a csv file
# (currently the caluculations are done within the wrapper script)
#-------------------------------------------------------------------------------------------------------------------
#
#    for i in $(grep real $all_results | awk '{ print substr($2,3,5) }'); do
#       total=$(echo $total+$i | bc )
#         echo "$i" >> $trim_timing_file
#           ((count++))
#           done
#
#           average=$(echo "scale=8; $total / $count" | bc)
#
#           echo $average > csm_db_history_delete_avg_results_$count_$now.csv
#
#----------------------------------------------------------------
# This removes all .timing files left over
# ToDo: handle unique .timing external file names (clean up)
#----------------------------------------------------------------
#
#    rm *timings #<-- this needs to be modified
#                 <-- (unique from other scripts running the
#                 <-- with .timings external files)
#
#----------------------------------------------------------------
