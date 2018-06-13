#!/bin/bash
#================================================================================
#   
#    csm_ras_event_action_table_archive.sh
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
#   usage:         run ./csm_ras_event_action_table_archive.sh
#   version:       1.0
#   create:        06-01-2018
#   last modified: 06-11-2018
#   change log:
#================================================================================

#----------------------------------------------------------------
# Defined variables
#----------------------------------------------------------------
# Command line variables passed in:
# 1. Database Name,
# 2. Value of archive records to be processed
#----------------------------------------------------------------

export PGOPTIONS='--client-min-messages=warning'
OPTERR=0

DEFAULT_DB="csmdb"
logpath="/var/log/ibm/csm/db"
logname="csm_db_archive_script.log"
cd "${BASH_SOURCE%/*}" || exit
now=$(date '+%Y-%m-%d.%H.%M.%S.%N')

#-------------------------------------------------------------------------------
# Current user connected
#-------------------------------------------------------------------------------

current_user=`id -u -n`
db_username="postgres"
pid=$BASHPID
parent_pid=$PPID

#----------------------------------------------------------------
# These are the variables for the avg processing
#----------------------------------------------------------------

count=0
total=0
average="0"

#----------------------------------------------------------------
# Below checks the arguments passed in on the command line
#----------------------------------------------------------------

#    if [ "$#" -ne 4 ]; then
#        echo "------------------------------------------------------------------------------------------------------------------------"
#        echo "[Error  ] illegal # of import arguments"
#        echo "[Info   ] Data_dir is where the archive files will be written"
#        echo "[Example] [./csm_ras_event_action_table_archive.sh] [dbname] [archive_counter] [table_name] [/data_dir/]"
#        echo "------------------------------------------------------------------------------------------------------------------------"
#        exit 1
#    fi
    
dbname=$1
archive_counter=$2
table_name=$3
data_dir=$4
cur_path=$data_dir
logpath=$data_dir   #<----- This file will live in "/var/log/ibm/csm/db" 

#----------------------------------------------------------------
# Below makes the directory if it does not exist
# First checks if the "/" is specified at the end of the
# directory given path. If not then it is added
#----------------------------------------------------------------

#    if [[ "${data_dir: -1}" != "/" ]]; then
#        data_dir="${data_dir}/"
#    fi

#    if [[ ! -e $data_dir ]]; then
#        mkdir -p $data_dir
#        if [ $? -ne 0 ]; then
#            echo "make directory failed for: $data_dir"
#            exit 1
#        else
#            chown postgres:root $data_dir
#            chmod 755 $data_dir
#        fi
#    elif [[ ! -d $data_dir ]]; then
#        echo "$data_dir already exists but is not a directory" 1>&2
#        exit 1
#    fi

#-------------------------------------------------------------------------------
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files to /tmp directory
#-------------------------------------------------------------------------------

#    if [ -d "$logpath" -a -w "$logpath" ]; then #<--- if logpath exist and u have permission
#   if [ -d "$logpath" ]; then
#        logdir="$logpath"
#    else
#        logdir="/tmp"
#    fi
#    logfile="${logdir}/${logname}"

#-------------------------------------------------------------------------------
# Log Message 
#-------------------------------------------------------------------------------

#    function LogMsg () {
#    LogTime=$(date '+%Y-%m-%d.%H:%M:%S')
#    echo "$LogTime ($pid) ($current_user) ($table_name.arc ) $1" >> $logfile
#    }
#
#   LogMsg "[Start   ] Welcome to CSM datatbase: ./csm_ras_event_action_table_archive.sh."

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
#        echo "[Error   ] PostgreSQL may not be installed. Please check configuration settings" <------add check from wrapper script
#        LogMsg "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
#        LogMsg "---------------------------------------------------------------------------------------"
#        exit 1
#    fi
#
#----------------------------------------------------------------
# Check if database exists
#----------------------------------------------------------------

#    db_exists="no"
#    psql -lqt | cut -d \| -f 1 | grep -qw $dbname
#        if [ $? -eq 0 ]; then
#        db_exists="yes"
#        fi

#----------------------------------------------------------------
# End it if the input argument requires an existing database
# (Database does not exist)
#----------------------------------------------------------------

#    if [ $db_exists == "no" ]; then
#
#        LogMsg "[Start ] Database does not exist."
#        echo "-------------------------------------------------------------------------------------------------------------"
#        echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting." #<------add check from wrapper script
#        echo "[Info    ] Please provide a valid DB that exists on the system (hint: psql -l)."
#        echo "-------------------------------------------------------------------------------------------------------------"
#        LogMsg "[Error ] Cannot perform action because the $dbname database does not exist. Exiting."
#        LogMsg "[End   ] Please provide a valid DB that exists on the system (hint: psql -l)."
#        LogMsg "[End   ] Database does not exist."
#        echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
##       LogMsg "---------------------------------------------------------------------------------------"
#        exit 1
#    fi

#----------------------------------------------------------------
# All the raw combined timing results before trimming
# Along with csm_ras_event_action archive results
#----------------------------------------------------------------

    time="$(time ( ls ) 2>&1 1>/dev/null )"

#-------------------------------------------------------------------------------
# psql ras archive query execution
#-------------------------------------------------------------------------------
return_code=0
if [ $? -ne 127 ]; then

json_file="$cur_path/$table_name.archive.${parent_pid}.$now.json"
archive_count_$table_name=$(`time psql -q -t -U $db_username -d $dbname "ON_ERROR_STOP=1" << THE_END
            \set ON_ERROR_STOP TRUE
            BEGIN;
            --DROP TABLE IF EXISTS temp_$table_name;
            CREATE TEMP TABLE temp_$table_name
                as
                    (SELECT *, ctid AS id
                        FROM $table_name
                        WHERE archive_history_time IS NULL
                        ORDER BY master_time_stamp ASC
                        LIMIT $archive_counter FOR UPDATE); -- (Lock rows associated with this archive batch)
                    
                        UPDATE $table_name
                        SET archive_history_time = 'now()'
                        FROM temp_$table_name
                        WHERE
                        $table_name.master_time_stamp = temp_$table_name.master_time_stamp
                        AND
                        $table_name.archive_history_time IS NULL
                        AND
                        temp_$table_name.id = $table_name.ctid;

                        ALTER TABLE temp_$table_name
                        DROP COLUMN id;

                        COPY (select row_to_json(temp_$table_name) from temp_$table_name)
                        to '$json_file';

                        COPY (select count(*) from temp_$table_name)
                        to '$cur_path/${parent_pid}_$table_name.count';
           COMMIT;
THE_END`
)
awk -v table="$table_name" '{print "{\"type\":\"db-"table"\",\"data\":"$0"}" }' $json_file > ${json_file}.swp
mv ${json_file}.swp $json_file

else
    echo "[Error] Archiving process for $table_name has been interrupted or terminated. please see log file"
    LogMsg "[Error] Archiving process for $table_name has been interrupted or terminated. please see log file"
    LogMsg "---------------------------------------------------------------------------------------"
    return_code=1
fi

#            LogMsg "[Info    ] Database archive complete."
#            LogMsg "[Info  ] Database process for $table_name table."
#            LogMsg "---------------------------------------------------------------------------------------"
#cat $tmp_logname
