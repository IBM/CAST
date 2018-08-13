#!/bin/bash
#================================================================================
#
#    csm_db_schema_version_upgrade_16_0.sh
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
#================================================================================

#================================================================================
#   usage:              ./csm_db_schema_version_upgrade_16_0.sh
#   current_version:    01.1
#   migration_version:  16.0 # <--------example version after the DB upgrade
#   create:             08-08-2018
#   last modified:      08-13-2018
#================================================================================
#set -x
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
version_comp="csm_create_tables.sql"
version_comp_2=`grep current_version $cur_path/$version_comp | awk '{print $3}'`
trigger_version="csm_create_triggers.sql"
trigger_version_2=`grep current_version $cur_path/$trigger_version | awk '{print $3}'`

migration_db_version="16.0"
csm_db_schema_csv="csm_db_schema_version_data.csv"
csm_db_schema_version_comp=`awk -F ',' ' NR==2 {print substr($0,0,4)}' $cur_path/$csm_db_schema_csv`

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
echo "  script_name    | [db_name] | Imports sql upgrades to csm db table(s) (appends)         "
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
# 3. Check the csmdb schema version
# 4. Check if the csm_create_tables is up-to-date with version 16.0
#========================================================================

#=================================================================
# The schema version csv file will be one of the comparison files
# used to determaine the upgrade.
#=================================================================
version=`psql -X -A -U $db_username -d $dbname -t -c "SELECT version FROM csm_db_schema_version"`


#----------------------------------------------------------
# Checks the files and compares to db schema version table
# If they all match then it will display
# currently running db schema version: $version"
#----------------------------------------------------------

if [[ $(bc <<< "$version == $migration_db_version") -eq 1 ]]; then
echo "[Info    ] $dbname is currently running db schema version: $version"
LogMsg "[Info    ] $dbname is currently running db schema version: $version"
echo "-------------------------------------------------------------------------------------------------"
echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
exit 0
fi

#----------------------------------------------------------
# Then checks the files and compares to see if they are
# previous versions currently running that are not up
# to date.
#----------------------------------------------------------

if [[ $(bc <<< "$version < $version_comp_2") -eq 0 ]] || [[ $(bc <<< "$version < $csm_db_schema_version_comp") -eq 0 ]] || [[ $(bc <<< "$version < 15.0") -eq 1 ]] || [[ $(bc <<< "$migration_db_version == $trigger_version_2") -eq 0 ]]; then
    echo "[Error   ] Cannot perform action because not compatible."
    echo "[Info    ] Required DB schema version 15.0, 15.1 or appropriate files in directory"
    LogMsg "[Error   ] Cannot perform action because not compatible."
    LogMsg "[Info    ] Required DB schema version 15.0, 15.1 or appropriate files in directory"
    echo "[Info    ] $dbname current_schema_version is running: $version"
    LogMsg "[Info    ] $dbname current_schema_version is running: $version"
    echo "[Info    ] csm_create_tables.sql file currently in the directory is: $version_comp_2 (required version) $migration_db_version"
    LogMsg "[Info    ] csm_create_tables.sql file currently in the directory is: $version_comp_2 (required version) $migration_db_version"
    echo "[Info    ] csm_create_triggers.sql file currently in the directory is: $trigger_version_2 (required version) $migration_db_version"
    LogMsg "[Info    ] csm_create_triggers.sql file currently in the directory is: $trigger_version_2 (required version) $migration_db_version"
    echo "[Info    ] csm_db_schema_version_data.csv file currently in the directory is: $csm_db_schema_version_comp (required version) $migration_db_version"
    LogMsg "[Info    ] csm_db_schema_version_data.csv file currently in the directory is: $csm_db_schema_version_comp (required version) $migration_db_version"
    echo "[Info    ] Please make sure you have the latest RPMs installed and latest DB files."
    LogMsg "[Info    ] Please make sure you have the latest RPMs installed and latest DB files."
    echo "-------------------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
exit 0
fi

#------------------------------
# Checks to see if file exists
# and version # (csv)
#------------------------------

if [ -f $cur_path/$csm_db_schema_csv ]; then
    updated_version=`awk -F ',' ' NR==2 {print $1}' $cur_path/$csm_db_schema_csv`
    echo "[Info    ] $dbname current_schema_version $version"
    LogMsg "[Info    ] $dbname current_schema_version $version"
