#!/bin/bash
#================================================================================
#
#    csm_db_ras_type_script.sh
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
#   usage:              ./csm_db_ras_type_script.sh
#   current_version:    01.7
#   create:             11-07-2017
#   last modified:      08-10-2018
#================================================================================

export PGOPTIONS='--client-min-messages=warning'

OPTERR=0
logpath="/var/log/ibm/csm/db"
#logpath=`pwd` #<------- Change this when pushing to the repo.
logname="csm_db_ras_type_script.log"
cd "${BASH_SOURCE%/*}" || exit
#cur_path=`pwd`

#==============================================
# Current user connected---
#==============================================
current_user=`id -u -n`
dbname="csmdb"
db_username="postgres"
csmdb_user="csmdb"
now1=$(date '+%Y-%m-%d %H:%M:%S')

BASENAME=`basename "$0"`

#==============================================
# Log Message---
#================================================================================
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log filesto /tmp directory
# The current version will only display results to the screen
#================================================================================

if [ -d "$logpath" ]; then
    logdir="$logpath"
else
    logdir="/tmp"
fi
logfile="${logdir}/${logname}"

#==============================================
# Log Message function
#==============================================

function LogMsg () {
now=$(date '+%Y-%m-%d %H:%M:%S')
echo "$now ($current_user) $1" >> $logfile
}

#================================
# Log messaging intro. header
#================================
echo "-------------------------------------------------------------------------------------"
LogMsg "[Start   ] Welcome to CSM database ras type automation script."
echo "[Start   ] Welcome to CSM database ras type automation script."

#==============================================
# Usage Command Line Functions
#==============================================

function usage () {
echo "================================================================================================="
echo "[Info ] $BASENAME : Load/Remove data from csm_ras_type table"
echo "[Usage] $BASENAME : [OPTION]... [DBNAME]... [CSV_FILE]"
echo "-------------------------------------------------------------------------------------------------"
echo "  Argument               |  DB Name  | Description                                               "
echo "-------------------------|-----------|-----------------------------------------------------------"
echo " -l, --loaddata          | [db_name] | Imports CSV data to csm_ras_type table (appends)          "
echo "                         |           | Live Row Count, Inserts, Updates, Deletes, and Table Size "
echo " -r, --removedata        | [db_name] | Removes all records from the csm_ras_type table           "
echo " -h, --help              |           | help                                                      "
echo "-------------------------|-----------|-----------------------------------------------------------"
echo "[Examples]"
echo "-------------------------------------------------------------------------------------------------"
echo "   $BASENAME -l, --loaddata           [dbname]    | [csv_file_name]                              "
echo "   $BASENAME -r, --removedata         [dbname]    |                                              "
echo "   $BASENAME -r, --removedata         [dbname]    | [csv_file_name]                              "
echo "   $BASENAME -h, --help               [dbname]    |                                              "
echo "================================================================================================="
}

#==============================================
#---Default flags---
#==============================================

loaddata="no"
removedata="no"

#==============================================
# long options to short along with fixed length
#==============================================

