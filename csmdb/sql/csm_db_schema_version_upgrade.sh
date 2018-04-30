#!/bin/bash
#================================================================================
#
#    csm_db_schema_version_upgrade.sh
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
#   usage:         ./csm_db_schema_version_upgrade.sh
#   version:       01.0
#   db_version:    14.9 # <--------example version after the DB upgrade
#   create:        12-13-2017
#   last modified: 01-19-2018
#================================================================================

export PGOPTIONS='--client-min-messages=warning'

OPTERR=0
logpath="/var/log/ibm/csm/db"
#logpath=`pwd` #<------- Change this when pushing to the repo.
logname="csm_db_schema_upgrade_script.log"
cd "${BASH_SOURCE%/*}" || exit
cur_path=`pwd`

#==============================================
# Current user connected---
#==============================================
current_user=`id -u -n`
dbname="csmdb"
db_username="postgres"
csmdb_user="csmdb"
now1=$(date '+%Y-%m-%d %H:%M:%S')
version_comp="csm_db_schema_upgrade_v_14_9.sql"
version_comp_2=`awk -F ',' ' NR==18 {print substr($0,20,6)}' $cur_path/$version_comp`
migration_db_version="14.9"
csm_db_schema_csv="csm_db_schema_version_data.csv"

BASENAME=`basename "$0"`

#=============================================================================
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files to /tmp directory.
# The current version will only display results to the screen
#=============================================================================

if [ -w "$logpath" ]; then
    logdir="$logpath"
else
    logdir="/tmp"
fi
logfile="${logdir}/${logname}"

#==============================================
# Log Message
#==============================================

function LogMsg () {
now=$(date '+%Y-%m-%d %H:%M:%S')
echo "$now ($current_user) $1" >> $logfile
}

#================================
# Log messaging intro. header
#================================
echo "-------------------------------------------------------------------------------------------------"
LogMsg "[Start   ] Welcome to CSM database schema version upgrate script."
echo "[Start   ] Welcome to CSM database schema version upgrate script."

#==============================================
# Usage Command Line Functions
#==============================================

function usage () {
echo "================================================================================================="
echo "[Info ] $BASENAME : Load CSM DB upgrade schema file"
echo "[Usage] $BASENAME : $BASENAME [DBNAME]"
echo "-------------------------------------------------------------------------------------------------"
echo "  Argument       |  DB Name  | Description                                               "
echo "-----------------|-----------|-------------------------------------------------------------------"
echo "  script_name    | [db_name] | Imports sql upgrates to csm db table(s) (appends)         "
echo "                 |           | fields, indexes, functions, triggers, etc                 "
echo "-----------------|-----------|-------------------------------------------------------------------"
echo "================================================================================================="
}

#================================================================
# Check if dbname exists
#================================================================

if [ -z "$1" ]; then
    echo "[Error   ] Please specify DB name"
    LogMsg "[Error   ] Please specify DB name"
    usage
    echo "-------------------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    exit 0
else
    dbname=$1
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
     echo "[Info    ] PostgreSQL is installed"
     LogMsg "[Info    ] PostgreSQL is installed"
     #LogMsg "---------------------------------------------------------------------------------------"
else
     echo "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
     LogMsg "[Error   ] PostgreSQL may not be installed. Please check configuration settings"
     LogMsg "---------------------------------------------------------------------------------------"
     #exit 1
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
     echo "-------------------------------------------------------------------------------------------------"
     echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
     #LogMsg "---------------------------------------------------------------------------------------"
     exit 0
fi

#========================================================================
# CSM_database_files
#========================================================================
# checks:
#------------------------------------------------------------------------
# 1. check existence of csv file (if exists read version # in) else exit
# 2. check to see if upgraded migration script exits else exit
# 3. Check the csmdb schema version
#========================================================================

#=================================================================
# The schema version csv file will be one of the comparison files
# used to determaine the upgrade.
#=================================================================
version=`psql -X -A -U $db_username -d $dbname -t -c "SELECT version FROM csm_db_schema_version"`

if [ -f $cur_path/$csm_db_schema_csv ]; then
    updated_version=`awk -F ',' ' NR==2 {print $1}' $cur_path/$csm_db_schema_csv`
    echo "[Info    ] $dbname current_schema_version $version"
    LogMsg "[Info    ] $dbname current_schema_version $version"
else
    echo "[Error   ] Cannot perform action because the $csm_db_schema_csv file does not exist."
    exit 1
fi

if [ -f $cur_path/$version_comp ]; then
    echo "[Info    ] $dbname schema_version_upgrade: $migration_db_version"
    LogMsg "[Info    ] $dbname schema_version_upgrade: $migration_db_version"
