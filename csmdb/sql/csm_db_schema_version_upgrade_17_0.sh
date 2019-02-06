#!/bin/bash
#--------------------------------------------------------------------------------
#
#    csm_db_schema_version_upgrade_17_0.sh
#
#  © Copyright IBM Corporation 2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#--------------------------------------------------------------------------------

#--------------------------------------------------------------------------------
#   usage:              ./csm_db_schema_version_upgrade_17_0.sh
#   current_version:    01.0
#   migration_version:  17.0 # <--------example version after the DB upgrade
#   create:             01-30-2019
#   last modified:      02-06-2019
#--------------------------------------------------------------------------------

#set -x
export PGOPTIONS='--client-min-messages=warning'

OPTERR=0
logpath="/var/log/ibm/csm/db"
#logpath=`pwd` #<------- Change this when pushing to the repo.
logname="csm_db_schema_upgrade_script.log"
cd "${BASH_SOURCE%/*}" || exit
cur_path=`pwd`
migration_dir="$cur_path/csm_db_migration_scripts"

line1_out="------------------------------------------------------------------------------------------------------------------------"
line2_log="------------------------------------------------------------------------------------"
line3_log="---------------------------------------------------------------------------------------------------------------------------"
line4_out="-------------------------------------------------------------------------------------------------------------"

#----------------------------------------------
# Current user connected
# DB variables
#----------------------------------------------

current_user=`id -u -n`
#dbname="csmdb"
dbname="$1"
db_username="postgres"
csmdb_user="csmdb"

#----------------------------------------------
# time variable
#----------------------------------------------

now1=$(date '+%Y-%m-%d %H:%M:%S')


#----------------------------------------------
# postgresql related files
#----------------------------------------------

version_comp="csm_create_tables.sql"
version_comp_2=`grep current_version $cur_path/$version_comp | awk '{print $3}'`
trigger_version="csm_create_triggers.sql"
trigger_version_2=`grep current_version $cur_path/$trigger_version | awk '{print $3}'`
ga_version="15.0"
upgrade_16_0="16.0"
upgrade_16_1="16.1"
required_pre_migration="16.2"
migration_db_version="17.0"

csm_db_schema_csv="csm_db_schema_version_data.csv"
csm_db_schema_version_comp=`awk -F ',' ' NR==2 {print substr($0,0,4)}' $cur_path/$csm_db_schema_csv`

BASENAME=`basename "$0"`

res1=$(date +%s.%N)

#-----------------------------------------------------------------------------
# CSM DB previous and latest DB scripts
#-----------------------------------------------------------------------------

dtf="$migration_dir/csm_drop_triggers_functions.sql"
dbu_16_0="$migration_dir/csm_db_schema_updates_16_0_ms.sql"
dbu_16_2="$migration_dir/csm_db_schema_updates_16_2_ms.sql"
dbu_17_0="$migration_dir/csm_db_schema_updates_17_0_ms.sql"

drop_nd="DROP TYPE IF EXISTS node_details;"
drop_sd="DROP TYPE IF EXISTS switch_details;"
dbu_16_2_a="ALTER TYPE compute_node_states ADD VALUE 'HARD_FAILURE';"
dbu_16_2_c="COMMENT ON TYPE compute_node_states IS 'compute_node_states type to help identify the states of the compute nodes';"


#-----------------------------------------------------------------------------
# This function drops all necessary items from the db
#-----------------------------------------------------------------------------
# 1. Creates drop triggers and drop functions (functions) if does not exist.
# 2. Execute the drop triggers command.
# 3. Execute the drop functions command.
# 4. Drop the node details type.
# 5. Drop the switch details type.
#-----------------------------------------------------------------------------

function db_drop_trgs_funcs () {
db_drop_1=`psql -v ON_ERROR_STOP=1 -X -q -t -U $csmdb_user -d $dbname << THE_END 2>&1
\i $dtf
SELECT func_csm_drop_all_triggers();
SELECT * FROM func_csm_delete_func('my_function_name');
$drop_nd
$drop_sd
THE_END`
}