reset=true
for arg in "$@"
do
    if [ -n "$reset" ]; then
      unset reset
      set --      # this resets the "$@" array
    fi
    case "$arg" in
        --loaddata)         set -- "$@" -l ;;
        -loaddata)          usage && exit 0 ;;
        --removedata)       set -- "$@" -r ;;
        -removedata)        usage && exit 0 ;;
        --help)             set -- "$@" -h ;;
        -help)              usage && exit 0 ;;
        -l|-r|-h)           set -- "$@" "$arg" ;;
        #-*)                usage && exit 0 ;;
        -*)                 usage
                            LogMsg "[Info    ] Script execution: $BASENAME [NO ARGUMENT]"
                            LogMsg "[Info    ] Wrong arguments were passed in (Please choose appropriate option from usage list -h, --help)"
                            LogMsg "[End     ] Please choose another option"
                            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
                                     exit 0 ;;
        # pass through anything else
        *)                           set -- "$@" "$arg" ;;
     esac
 done

 #==============================================
 # now we can drop into the short getopts
 #==============================================
 # Also checks the existence of the file name
 # If file name is not available then an
 # error message will prompt and will be logged.
 #==============================================

 while getopts "lr:h" arg; do
     case ${arg} in
         l)
            #============================================================================
            # Populate Data in the csm_ras_type table
            # (Import from CSV file)
            # probably want to set populate below during conflict checks
            #============================================================================
             
            #----------------------------------------------------------------
            # Check if csv file exists
            #----------------------------------------------------------------

                if [ -z "$3" ]; then
                    echo "[Error   ] Please specify csv file to import"
                    LogMsg "[Error   ] Please specify csv file to import"
                    echo "-------------------------------------------------------------------------------------"
                    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
                    exit 1
                else
                loaddata="yes"
                dbname="$2"
                csv_file_name="$3"
                    if [ -f $csv_file_name ]; then
                        #echo "-------------------------------------------------------------------------------------"
                        echo "[Info    ] $3 file exists"
                        LogMsg "[Info    ] $3 file exists"
                        #LogMsg "------------------------------------------------------------------------------"
                        #echo "-------------------------------------------------------------------------------------"
                    else    
                        echo "[Error   ] File $csv_file_name can not be located or doesn't exist"
                        echo "[Info    ] Please choose another file or check path"
                        LogMsg "[Error   ] Cannot perform action because the $csv_file_name file does not exist. Exiting."
                        echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
                        echo "-------------------------------------------------------------------------------------"
                    exit 0
                    fi
                fi
             ;;
         r)
             #============================================================================
             # Remove all data from the csm_ras_type table
             # (Completely removes all data from the table)
             #============================================================================
             removedata="yes"
             dbname=$2
             csv_file_name="$3"
             ;;
         #h|*)
         h)
             #usage && exit 0
             usage
             LogMsg "[Info    ] Script execution: ./csm_db_stats.sh -h, --help"
             LogMsg "[End     ] Help menu query executed"
             echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
             exit 0
             ;;
        *)
            #usage && exit 0
            usage
            LogMsg "[Info    ] Script execution: $BASENAME [NO ARGUMENT]"
            LogMsg "[Info    ] Wrong arguments were passed in (Please choose appropriate option from usage list -h, --help)"
            LogMsg "[End     ] Please choose another option"
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
            exit 0
            ;;
    esac
done

