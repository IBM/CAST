#!/bin/bash
#================================================================================
#   
#    csm_history_table_archive_template.sh
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
#   usage:         run ./csm_history_table_archive_template.sh
#   version:       1.0
#   create:        02-01-2018
#   last modified: 03-15-2018
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
        echo "------------------------------------------------------------------------------------------------------------------------"
        echo "[Error  ] illegal # of import arguments"
        echo "[Info   ] Data_dir is where the archive files will be written"
        echo "[Example] [./csm_history_table_archive_template.sh] [dbname] [archive_counter] [history_table_name] [/data_dir/]"
        echo "------------------------------------------------------------------------------------------------------------------------"
        exit 1
    fi
    
    dbname=$1
    archive_counter=$2
    table_name=$3
    data_dir=$4
    cur_path=$data_dir
    logpath=$data_dir   #<----- This file will live in "/var/log/ibm/csm/db" 

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
#   if [ -d "$logpath" ]; then
        logdir="$logpath"
    else
        logdir="/tmp"
    fi
    logfile="${logdir}/${logname}"

#-------------------------------------------------------------------------------
# Log Message 
#-------------------------------------------------------------------------------

    function LogMsg () {
    LogTime=$(date '+%Y-%m-%d.%H:%M:%S')
    echo "$LogTime ($current_user) ($table_name.arc ) $1" >> $logfile
    }

#   LogMsg "[Start   ] Welcome to CSM datatbase: ./csm_history_table_archive_template.sh."

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
        echo "-------------------------------------------------------------------------------------------------------------"
        echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting." #<------add check from wrapper script
        echo "[Info    ] Please provide a valid DB that exists on the system (hint: psql -l)."
        echo "-------------------------------------------------------------------------------------------------------------"
        LogMsg "[Error ] Cannot perform action because the $dbname database does not exist. Exiting."
        LogMsg "[End   ] Please provide a valid DB that exists on the system (hint: psql -l)."
        LogMsg "[End   ] Database does not exist."
        echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
#       LogMsg "---------------------------------------------------------------------------------------"
        exit 1
    fi

#----------------------------------------------------------------
# All the raw combined timing results before trimming
# Along with csm allocation history archive results
#----------------------------------------------------------------

    time="$(time ( ls ) 2>&1 1>/dev/null )"

#-------------------------------------------------------------------------------
# psql history archive query execution
#-------------------------------------------------------------------------------
archive_count_$table_name=$(`time psql -q -t -U $db_username -d $dbname << THE_END
            BEGIN;
            DROP TABLE IF EXISTS temp_$table_name;
            CREATE TEMP TABLE temp_$table_name
                as
                    (SELECT *
                        from(
                        SELECT * FROM (
                        SELECT *, ROW_NUMBER() OVER(ORDER BY history_time) num
                        FROM $table_name
                        WHERE archive_history_time IS NULL) sh1
                        where num <= $archive_counter) sh2
                        ORDER BY history_time ASC);

                    COPY temp_$table_name
                    to '$cur_path/$table_name.archive.$now.csv';

                    COPY (select count(*) from temp_$table_name)
                    to '$cur_path/$table_name.count';

                        UPDATE $table_name
                        SET archive_history_time = 'now()'
                        FROM temp_$table_name
                        WHERE
                        $table_name.history_time = temp_$table_name.history_time
                        AND
                        $table_name.archive_history_time IS NULL;

            COMMIT;
THE_END`
)            
            #LogMsg "[Info    ] Database archive complete."
            LogMsg "[Info  ] Database process for $table_name table."
            #LogMsg "---------------------------------------------------------------------------------------"