#-----------------------------------------------------------------------------
# This function creates all necessary items for the db from previous versions
#-----------------------------------------------------------------------------
# 1. Creates items from the 16.0 upgrade.
# 1. Creates items from the 16.2 upgrade.
#-----------------------------------------------------------------------------

db_16_2_check=`psql -v ON_ERROR_STOP=1 -A -q -t -U $csmdb_user -d $dbname << THE_END 2>&1
SELECT count(*) FROM pg_enum WHERE enumtypid = 'compute_node_states'::regtype
AND enumlabel = 'HARD_FAILURE';
THE_END`

function db_prev_ver_to_16_2 () {
db_prev_ver_16_0=`psql -v ON_ERROR_STOP=1 -q -t -U $csmdb_user -d $dbname << THE_END 2>&1
\i $dbu_16_0
\i $dbu_16_2
THE_END`
}

function db_prev_ver_16_2_type () {
db_prev_ver_16_2=`psql -v ON_ERROR_STOP=1 -q -t -U postgres -d $dbname << THE_END 2>&1
SELECT func_alt_type_val('compute_node_states','HARD_FAILURE');
$dbu_16_2_c
THE_END`
}

#-----------------------------------------------------------------------------
# This checks the existence of the default log directory.
# If the default doesn't exist it will write the log files to /tmp directory.
# The current version will only display results to the screen
#-----------------------------------------------------------------------------

if [ -w "$logpath" ]; then
    logdir="$logpath"
else
    logdir="/tmp"
fi
logfile="${logdir}/${logname}"

#----------------------------------------------
# Log Message
#----------------------------------------------

function LogMsg () {
now=$(date '+%Y-%m-%d %H:%M:%S')
echo "$now ($current_user) $1" >> $logfile
}

#--------------------------------
# Log messaging intro. header
#--------------------------------

echo "${line1_out}"
LogMsg "[Start   ] Welcome to CSM database schema version upgrade script."
LogMsg "${line2_log}" >> $logfile
echo "[Start   ] Welcome to CSM database schema version upgrade script."

#----------------------------------------------
# Usage Command Line Functions
#----------------------------------------------

function usage () {
echo "-------------------------------------------------------------------------------------------------"
echo "[Info ] $BASENAME : Load CSM DB upgrade schema file"
echo "[Usage] $BASENAME : $BASENAME [DBNAME]"
echo "-------------------------------------------------------------------------------------------------"
echo "  Argument       |  DB Name  | Description                                               "
echo "-----------------|-----------|-------------------------------------------------------------------"
echo "  script_name    | [db_name] | Imports sql upgrades to csm db table(s) (appends)         "
echo "                 |           | fields, indexes, functions, triggers, etc                 "
echo "-----------------|-----------|-------------------------------------------------------------------"
echo "-------------------------------------------------------------------------------------------------"
}

#---------------------------------------------
# The optstring for input.
#---------------------------------------------

optstring="h"

while getopts $optstring OPTION
do
    case $OPTION in
        h|*)
            usage; exit 1;;
    esac
done

#----------------------------------------------------------------
# Check if dbname exists
#----------------------------------------------------------------

if [ -z "$1" ]; then
    echo "[Error   ] Please specify DB name"
    LogMsg "[Error   ] Please specify DB name"
    usage
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 0
else
    dbname=$1
fi

#--------------------------------------------------
# Check if postgresql exists already and root user
#--------------------------------------------------

string1="$now1 ($current_user) [Info    ] DB Users:"
    psql -U $db_username -t -c '\du' | cut -d \| -f 1 | grep -qw root
        
        #--------------------------------
        # If it does not exist then exit
        #--------------------------------

        if [ $? -ne 0 ]; then
            db_user_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
            \set ON_ERROR_STOP true
            select string_agg(usename,' | ') from pg_user;