#==============================================
# Built in checks
# Checks to see if no arguments are passed in
#==============================================

 if [ $# -eq 0 ]; then
     #usage && exit 0
     usage
     LogMsg "[Info    ] Script execution: $BASENAME [NO ARGUMENT]"
     LogMsg "[Info    ] Wrong arguments were passed in (Please choose appropriate option from usage list -h, --help)"
     LogMsg "[End     ] Please choose another option"
     echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
     exit 0
 fi

#=======================================
# Check if postgresql exists already
#=======================================

string1="$now1 ($current_user) [Info    ] DB Names:"
psql -l 2>>/dev/null $logfile

if [ $? -ne 127 ]; then       #<------------This is the error return code
db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
\set ON_ERROR_STOP true
select string_agg(datname,' | ') from pg_database;
EOF`
    echo "$string1 $db_query" | sed "s/.\{80\}|/&\n$string1 /g" >> $logfile
    echo "[Info    ] PostgreSQL is installed" #This message can be displayed to the screen as output
    LogMsg "[Info    ] PostgreSQL is installed"
    #LogMsg "---------------------------------------------------------------------------------------"
else
    echo "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
    LogMsg "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
    LogMsg "---------------------------------------------------------------------------------------"
    exit 1
fi

#======================================
# Check if database exists already
#======================================
db_exists="no"
psql -lqt | cut -d \| -f 1 | grep -qw $dbname
if [ $? -eq 0 ]; then
    db_exists="yes"
fi

#==================================================================
# End it if the input argument requires an existing database
#==================================================================
if [ $db_exists == "no" ]; then
     #==========================
     # Database does not exist
     #==========================
     LogMsg "[Info    ] $dbname database does not exist."
     echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
     LogMsg "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
     LogMsg "[End     ] Database does not exist"
     echo "-------------------------------------------------------------------------------------"
     echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
     #LogMsg "---------------------------------------------------------------------------------------"
     exit 0
fi

#----------------------------------------------------------------
# All the raw combined timing results before trimming
# Along with csm allocation history archive results
#----------------------------------------------------------------

csm_ras_type_count="csm_ras_type_count"

#-------------------------------------------------------------------------------
# psql csm_ras_type csv data import
#-------------------------------------------------------------------------------
return_code=0
ras_script_errors="$(mktemp)"
#trap 'rm -f "$ras_script_errors"' EXIT

if [ $loaddata == "yes" ]; then
count=`psql -v ON_ERROR_STOP=1 -q -t -U $db_username -d $dbname -P format=wrapped << THE_END
select count(*) from csm_ras_type;

THE_END`

import_count=`psql -v ON_ERROR_STOP=1 -q -t -U $db_username -d $dbname -P format=wrapped << THE_END
BEGIN;
LOCK TABLE csm_ras_type IN EXCLUSIVE MODE;
    DROP TABLE IF EXISTS tmp_ras_type_data;
    CREATE TEMP TABLE tmp_ras_type_data
        as
        SELECT * FROM csm_ras_type;
        \copy tmp_ras_type_data FROM '$csv_file_name' with csv header;
        SELECT count(*) FROM tmp_ras_type_data;
        INSERT INTO csm_ras_type
        SELECT DISTINCT ON (msg_id) * FROM tmp_ras_type_data
            WHERE NOT EXISTS (
                SELECT msg_id
                FROM csm_ras_type crt
                WHERE
                tmp_ras_type_data.msg_id = crt.msg_id)
        ORDER BY msg_id ASC;
        select
        (SELECT "count"(*) as cnt1 from csm_ras_type) - (SELECT "count"(*) as cnt2 from tmp_ras_type_data) as total_count;
        select count(*) from csm_ras_type;
        select count(*) from csm_ras_type_audit;
COMMIT; 
THE_END`

if [[ 0 -ne $? ]]; then
    #>> "$ras_script_errors" #| tee -a "$ras_script_errors" | \
    #awk '/^ERROR:.*$/ { print "'"$(date '+%Y-%m-%d.%H:%M:%S') ($current_user) ($BASENAME ) [Error ] "'" $0 }' | tee -a >>"${logfile}"
    echo "Something went wrong; error log follows:" >> "$ras_script_errors"
    exit 0
fi

count=$(grep -vc "^#" $csv_file_name)
set -- $import_count
Difference=$(($count + $2))

    echo "[Info    ] Record import count: $Difference"
    LogMsg "[Info    ] Record import count: $Difference"
    echo "[Info    ] csm_ras_type live row count: $3"
    LogMsg "[Info    ] csm_ras_type live row count: $3"
    echo "[Info    ] csm_ras_type_audit live row count: $4"
    LogMsg "[Info    ] csm_ras_type_audit live row count: $4"
    echo "[Info    ] Database: $dbname csv upload process complete for csm_ras_type table."
    LogMsg "[End     ] Database: $dbname csv upload process complete for csm_ras_type table."
    echo "-------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    exit $return_code
    fi

#===============================================
# Remove all data from the database tables
#===============================================
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------
return_code=0
if [ $removedata == "yes" ]; then
    if [ ! -z "$3" ]; then
            if [ -f $csv_file_name ]; then
                #echo "-------------------------------------------------------------------------------------"
                echo "[Info    ] $3 file exists"
                LogMsg "[Info    ] $3 file exists"
                #LogMsg "------------------------------------------------------------------------------"
                #echo "-------------------------------------------------------------------------------------"
            else    
                echo "[Error   ] File $csv_file_name can not be located or doesn't exist"
                echo "[Info    ] Please choose another file or check path"
                LogMsg "[Error   ] Cannot perform action because the $csv_file_name file does not exist. Exiting."
                echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
                echo "-------------------------------------------------------------------------------------"
            exit 0
            fi
    fi
    echo "[Warning ] This will drop csm_ras_type table data from $dbname database. Do you want to continue [y/n]?"
    LogMsg "[Info    ] $dbname database drop process begin."
    read -s -n 1 removedata
    case "$removedata" in
        [yY][eE][sS]|[yY])
        echo "[Info    ] User response: $removedata"
        LogMsg "[Info    ] User response: $removedata"
delete_count_csm_ras_type=`psql -q -t -U $db_username $dbname << THE_END
            BEGIN;
            LOCK TABLE csm_ras_type IN EXCLUSIVE MODE;
            with delete_1 AS(
            DELETE FROM csm_ras_type RETURNING *)
            select count(*) from delete_1;
            select count(*) from csm_ras_type;
            select count(*) from csm_ras_type_audit;
            COMMIT;
THE_END`

set -- $delete_count_csm_ras_type
            
            echo "[Info    ] Record delete count from the csm_ras_type table: $1"
            LogMsg "[Info    ] Record delete count from the csm_ras_type table: $1"
            echo "[Info    ] csm_ras_type live row count: $2"
            LogMsg "[Info    ] csm_ras_type live row count: $2"
            echo "[Info    ] csm_ras_type_audit live row count: $3"
            LogMsg "[Info    ] csm_ras_type_audit live row count: $3"
            echo "[Info    ] Data from the csm_ras_type table has been successfully removed"
            LogMsg "[Info    ] Data from the csm_ras_type table has been successfully removed"
                if [ ! -z "$csv_file_name" ]; then
                    LogMsg "[Info    ] $dbname database remove all data from the csm_ras_type table."
                    echo "-------------------------------------------------------------------------------------"
                else
                    LogMsg "[End     ] $dbname database remove all data from the csm_ras_type table."
                    echo "-------------------------------------------------------------------------------------"
                    #echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
                fi

count=`psql -q -t -U $db_username -d $dbname -P format=wrapped << THE_END
select count(*) from csm_ras_type;

THE_END`
import_count=`psql -v ON_ERROR_STOP=1 -q -t -U $db_username -d $dbname -P format=wrapped << THE_END
BEGIN;
LOCK TABLE csm_ras_type IN EXCLUSIVE MODE;
    DROP TABLE IF EXISTS tmp_ras_type_data;
    CREATE TEMP TABLE tmp_ras_type_data
        as
        SELECT * FROM csm_ras_type;
        \copy tmp_ras_type_data FROM '$csv_file_name' with csv header;
        SELECT count(*) FROM tmp_ras_type_data;
        INSERT INTO csm_ras_type
        SELECT DISTINCT ON (msg_id) * FROM tmp_ras_type_data
            WHERE NOT EXISTS (
                SELECT msg_id
                FROM csm_ras_type crt
                WHERE
                tmp_ras_type_data.msg_id = crt.msg_id)
        ORDER BY msg_id ASC;
        select
        (SELECT "count"(*) as cnt1 from csm_ras_type) - (SELECT "count"(*) as cnt2 from tmp_ras_type_data) as total_count;
        select count(*) from csm_ras_type;
        select count(*) from csm_ras_type_audit;
COMMIT;
THE_END` 2>>/dev/null

count=$(grep -vc "^#" $csv_file_name)
set -- $import_count
Difference=$(($count + $2))

    echo "[Info    ] Record import begin process"
    LogMsg "[Info    ] Record import begin process"
    echo "[Info    ] Record import count: $Difference"
    LogMsg "[Info    ] Record import count: $Difference"
    echo "[Info    ] csm_ras_type live row count: $3"
    LogMsg "[Info    ] csm_ras_type live row count: $3"
    echo "[Info    ] csm_ras_type_audit live row count: $4"
    LogMsg "[Info    ] csm_ras_type_audit live row count: $4"
    echo "[Info    ] Database csv upload process complete for csm_ras_type table."
    LogMsg "[End     ] Database csv upload process complete for csm_ras_type table."
    echo "-------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    exit $return_code
else
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    fi
        ;;
        *)
            echo "[Info    ] User response: $removedata"
            LogMsg "[Info    ] User response: $removedata"
            echo "[Info    ] Data removal from the csm_ras_type table has been aborted"
            LogMsg "[Info    ] Data removal from the csm_ras_type table has been aborted"
            LogMsg "[End     ] $dbname database attempted removal process has ended."
            echo "-------------------------------------------------------------------------------------"
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
            return_code=1
        ;;
    esac
exit $return_code
fi
