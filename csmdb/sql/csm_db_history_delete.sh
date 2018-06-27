#!/bin/bash
#================================================================================
#   
#    csm_db_history_delete.sh
# 
#    © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================


#-----------------
# Default Flags
#-----------------
dir="no"
time="no"
dbname="no"

usage()
{
cat << EOF
----------------------------------------------------------------------------------------
CSM Database History Delete Usage -
========================================================================================
   -h                       Display this message.
   -t <dir>                 Target directory to write the delete log to. 
                            Default: "/var/log/ibm/csm/delete"
   -n <time-count mins.>    The time (in mins.) of oldest records which to delete.
                            Attention: requires users input value
   -d <db>                  Database to delete tables from.
                            Attention: requires users input value
========================================================================================
EOF
}

#---------------------------------------------
# Change to the directory housing the script.
#---------------------------------------------

cd $( dirname "${BASH_SOURCE[0]}" )

#--------------------------
# The optstring for input.
#--------------------------

optstring="ht:n:d:"

TARGET_DIRECTORY="/var/log/ibm/csm/delete"  # The target directory to write the log to.
#NUM_ENTRIES=100                             # The time (in mins.) of oldest records which to delete.
#DATABASE="csmdb"                            # The database to search for history tables to delete.

#----------------------------------------------------------------------------------------------------
# A list of tables for processing.
#----------------------------------------------------------------------------------------------------

TABLES=( csm_allocation_history csm_allocation_node_history csm_allocation_state_history \
    csm_config_history csm_db_schema_version_history csm_diag_result_history csm_diag_run_history \
    csm_dimm_history csm_gpu_history csm_hca_history csm_ib_cable_history csm_lv_history \
    csm_lv_update_history csm_node_history csm_node_state_history csm_processor_socket_history \
    csm_ssd_history csm_ssd_wear_history csm_step_history csm_step_node_history csm_switch_history\
    csm_switch_inventory_history csm_vg_history csm_vg_ssd_history)

#-------------------------------
# The list of RAS tables.
#-------------------------------

RAS_TABLES=(csm_ras_event_action)

while getopts $optstring OPTION
do
    case $OPTION in
        h)
            usage; exit 1;;
        t)
            dir="yes"
            [[ -z "${OPTARG}" || "${OPTARG}" == -* ]] \
                && {
                echo -e "----------------------------------------------------------------------------------------";
                echo -e "[ERROR ] Invalid or no speficied dir value";
                usage
                exit 1; }
            TARGET_DIRECTORY=${OPTARG};;
        n)
            time="yes"
            [[ -z "${OPTARG}" || "${OPTARG}" == -* ]] \
                && {
                echo -e "----------------------------------------------------------------------------------------";
                echo -e "[ERROR ] Invalid or no speficied time interval value";
                usage
                exit 1; }
                NUM_ENTRIES=${OPTARG};;
        d)
            dbname="yes"
            [[ -z "${OPTARG}" || "${OPTARG}" == -* ]] \
                && {
                echo -e "----------------------------------------------------------------------------------------";
                echo -e "[ERROR ] No database speficied";
                usage
                exit 1; }
                DATABASE=${OPTARG};;
    esac
done 2>>/dev/null

#---------------------------------------------------------------------------------
# Checks to see if input arguments are conflicting or are present
#---------------------------------------------------------------------------------

if [[ ($dir == "no") || ($time == "no") || ($dbname == "no") ]]; then
        echo "----------------------------------------------------------------------------------------";
        echo "[Error   ] missing arguments"
        echo "[Example ] ./csm_db_history_delete.sh -d [dbname] -n [time_interval] -t [/data_dir/]"
        #echo "----------------------------------------------------------------------------------------";
        usage
        echo "----------------------------------------------------------------------------------------";
        exit 0
fi

#-------------------------------
# Iterate over History tables.
#-------------------------------

for table in "${TABLES[@]}"
do
    ./csm_history_wrapper_delete_script_template.sh ${DATABASE} ${NUM_ENTRIES} ${table} ${TARGET_DIRECTORY} \
        > /dev/null 2>&1
done

#-------------------------------
# Iterate over RAS tables.
#-------------------------------

for table in "${RAS_TABLES[@]}"
do
    ./csm_ras_event_action_wrapper_delete_script.sh ${DATABASE} ${NUM_ENTRIES} ${table} ${TARGET_DIRECTORY} \
        > /dev/null 2>&1

done
