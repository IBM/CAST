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
#   version:       1.1
#   create:        02-01-2018
#   last modified: 06-18-2018
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
now=$(date '+%Y-%m-%d')

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

dbname=$1
archive_counter=$2
table_name=$3
data_dir=$4
cur_path=$data_dir
logpath=$data_dir   #<----- This file will live in "/var/log/ibm/csm/db" 

#----------------------------------------------------------------
# All the raw combined timing results before trimming
# Along with csm allocation history archive results
#----------------------------------------------------------------

time="$(time ( ls ) 2>&1 1>/dev/null )"

#-------------------------------------------------------------------------------
# psql history archive query execution
#-------------------------------------------------------------------------------
return_code=0
if [ $? -ne 127 ]; then

json_file="$cur_path/$table_name.archive.$now.json"
swap_file="$cur_path/.$table_name.archive.$now.swp"
archive_count_$table_name=$(`time psql -q -t -U $db_username -d $dbname << THE_END
            \set AUTOCOMMIT off
            \set ON_ERROR_ROLLBACK on
            \set ON_ERROR_STOP TRUE
            BEGIN;
            --DROP TABLE IF EXISTS temp_$table_name;
            CREATE TEMP TABLE temp_$table_name
                as
                    (SELECT *, ctid AS id
                        FROM $table_name
                        WHERE archive_history_time IS NULL
                        ORDER BY history_time ASC
                        LIMIT $archive_counter FOR UPDATE); -- (Lock rows associated with this archive batch)
                    
                        UPDATE $table_name
                        SET archive_history_time = 'now()'
                        FROM temp_$table_name
                        WHERE
                        $table_name.history_time = temp_$table_name.history_time
                        AND
                        $table_name.archive_history_time IS NULL
                        AND
                        temp_$table_name.id = $table_name.ctid;

                        ALTER TABLE temp_$table_name
                        DROP COLUMN id;

                        COPY (select row_to_json(temp_$table_name) from temp_$table_name)
                        to '$swap_file';

                        COPY (select count(*) from temp_$table_name)
                        to '$cur_path/${parent_pid}_$table_name.count';
           COMMIT;
THE_END`
)

sed -i "s:([\b\f\n\r\t\"]):\\\1:g" ${swap_file}

# Enrich the 
awk -v table="$table_name" '{print "{\"type\":\"db-"table"\",\"data\":"$0"}" }' ${swap_file}\
    >> ${json_file}
if [ $? -eq 0 ]
then
    rm -f ${swap_file}
fi

else
    echo "[Error] Archiving process for $table_name has been interrupted or terminated. please see log file"
    LogMsg "[Error] Archiving process for $table_name has been interrupted or terminated. please see log file"
    LogMsg "---------------------------------------------------------------------------------------"
    return_code=1
fi