EOF`
            echo "$string1 $db_user_query" | sed "s/.\{60\}|/&\n$string1 /g" >> $logfile
            echo "${line1_out}"
            echo "[Error   ] Postgresql may not be configured correctly. Please check configuration settings."
            LogMsg "[Error   ] Postgresql may not be configured correctly. Please check configuration settings."
            echo "${line1_out}"
            echo "${line3_log}" >> $logfile
            exit 0
        fi

#-------------------------------------------------
# Check if postgresql exists already and DB name
#-------------------------------------------------

string2="$now1 ($current_user) [Info    ] DB Names:"
    psql -lqt | cut -d \| -f 1 | grep -qw $dbname 2>>/dev/null
        
        #--------------------------------
        # If it does not exist then exit
        #--------------------------------
        
        if [ $? -eq 0 ]; then       #<------------This is the error return code
            db_query=`psql -U $db_username -q -A -t -P format=wrapped <<EOF
            \set ON_ERROR_STOP true
            select string_agg(datname,' | ') from pg_database;
EOF`
            echo "$string2 $db_query" | sed "s/.\{60\}|/&\n$string2 /g" >> $logfile
            LogMsg "[Info    ] PostgreSQL is installed"
            LogMsg "${line2_log}" >> $logfile
        else
            echo "[Error   ] PostgreSQL may not be installed or DB: $dbname may not exist."
            echo "[Info    ] Please check configuration settings or psql -l"
            echo "${line1_out}"
            LogMsg "[Error   ] PostgreSQL may not be installed or DB $dbname may not exist."
            LogMsg "[Info    ] Please check configuration settings or psql -l"
            echo "${line3_log}" >> $logfile
            exit 1
        fi

#--------------------------------------
# Check if database exists already
#--------------------------------------

db_exists="no"

psql -lqt | cut -d \| -f 1 | grep -qw $dbname

if [ $? -eq 0 ]; then
    db_exists="yes"
fi

#------------------------------------------------------------------
# End it if the input argument requires an existing database
#------------------------------------------------------------------

if [ $db_exists == "no" ]; then
     
     #--------------------------
     # Database does not exist
     #--------------------------
     
     LogMsg "[Info    ] $dbname database does not exist."
     echo "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
     LogMsg "[Error   ] Cannot perform action because the $dbname database does not exist. Exiting."
     LogMsg "[End     ] Database does not exist"
     echo "${line1_out}"
     echo "${line3_log}" >> $logfile
     exit 0
fi

#------------------------------------------------------------------------
# CSM_database_files
#------------------------------------------------------------------------
# checks:
#------------------------------------------------------------------------
# 1. Check existence of csv file (if exists read version # in) else exit
# 3. Check the csmdb schema version
# 4. Check if the csm_create_tables is up-to-date with latest version.
#------------------------------------------------------------------------

#------------------------------------------------------------------------
# The schema version csv file will be one of the comparison files
# used to determaine the upgrade.
#------------------------------------------------------------------------

version=`psql -v ON_ERROR_STOP=1 -X -A -U $db_username -d $dbname -t -c "SELECT version FROM csm_db_schema_version" 2>>/dev/null`

#---------------------------------------
# If no csm_db_schema_version then exit
#---------------------------------------

if [ $? -ne 0 ]; then
    echo "[Error   ] Cannot perform action because DB: $dbname is not compatible."
    LogMsg "[Error   ] Cannot perform action because DB: $dbname is not compatible."
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 0
fi

#----------------------------------------------------------
# Checks the files and compares to db schema version table
# If they all match then it will display
# currently running db schema version: $version"
#----------------------------------------------------------

if [[ $(bc <<< "${version} == ${migration_db_version}") -eq 1 ]]; then
    echo "[Info    ] $dbname is currently running db schema version: $version"
    echo "[Info    ] Log Dir: $logdir/$logname"
    LogMsg "[Info    ] $dbname is currently running db schema version: $version"
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 0
else
    echo "[Info    ] $dbname current_schema_version is running: $version"
    echo "[Info    ] Log Dir: $logdir/$logname"
    LogMsg "[Info    ] $dbname current_schema_version is running: $version"
fi

#-----------------------------------------
# Checks to see if running the GA release
#-----------------------------------------

if [[ $(bc <<< "${version}") < "$ga_version" ]]; then
    echo "[Error   ] The database migration script does not support versions below $ga_version"
    LogMsg "[Error   ] The database migration script does not support versions below $ga_version"
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 0
fi

#-----i------------------------------------------------------------------
# Checks to see if migration file exists
# in the directory
#------------------------------------------------------------------------

if [ -f $dtf ] && [ -f $dbu_16_0 ] && [ -f $dbu_16_2 ] && [ -f $dbu_17_0 ]; then
    echo "[Info    ] $dbname schema_version_upgrade_files exist"
    LogMsg "[Info    ] $dbname schema_version_upgrade_files exist"
else
    echo "[Error   ] Cannot perform action because the migration files may not exist."
    LogMsg "[Error   ] Cannot perform action because the migration files may not exist."
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 1
fi

#----------------------------------------------------------
# Checks the existing version which to migrate from 
#----------------------------------------------------------

if [[ $(bc <<< "${version}") < "$required_pre_migration" ]]; then
    #echo "${line1_out}"
    echo "[Info    ] ${line4_out}"
    echo "[Info    ] There are critical migration steps needed to get to the latest schema version: $migration_db_version"
    LogMsg "[Info    ] There are critical migration steps needed to get to the latest schema version: $migration_db_version"
    

    #----------------------------------------------------------
    # Check which version before the migration process.
    # Then display which steps are needed to get to the 
    # lastest schema version.
    #----------------------------------------------------------

    if [[ $(bc <<< "${version}") == 15.0 ]]; then
        echo "[Info    ] These include versions 15.1, 16.0, 16.1 and 16.2"
        LogMsg "[Info    ] These include versions 15.1, 16.0, 16.1 and 16.2"
        echo "[Warning ] Do you want to continue [y/n]?:" 
    elif [[ $(bc <<< "${version}") == 15.1 ]]; then
        echo "[Info    ] These include versions 16.0, 16.1 and 16.2"
        LogMsg "[Info    ] These include versions 16.0, 16.1 and 16.2"
        echo "[Warning ] Do you want to continue [y/n]?:"
    elif [[ $(bc <<< "${version}") == 16.0 ]]; then
        echo "[Info    ] These include versions 16.1 and 16.2"
        LogMsg "[Info    ] These include versions 16.1 and 16.2"
        echo "[Warning ] Do you want to continue [y/n]?:"
    elif [[ $(bc <<< "${version}") == 16.1 ]]; then
        echo "[Info    ] This includes version 16.2"
        LogMsg "[Info    ] This includes versions 16.2"
        echo "[Warning ] Do you want to continue [y/n]?:"
    fi

#----------------------------------------------------------
# Read in the users response from prompt
#----------------------------------------------------------

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
        echo "${line1_out}"
        echo "${line3_log}" >> $logfile
        exit 0
    fi
fi

#------------------------------
# Drop Triggers and Functions
# for all previous versions
#------------------------------

if [[ $(bc <<< "${version}") < "$migration_db_version" ]]; then

db_drop_trgs_funcs 2>&1
    
    #----------------------------------------------------------------
    # This checks the return code of the drop process
    # if there is a DB error message then this is captured in the
    # log file.
    #----------------------------------------------------------------

    if [[ $? -ne 0 ]]; then
         echo "$(db_drop_trgs_funcs)" |& awk '/^ERROR:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        echo "[Error   ] Cannot perform drop triggers and drop functions command for version: $version"
        LogMsg "[Error   ] Cannot perform drop triggers and drop functions command for version: $version"
        echo "${line1_out}"
        echo "${line3_log}" >> $logfile
    fi
fi

#----------------------------------------------------------------
# Previous db upgrades that get executed
#----------------------------------------------------------------

if [[ $(bc <<< "${version}") < "$required_pre_migration" ]]; then

db_prev_ver_to_16_2 2>&1

    #----------------------------------------------------------------
    # This checks the return code of the upgrade process
    # if there is a DB error message then this is captured in the
    # log file.
    #----------------------------------------------------------------

    if [[ $? -ne 0 ]]; then
         echo "$(db_prev_ver_to_16_2)" |& awk '/^ERROR:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        echo "[Error   ] Cannot perform the upgrade process to $required_pre_migration"
        LogMsg "[Error   ] Cannot perform the upgrade process to $required_pre_migration"
        exit 0
    fi
    
    #----------------------------------------------------------------
    # This checks to see if the db "type" exists
    # if not then it will create it.
    #----------------------------------------------------------------
    
    if [[ "${db_16_2_check}" -eq 0 ]]; then
        
        db_prev_ver_16_2_type 2>&1
    fi

    #----------------------------------------------------------------
    # This checks the return code of the upgrade process
    # if there is a DB error message then this is captured in the
    # log file.
    #----------------------------------------------------------------

    if [[ $? -ne 0 ]]; then
         echo "$(db_prev_ver_16_2_type)" |& awk '/^ERROR:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        echo "[Error   ] Cannot perform the upgrade process to $required_pre_migration"
        LogMsg "[Error   ] Cannot perform the upgrade process to $required_pre_migration"
        exit 0
    else
        echo "[Info    ] ${line4_out}"
        echo "[Info    ] Migration from $version to $required_pre_migration [Complete]"
        LogMsg "[Info    ] Migration from $version to $required_pre_migration [Complete]"
        echo "[Info    ] ${line4_out}"
        LogMsg "${line2_log}" >> $logfile
    fi
fi

#----------------------------------------------------------
# Then you can proceed to migrated to the latest version
#----------------------------------------------------------

#----------------------------------------------------------
# Then checks the files and compares to see if they are
# previous versions currently running that are not up
# to date.
#----------------------------------------------------------

if [[ $(bc <<< "${version} < ${version_comp_2}") -eq 0 ]] || [[ $(bc <<< "${version} < ${csm_db_schema_version_comp}") -eq 0 ]] || [[ $(bc <<< "${version} < $ga_version") -eq 1 ]] || [[ $(bc <<< "${migration_db_version} == ${trigger_version_2}") -eq 0 ]]; then
    echo "[Error   ] Cannot perform action because not compatible."
    echo "[Info    ] Required DB schema version 15.0, 15.1, 16.0, 16.1, 16.2 or appropriate files in directory"
    LogMsg "[Error   ] Cannot perform action because not compatible."
    LogMsg "[Info    ] Required DB schema version 15.0, 15.1, 16.0, 16.1, 16.2 or appropriate files in directory"
    echo "[Info    ] csm_create_tables.sql file currently in the directory is: $version_comp_2 (required version) $migration_db_version"
    LogMsg "[Info    ] csm_create_tables.sql file currently in the directory is: $version_comp_2 (required version) $migration_db_version"
    echo "[Info    ] csm_create_triggers.sql file currently in the directory is: $trigger_version_2 (required version) $migration_db_version"
    LogMsg "[Info    ] csm_create_triggers.sql file currently in the directory is: $trigger_version_2 (required version) $migration_db_version"
    echo "[Info    ] csm_db_schema_version_data.csv file currently in the directory is: $csm_db_schema_version_comp (required version) $migration_db_version"
    LogMsg "[Info    ] csm_db_schema_version_data.csv file currently in the directory is: $csm_db_schema_version_comp (required version) $migration_db_version"
    echo "[Info    ] Please make sure you have the latest RPMs installed and latest DB files."
    LogMsg "[Info    ] Please make sure you have the latest RPMs installed and latest DB files."
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
exit 0
fi

#------------------------------
# Checks to see if file exists
# and version # (csv)
#------------------------------

if [ -f $cur_path/$csm_db_schema_csv ]; then
    updated_version=`awk -F ',' ' NR==2 {print $1}' $cur_path/$csm_db_schema_csv`
    echo "[Info    ] $dbname current_schema_version.csv $updated_version"
    LogMsg "[Info    ] $dbname current_schema_version.csv $updated_version"
else
    echo "[Error   ] Cannot perform action because the $csm_db_schema_csv file does not exist."
    echo "${line1_out}"
    exit 1
fi

#------------------------------
# Checks to see if file exists
# and version # (create tables)
#------------------------------

if [ -f $cur_path/$version_comp ]; then
    echo "[Info    ] $dbname schema_version_upgrade_tables: $migration_db_version"
    LogMsg "[Info    ] $dbname schema_version_upgrade_tables: $migration_db_version"
else
    echo "[Error   ] Cannot perform action because the $version_comp is not compatible."
    LogMsg "[Error   ] Cannot perform action because the $version_comp is not compatible."
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 1
fi

#----------------------------------------------------------------------------------------------------
# This queries the actual database to see which version it
# is currently running.
#----------------------------------------------------------------------------------------------------
#version=`psql -X -A -U $db_username -d $dbname -t -c "SELECT version FROM csm_db_schema_version"`
# return code added to ensure it was successful or failed during this step
#----------------------------------------------------------------------------------------------------

upgradedb="no"
return_code=0

#----------------------------------------------------------------------------------------------------
# Checks before the migration process
#----------------------------------------------------------------------------------------------------
# 1. Current db is less than the upgrade version.
# 2. Migration script version is equal to the create_tables.sql file
#----------------------------------------------------------------------------------------------------

if [[ $(bc <<< "$version < $updated_version") -eq 1 ]] && [ $migration_db_version == $version_comp_2 ]; then
  echo "[Warning ] This will migrate $dbname database to schema version $migration_db_version. Do you want to continue [y/n]?:"

#---------------------------------------------------------
# Read in the users response confirming migration process
#---------------------------------------------------------

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
    echo "${line1_out}"
    echo "${line3_log}" >> $logfile
    exit 0
fi 
    #--------------------------------
    # Checks the current connections
    #--------------------------------
    
    connections_count=`psql -v ON_ERROR_STOP=1 -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname';"`
    
    #-----------------------------------------------
    # If there are connections the script will exit
    #-----------------------------------------------
    
    if [ $connections_count -gt 0 ]; then
        LogMsg "[Error   ] $dbname can not proceed because of existing connection(s) to the database."
        psql -v ON_ERROR_STOP=1 -t -A -U $db_username -c "select to_char(now(),'YYYY-MM-DD HH24:MI:SS') || ' ($current_user) [Info    ] Current Connection | User: ' || usename || ' ',' Datebase: ' \
        || datname, ' Connection(s): ' || count(*), ' Duration: ' || now() - backend_start as Duration \
        from pg_stat_activity WHERE datname='$dbname' group by usename, datname, Duration order by datname, Duration desc;" &>> $logfile
        connections=`psql -t -U $db_username -c "select distinct usename from pg_stat_activity WHERE datname='$dbname';"`
        
        #-----------------------------------------------------
        # Array created for each connections that are present
        #-----------------------------------------------------

        if [ ${#connections[@]} -gt 0 ]; then
            echo "[Error   ] $dbname has existing connection(s) to the database."
            
            #----------------------------------------------
            # Loops through in case of multiple connections
            #----------------------------------------------
            
            for i in ${connections[@]}; do
                connection_count=`psql -v ON_ERROR_STOP=1 -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname' and usename='$i';"`
                shopt -s extglob
                connection_count="${connection_count##*( )}"
                connection_count="${connection_count%%*( )}"
                echo "[Error   ] User: $i has $connection_count connection(s)"
            done
            echo "[Info    ] See log file for connection details"
            echo "${line1_out}"
            echo "${line3_log}" >> $logfile
        fi
    else
    echo "[Info    ] There are no connections to $dbname"
    LogMsg "[Info    ] There are no connections to $dbname"

#---------------------------------------------
# CSM DB schema upgrade to the latest version
#---------------------------------------------
    
string2="$now1 ($current_user) [Error   ]"
string3="$db_query_2"

db_query_2=`psql -v ON_ERROR_STOP=1 -q -t -U $csmdb_user -d $dbname << THE_END 2>&1
\i $dbu_17_0
\i $trigger_version

UPDATE csm_db_schema_version
SET
version = '$migration_db_version',
create_time = 'now()'
where $migration_db_version > $version;
THE_END` 

    #----------------------------------------------------------------
    # This checks the return code of the latest upgrade process
    # if there is a DB error message then this is captured in the
    # log file.
    #----------------------------------------------------------------
    
    if [[ $? -ne 0 ]]; then
         echo "$db_query_2"
         echo "$db_query_2" |& awk '/^ERROR:.*$/{$1=""; gsub(/^[ \t]+|[ \t]+$/,""); print "'"$(date '+%Y-%m-%d %H:%M:%S') ($current_user) [Error   ] DB Message: "'"$0}' >>"${logfile}"
        echo "[Error   ] Cannot perform the upgrade process to $migration_db_version"
        LogMsg "[Error   ] Cannot perform the upgrade process to $migration_db_version"
        echo "[Info    ] ${line2_log}"
        LogMsg "${line2_log}" >> $logfile
        exit 0
    else
        echo "[Info    ] ${line4_out}"
        echo "[Info    ] Migration from $required_pre_migration to $migration_db_version [Complete]"
        LogMsg "[Info    ] Migration from $required_pre_migration to $migration_db_version [Complete]"
        #echo "[Info    ] ${line2_log}"
        #LogMsg "${line2_log}" >> $logfile
    fi

#---------------------------------------------
# Automated RAS script update/inserting
# new message types
#---------------------------------------------

./csm_db_ras_type_script.sh -l $dbname csm_ras_type_data.csv

res2=$(date +%s.%N)
dt=$(echo "$res2 - $res1" | bc)
dd=$(echo "$dt/86400" | bc)
dt2=$(echo "$dt-86400*$dd" | bc)
dh=$(echo "$dt2/3600" | bc)
dt3=$(echo "$dt2-3600*$dh" | bc)
dm=$(echo "$dt3/60" | bc)
ds=$(echo "$dt3-60*$dm" | bc)

#----------------------------------------------------------------
# This checks the return code of the migration script process
# if there is an error message then this is captured in the
# log file.
#----------------------------------------------------------------

if [ $? -eq 0  ]; then
        echo "[Complete] $dbname database schema update $migration_db_version."
        echo "${line1_out}"
        #echo "${line3_log}" >> $logfile
        printf "[Timing  ] %d:%02d:%02d:%02.4f\n" $dd $dh $dm $ds
            LogMsg "${line2_log}" >> $logfile
            LogMsg "[Complete] $dbname database schema update $migration_db_version."
            LogMsg "${line2_log}" >> $logfile
            LogMsg "[Timing  ] $dd:$dh:$dm:0$ds"
            echo "${line1_out}"
            echo "${line3_log}" >> $logfile
            exit 1
        else
        echo "[Error   ] Database schema update failed for $dbname"
            echo "[Error   ] $db_query_2"
            LogMsg "[Error   ] Database schema update failed for $dbname"
            echo "$string2 $db_query_2" >>$logfile
            echo "${line1_out}"
            echo "${line3_log}" >> $logfile
            exit 1
            #return_code=1
        fi
fi
else
echo "[Info    ] $dbname is currently running db schema version: $version"
LogMsg "[Info    ] $dbname is currently running db schema version: $version"
echo "${line1_out}"
echo "${line3_log}" >> $logfile
#fi
exit $return_code
fi