else
    echo "[Error   ] Cannot perform action because the $csm_db_schema_csv file does not exist."
    exit 1
fi

#------------------------------
# Checks to see if file exists
# and version # (create tables)
#------------------------------

if [ -f $cur_path/$version_comp ]; then
    echo "[Info    ] $dbname schema_version_upgrade: $migration_db_version"
    LogMsg "[Info    ] $dbname schema_version_upgrade: $migration_db_version"
else
    echo "[Error   ] Cannot perform action because the $version_comp is not compatible."
    LogMsg "[Error   ] Cannot perform action because the $version_comp is not compatible."
    echo "-------------------------------------------------------------------------------------------------"
    echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
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

if [[ $(bc <<< "$version < $updated_version") -eq 1 ]] && [ $migration_db_version == $version_comp_2 ]; then
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
    connections_count=`psql -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname';"`
    #--------------------------------------------------------------------------------------------------------------
    if [ $connections_count -gt 0 ]; then
        LogMsg "[Error   ] $dbname will not be dropped because of existing connection(s) to the database."
        psql -t -A -U $db_username -c "select to_char(now(),'YYYY-MM-DD HH24:MI:SS') || ' ($current_user) [Info    ] Current Connection | User: ' || usename || ' ',' Datebase: ' \
        || datname, ' Connection(s): ' || count(*), ' Duration: ' || now() - backend_start as Duration \
        from pg_stat_activity WHERE datname='$dbname' group by usename, datname, Duration order by datname, Duration desc;" &>> $logfile
        connections=`psql -t -U $db_username -c "select distinct usename from pg_stat_activity WHERE datname='$dbname';"`
        #-------------------------------------------------------------------------------------------------------------------
        if [ ${#connections[@]} -gt 0 ]; then
            echo "[Error   ] $dbname has existing connection(s) to the database."
            #==============================================
            # Loops through in case of multiple connections
            #==============================================
            #----------------------------------------------------------------------------------------------------------------------------------
            for i in ${connections[@]}; do
                connection_count=`psql -t -U $db_username -c "select count(*) from pg_stat_activity WHERE datname='$dbname' and usename='$i';"`
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

#==========================================================================

    #=============================================
    # CSM DB schema upgrade
    #=============================================
    string2="$now1 ($current_user) [Error   ]"
    string3="$db_query_2"
    #db_query_2=`psql -q -U $csmdb_user -d $dbname -f $version_comp 2>&1`
    
    db_query_2=`psql -q -t -U $db_username -d $dbname << THE_END

DROP TRIGGER IF EXISTS tr_csm_allocation_update ON csm_allocation CASCADE;
DROP TRIGGER IF EXISTS tr_csm_allocation_state_change ON csm_allocation CASCADE;
DROP TRIGGER IF EXISTS tr_csm_node_update ON csm_node CASCADE;
DROP TRIGGER IF EXISTS tr_csm_node_state ON csm_node CASCADE;
DROP TRIGGER IF EXISTS tr_csm_allocation_node_change ON csm_allocation_node CASCADE;
DROP TRIGGER IF EXISTS tr_csm_step_node_history_dump ON csm_step_node CASCADE;
DROP TRIGGER IF EXISTS tr_csm_ras_type_update ON csm_ras_type CASCADE;
DROP TRIGGER IF EXISTS tr_csm_vg_history_dump ON csm_vg CASCADE;
DROP TRIGGER IF EXISTS tr_csm_vg_ssd_history_dump ON csm_vg_ssd CASCADE;
DROP TRIGGER IF EXISTS tr_csm_ssd_history_dump ON csm_ssd CASCADE;
DROP TRIGGER IF EXISTS tr_csm_ssd_wear ON csm_ssd CASCADE;
DROP TRIGGER IF EXISTS tr_csm_processor_socket_history_dump ON csm_processor_socket CASCADE;
DROP TRIGGER IF EXISTS tr_csm_gpu_history_dump ON csm_gpu CASCADE;
DROP TRIGGER IF EXISTS tr_csm_hca_history_dump ON csm_hca CASCADE;
DROP TRIGGER IF EXISTS tr_csm_dimm_history_dump ON csm_dimm CASCADE;
DROP TRIGGER IF EXISTS tr_csm_switch_history_dump ON csm_switch CASCADE;
DROP TRIGGER IF EXISTS tr_csm_ib_cable_history_dump ON csm_ib_cable CASCADE;
DROP TRIGGER IF EXISTS tr_csm_switch_inventory_history_dump ON csm_switch_inventory CASCADE;
DROP TRIGGER IF EXISTS tr_csm_config_history_dump ON csm_config CASCADE;
DROP TRIGGER IF EXISTS tr_csm_lv_update_history_dump ON csm_lv CASCADE;
DROP TRIGGER IF EXISTS tr_csm_diag_result_history_dump ON csm_diag_result CASCADE;
DROP TRIGGER IF EXISTS tr_csm_db_schema_version_history_dump ON csm_db_schema_version CASCADE;

-- CSM API database helper functions
DROP FUNCTION IF EXISTS fn_csm_allocation_node_sharing_status(i_allocation_id bigint,i_type text,i_state text,i_shared boolean,variadic i_nodenames text[]) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_finish_data_stats(allocationid bigint,i_state text,node_names text[],ib_rx_list bigint[],ib_tx_list bigint[],gpfs_read_list bigint[],gpfs_write_list bigint[],energy_list bigint[],pc_hit_list bigint[],gpu_usage_list bigint[],cpu_usage_list bigint[],mem_max_list bigint[], out o_end_time timestamp, out o_final_state text) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_create_data_aggregator(i_allocation_id bigint,i_state text,i_node_names text[],i_ib_rx_list bigint[],i_ib_tx_list bigint[],i_gpfs_read_list bigint[],i_gpfs_write_list bigint[],i_energy bigint[],i_power_cap integer[],i_ps_ratio integer[],i_power_cap_hit bigint[],i_gpu_usage bigint[], out o_timestamp timestamp) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_node_change() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_revert(allocationid bigint) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_step_begin(i_step_id bigint,i_allocation_id bigint,i_status text,i_executable text,i_working_directory text,i_argument text,i_environment_variable text,i_num_nodes integer,i_num_processors integer,i_num_gpus integer,i_projected_memory integer,i_num_tasks integer,i_user_flags text,i_node_names text[], OUT o_begin_time timestamp) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_step_end(IN i_stepid bigint,IN i_allocationid bigint,IN i_exitstatus int,IN i_errormessage text,IN i_cpustats text,IN i_totalutime double precision,IN i_totalstime double precision,IN i_ompthreadlimit text,IN i_gpustats text,IN i_memorystats text,IN i_maxmemory bigint,IN i_iostats text,OUT o_user_flags text,OUT o_num_nodes int,OUT o_nodes text, OUT o_end_time timestamp) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_update() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_state_history_state_change() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_update_state(IN i_allocationid bigint,IN i_state text,OUT o_primary_job_id bigint,OUT o_secondary_job_id integer,OUT o_user_flags text,OUT o_system_flags text,OUT o_num_nodes integer,OUT o_nodes text,OUT o_isolated_cores integer,OUT o_user_name text,OUT o_shared boolean,OUT o_num_gpus integer,OUT o_num_processors integer,OUT o_projected_memory integer,OUT o_state text) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_lv_upsert(l_logical_volume_name text,l_node_name text,l_allocation_id bigint,l_vg_name text,l_state char(1),l_current_size bigint,l_max_size bigint,l_begin_time timestamp,l_updated_time timestamp,l_file_system_mount text,l_file_system_type text) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_node_update() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_node_delete(i_node_names text[]) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_node_state() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_node_attributes_query_details(text) CASCADE;
DROP TYPE IF EXISTS node_details;
DROP FUNCTION IF EXISTS fn_csm_ras_type_update() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_switch_attributes_query_details(text) CASCADE;
DROP TYPE IF EXISTS switch_details;
DROP FUNCTION IF EXISTS fn_csm_vg_create(i_available_size bigint,i_node_name text,i_ssd_count int,i_ssd_serial_numbers text[],i_ssd_allocations bigint[],i_total_size bigint,i_vg_name text,i_is_scheduler boolean) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_vg_delete(i_node_name text,i_vg_name text);
-- CSM database history dump functions
DROP FUNCTION IF EXISTS fn_csm_allocation_history_dump(allocationid bigint,endtime timestamp,exitstatus int,i_state text,finalize boolean,node_names text[],ib_rx_list bigint[],ib_tx_list bigint[],gpfs_read_list bigint[],gpfs_write_list bigint[],energy_list bigint[],pc_hit_list bigint[],gpu_usage_list bigint[],cpu_usage_list bigint[],mem_max_list bigint[],out o_end_time timestamp) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_config_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_dimm_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_gpu_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_hca_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_switch_inventory_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_ib_cable_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_lv_history_dump(text,text,bigint,timestamp,timestamp,bigint,bigint) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_lv_update_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_lv_modified_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_processor_socket_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_ssd_wear() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_ssd_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_step_history_dump(i_stepid bigint,i_allocationid bigint,i_endtime timestamp with time zone,i_exitstatus int,i_errormessage text,i_cpustats text,i_totalutime double precision,i_totalstime double precision,i_ompthreadlimit text,i_gpustats text,i_memorystats text,i_maxmemory bigint,i_iostats text) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_step_node_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_switch_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_vg_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_vg_ssd_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_diag_run_history_dump(_run_id bigint,_end_time timestamp with time zone,_status text,_inserted_ras boolean) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_diag_result_history_dump() CASCADE;
DROP FUNCTION IF EXISTS fn_csm_db_schema_version_history_dump() CASCADE;
-- CSM INVENTORY COLLECTION RELATED FUNCTIONS
DROP FUNCTION IF EXISTS fn_csm_switch_inventory_collection(int,text[],text[],text[],text[],text[],text[],boolean[],text[],text[],text[],int[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],int[],text[],text[]) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_switch_children_inventory_collection(int,text[],text[],text[],text[],text[],text[],int[],int[],int[],text[],text[],text[],text[]) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_ib_cable_inventory_collection(int,text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[],text[]) CASCADE;
DROP FUNCTION IF EXISTS fn_csm_allocation_delete_start(i_allocation_id bigint,i_primary_job_id bigint,i_secondary_job_id integer,i_timeout_time bigint,OUT o_allocation_id bigint,OUT o_primary_job_id bigint,OUT o_secondary_job_id integer,OUT o_user_flags text,OUT o_system_flags text,OUT o_num_nodes integer,OUT o_state text,OUT o_type text,OUT o_isolated_cores integer,OUT o_user_name text,OUT o_nodelist text) CASCADE;
THE_END`    

db_query_3=`psql -q -t -U $csmdb_user -d $dbname << THE_END
\i csm_create_triggers.sql

ALTER TABLE csm_processor_socket DROP CONSTRAINT csm_processor_socket_pkey;
ALTER TABLE csm_processor_socket ADD PRIMARY KEY (node_name, serial_number);

ALTER TABLE csm_dimm DROP CONSTRAINT csm_dimm_pkey;
ALTER TABLE csm_dimm ADD PRIMARY KEY (node_name, serial_number);

COMMENT ON COLUMN csm_allocation_node.energy is 'the total energy used by the node in joules during the allocation';
COMMENT ON COLUMN csm_allocation_node.gpu_usage is 'the total usage aggregated across all GPUs in the node in seconds during the allocation';
COMMENT ON COLUMN csm_allocation_node.gpu_energy is 'the total energy used across all GPUs in the node in joules during the allocation';
COMMENT ON COLUMN csm_allocation_node_history.energy is 'the total energy used by the node in joules during the allocation'; 
COMMENT ON COLUMN csm_allocation_node_history.gpu_usage is 'the total usage aggregated across all GPUs in the node in seconds during the allocation';
COMMENT ON COLUMN csm_allocation_node_history.gpu_energy is 'the total energy used across all GPUs in the node in joules during the allocation';

UPDATE csm_db_schema_version
SET
version = '16.0',
create_time = 'now()'
where 16.0 > 15.0;
THE_END`    

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
else
echo "[Info    ] $dbname is currently running db schema version: $version"
LogMsg "[Info    ] $dbname is currently running db schema version: $version"
echo "-------------------------------------------------------------------------------------------------"
echo "-----------------------------------------------------------------------------------------------------------------------------------" >> $logfile
#fi
exit $return_code
fi
