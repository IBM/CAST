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
#   version:       1.1
#   create:        04-10-2017
#   last modified: 06-18-2018
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
pid=$BASHPID
parent_pid=$PPID

#----------------------------------------------------------------
# These are the variables for the avg processing
#----------------------------------------------------------------

count=0
total=0
average="0"


dbname=$1
interval_time=$2
i=$interval_time  #<-------variable passed into the SQL statement
table_name1=$3
data_dir=$4
cur_path=$data_dir
logpath=$data_dir

#----------------------------------------------------------------
# All the raw combined timing results before trimming
# Along with history archive results
#----------------------------------------------------------------

    time="$(time ( ls ) 2>&1 1>/dev/null )"
#   all_results="csm_delete_archive_all_results.$now.timings"
    tmp_delete_count="$data_dir/${parent_pid}_${table_name1}_delete_count.count"

#-------------------------------------------------------------------------------
# psql history archive query execution
# variables were created and nested queries to track delete count totals
#-------------------------------------------------------------------------------

delete_count=`time psql -q -tA -U $db_username -d $dbname "ON_ERROR_STOP=1" << THE_END
    WITH delete_1 AS(
    DELETE FROM $table_name1 WHERE history_time < (NOW() - INTERVAL '$i MIN') AND archive_history_time IS NOT NULL RETURNING *)
    select count(*) from delete_1;
THE_END`
    echo "$delete_count" > "$tmp_delete_count"

#    LogMsg "[Info  ] Database process complete for deletion script."