else
    echo "[Error   ] Cannot perform action because the $migration_db_version is not compatible."
    exit 1
fi
#=================================================================
# This queries the actual database to see which version it
# is currently running.
#=================================================================
#version=`psql -X -A -U $db_username -d $dbname -t -c "SELECT version FROM csm_db_schema_version"`
# return code added to ensure it was successful or failed during this step
#-------------------------------------------------------------------------

upgradedb="no"

return_code=0

if [[ $(bc <<< "$version <= $updated_version") -eq 1 ]] && [ $migration_db_version == $version_comp_2 ]; then
  echo "[Warning ] This will migrate $dbname database to schema version $migration_db_version. Do you want to continue [y/n]?:"
read -s -n 1 confirm
if [ "$confirm" = "y" ]; then
        echo "[Info    ] User response: $confirm"
        LogMsg "[Info    ] User response: $confirm"
        echo "[Info    ] $dbname migration process begin."
        LogMsg "[Info    ] $dbname migration process begin."
else
    echo "[Info    ] User response: $confirm"
    LogMsg "[Info    ] User response: $confirm"
    echo "[Error   ] Migration session for DB: $dbname User response: ****(NO)****  not updated"
    LogMsg "[Error   ] Migration session for DB: $dbname User response: ****(NO)****  not updated"
    echo "-------------------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
    exit 0
fi 
    #===============================
    # Checks the current connections
    #===============================
    connections_count=`psql -t -U $csmdb_user -c "select count(*) from pg_stat_activity WHERE datname='$dbname';"`
    #--------------------------------------------------------------------------------------------------------------
    if [ $connections_count -gt 0 ]; then
        LogMsg "[Error   ] $dbname will not be dropped because of existing connection(s) to the database."
        psql -t -A -U $csmdb_user -c "select to_char(now(),'YYYY-MM-DD HH24:MI:SS') || ' ($current_user) [Info    ] Current Connection | User: ' || usename || ' ',' Datebase: ' \
        || datname, ' Connection(s): ' || count(*), ' Duration: ' || now() - backend_start as Duration \
        from pg_stat_activity WHERE datname='$dbname' group by usename, datname, Duration order by datname, Duration desc;" &>> $logfile
        connections=`psql -t -U $csmdb_user -c "select distinct usename from pg_stat_activity WHERE datname='$dbname';"`
        #-------------------------------------------------------------------------------------------------------------------
        if [ ${#connections[@]} -gt 0 ]; then
            echo "[Error   ] $dbname has existing connection(s) to the database."
            #==============================================
            # Loops through in case of multiple connections
            #==============================================
            #----------------------------------------------------------------------------------------------------------------------------------
            for i in ${connections[@]}; do
                connection_count=`psql -t -U $csmdb_user -c "select count(*) from pg_stat_activity WHERE datname='$dbname' and usename='$i';"`
                shopt -s extglob
                connection_count="${connection_count##*( )}"
                connection_count="${connection_count%%*( )}"
                echo "[Error   ] User: $i has $connection_count connection(s)"
            done
            echo "[Info    ] See log file for connection details"
            echo "-------------------------------------------------------------------------------------------------"
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
        fi
    else
    echo "[Info    ] There are no connections to $dbname"
    LogMsg "[Info    ] There are no connections to $dbname"
    #=============================================
    # Reads in the csm_db_schema_upgrade.sql file
    #=============================================
    string2="$now1 ($current_user) [Error   ]"
    string3="$db_query_2"
    db_query_2=`psql -q -U $csmdb_user -d $dbname -f $version_comp 2>&1`
        if [ $? -eq 0  ]; then
        echo "[Complete] $dbname database schema update $migration_db_version."
            LogMsg "[Complete] $dbname database schema update $migration_db_version."
            echo "-------------------------------------------------------------------------------------------------"
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
            exit 1
        else
        echo "[Error   ] Database schema update failed for $dbname"
            echo "[Error   ] $db_query_2"
            LogMsg "[Error   ] Database schema update failed for $dbname"
            echo "$string2 $db_query_2" >>$logfile
            echo "-------------------------------------------------------------------------------------------------"
            echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
            exit 1
            #return_code=1
        fi
fi
echo "[Info    ] $dbname is currently running db schema version: $version"
#echo "[Error   ] The database is upgraded to the most recent version"
else
echo "[Info    ] $dbname is currently running db schema version: $version"
LogMsg "[Info    ] $dbname is currently running db schema version: $version"
echo "-------------------------------------------------------------------------------------------------"
echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
#fi
exit $return_code
fi
