#! /bin/bash
#================================================================================
#
#    buckets/advanced/allocation_metrics_test.sh
#
#  © Copyright IBM Corporation 2020. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

#--------------------------------------------------------------------------------
# Advanced Bucket for allocation metrics testing
# Allocation create, run some environmental tests related to CPUs, GPUs, etc.
# DEBUG: set -x
#--------------------------------------------------------------------------------

#----------------------------------
# Check for the csm_test.cfg file
#----------------------------------
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

#---------------------
# Log file variables
#---------------------
LOG=${LOG_PATH}/buckets/advanced/allocation_metrics.log
TEMP_LOG=${LOG_PATH}/buckets/advanced/allocation_metrics_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/advanced/allocation_metrics_flags.log

#----------------------------------
# Check for the function.sh file
#----------------------------------
if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "       Starting Advanced Allocation Metrics Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

echo "------------------------------------------------------------"
echo "Starting Advanced Allocation Metrics Bucket"
echo "------------------------------------------------------------"

#----------------------------------
# Single compute node for testing
#----------------------------------
NODES="${SINGLE_COMPUTE}"

#------------------------------------------
# User specified in the csm-test.cfg file
#------------------------------------------
USERID="${USER}"

#----------------------------------------------------------------
# This value can be manually set to the desired isolated cores.
#----------------------------------------------------------------
ISOLATED_CORES=2

#------------------------------------
# Verbose can be enabled if desired
#------------------------------------
#VERBOSE=TRUE

#----------------------------------------------------
# Check to see if the compute nodes are in service
#----------------------------------------------------
#Test Case 0 Setup Compute Nodes to "IN_SERVICE":
SET_NODES=$(${FVT_PATH}/tools/update_computes_in_service.sh) > ${TEMP_LOG} 2>&1
if [ $? -eq 1 ]; then
    echo "------------------------------------------------------------"
    echo "FAILED Advanced Allocation Metrics Bucket"
    echo "------------------------------------------------------------"
    check_return_exit $? 1 "Test Case 0: Set Compute Nodes to (IN_SERVICE): Calling update_computes_in_service"
    exit 1
else
    check_return_exit $? 1 "Test Case 0: Set Compute Nodes to (IN_SERVICE): Calling update_computes_in_service"
fi

#----------------------------------------------------------
# Gathering the allocation info. based on user and node(s)
#----------------------------------------------------------
echo "Creating an allocation"
ALLOCATION_ID=$(/opt/ibm/csm/bin/csm_allocation_create -j 1023 -u "$USERID" -n "$NODES" --isolated_cores "$ISOLATED_CORES" | grep "allocation_id:" | awk {'print $2'})
echo "$ALLOCATION_ID" > ${TEMP_LOG} 2>&1

rm -f ${TEMP_LOG}
if [[ "$ALLOCATION_ID" -lt 0 ]] || [[ -z "$ALLOCATION_ID" ]]; then
    check_return_flag_value $? 1 "Test Case 1: Calling csm_allocation_create with $ISOLATED_CORES isolated core(s)"
    echo "------------------------------------------------------------"
    echo "FAILED Advanced Allocation Metrics Bucket"
    echo "------------------------------------------------------------"
    exit 1
else
    check_return_flag_value $? 1 "Test Case 1: Calling csm_allocation_create with $ISOLATED_CORES isolated core(s)"
fi

#----------------------------------------------
# Put some of the echo commands into the Log etc.
#----------------------------------------------
echo "ALLOCATION_ID = $ALLOCATION_ID"

#----------------------------------------------
# Another debug line if VERBOSE is uncommented
#----------------------------------------------
if [ -n "$VERBOSE" ] ; then
  /bin/psql -U postgres csmdb --pset footer -c "select node_name,energy,power_cap_hit,gpu_usage,gpu_energy,cpu_usage,memory_usage_max from csm_allocation_node where allocation_id = $ALLOCATION_ID"
fi

#----------------------------------------------
# Output for running test case
#----------------------------------------------
echo "Running tests, please wait..."

#----------------------------------------------
# Change this line to single compute only
#----------------------------------------------
IFS=', ' read -r -a NODE_ARRAY <<< "$NODES"

#echo "${NODE_ARRAY[0]}"

#--------------------------------------------------------
# We can set up multiple nodes in the future if needed
#--------------------------------------------------------
for NODE in "${NODE_ARRAY[@]}" ; do
   # Do some GPU work on the node: 
   su - $USERID -c "ssh ${NODE} /usr/bin/dcgmi diag -r \'diagnostic\' --parameters \'diagnostic.test_duration=60.0\' \> /dev/null" &
   
   # Do some CPU work on the node:
   su - $USERID -c "ssh ${NODE} '/usr/bin/seq 176 | /usr/bin/xargs -P0 -n1 timeout 5 yes > /dev/null'" &
   
   # Don't do any work on the node:
   #su - $USERID -c "ssh ${NODE} sleep 5" &
done
wait

#----------------------------------------------
# This section is for further debugging
#----------------------------------------------
# echo "Press any key to continue"
# read input 
#----------------------------------------------
#xdsh $NODES "ps u -p \`pgrep nv-hostengine\` | grep root" | sort

/opt/ibm/csm/bin/csm_allocation_delete -a $ALLOCATION_ID | grep "Allocation" | cut -c 3-

#----------------------------------------------
# This section is for debugging
#----------------------------------------------
if [ -n "$VERBOSE" ] ; then
  /bin/psql -U postgres csmdb --pset footer -c "select node_name,energy,power_cap_hit,gpu_usage,gpu_energy,cpu_usage,memory_usage_max from csm_allocation_node_history where allocation_id = $ALLOCATION_ID AND csm_allocation_node_history.state = 'complete'"
fi

#----------------------------------------------
# Gather the information from the allocation
#----------------------------------------------
aqd=`/opt/ibm/csm/bin/csm_allocation_query_details -a $ALLOCATION_ID | egrep "compute_nodes:" -A 100 | egrep "num_transitions:" -B 100 | egrep -v "ib_rx:|ib_tx:|power_cap:|power_shifting_ratio:|num_transitions:"`

#echo "allocation_query_details:$aqd"

#----------------------------------------------
# Modify the string for the array
#----------------------------------------------
aqd_checks=`echo "$aqd" | awk -vORS=, '{ print $2 }' | sed 's/,$/\n/' | cut -c 3-`
#echo "$aqd_checks"

#---------------------------------------------------
# build the array from the allocation string above
#---------------------------------------------------
IFS=', ' read -r -a array <<< "$aqd_checks" > ${TEMP_LOG} 2>&1

#----------------------------------------------------------------------------------
# Check everything within the array to make sure the expected values are returned.
#----------------------------------------------------------------------------------
# Check gpfs_read
#----------------------------------------------------------------------------------
if [ "${array[0]}" -lt 0 ] || [ -z "${array[0]}" ]; then
    echo "${array[0]}" > ${TEMP_LOG} 2>&1
    gpfs_read=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 2: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpfs_read: ${array[0]}"
else
    # If the gpfs_read size is greater than 0 then it should gather the size.
    echo "${array[0]}" > ${TEMP_LOG} 2>&1
    gpfs_read=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 2: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpfs_read: ${array[0]}"
fi

#---------------------------------
# Check gpfs_write
#---------------------------------
if [ "${array[1]}" -lt 0 ] || [ -z "${array[1]}" ]; then
    echo "${array[1]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 3: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpfs_write: ${array[1]}"
else
    # If the gpfs_write size is greater than 0 then it should gather the size.
    echo "${array[1]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 3: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpfs_write: ${array[1]}"
fi

#---------------------------------
# Check energy_consumed
#---------------------------------
if [ "${array[2]}" -lt 0 ] || [ -z "${array[2]}" ]; then
    echo "${array[2]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 4: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check energy_consumed: ${array[2]}"
else
    # If the energy_consumed size is greater than 0 then it should gather the size.
    echo "${array[2]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 4: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check energy_consumed: ${array[2]}"
fi

#---------------------------------
# Check power_cap_hit
#---------------------------------
if [ "${array[3]}" -lt 0 ] || [ -z "${array[3]}" ]; then
    echo "${array[3]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 5: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check power_cap_hit: ${array[3]}"
else
    # If the power_cap_hit: size is greater than 0 then it should gather the size.
    echo "${array[3]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 5: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check power_cap_hit: ${array[3]}"
fi

#---------------------------------
# Check gpu_energy
#---------------------------------
if [ "${array[4]}" -lt 0 ] || [ -z "${array[4]}" ]; then
    echo "${array[4]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 6: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpu_energy: ${array[4]}"
else
    # If the gpu_energy: size is greater than 0 then it should gather the size.
    echo "${array[4]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 6: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpu_energy: ${array[4]}"
fi

#---------------------------------
# Check gpu_usage
#---------------------------------
if [ "${array[5]}" -lt 0 ] || [ -z "${array[5]}" ]; then
    echo "${array[5]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 7: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpu_usage: ${array[5]}"
else
    # If the gpu_usage: size is greater than 0 then it should gather the size.
    echo "${array[5]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 7: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check gpu_usage: ${array[5]}"
fi

#---------------------------------
# Check cpu_usage
#---------------------------------
if [ "${array[6]}" -lt 0 ] || [ -z "${array[6]}" ]; then
    echo "${array[6]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 8: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check cpu_usage: ${array[6]}"
else
    # If the cpu_usage: size is greater than 0 then it should gather the size.
    echo "${array[6]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 8: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check cpu_usage: ${array[6]}"
fi

#---------------------------------
# Check memory_usage_max
#---------------------------------
if [ "${array[7]}" -lt 0 ] || [ -z "${array[7]}" ]; then
    echo "${array[7]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 1 "Test Case 9: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check memory_usage_max: ${array[7]}"
else
    # If the memory_usage_max: size is greater than 0 then it should gather the size.
    echo "${array[7]}" > ${TEMP_LOG} 2>&1
    gpfs_write=`cat ${TEMP_LOG}`
    check_return_flag_value $? 0 "Test Case 9: csm_allocation_query_details on ${SINGLE_COMPUTE} allocation_id: $ALLOCATION_ID check memory_usage_max: ${array[7]}"
fi

echo "------------------------------------------------------------"
echo "Completed Advanced Allocation Metrics Bucket"
echo "------------------------------------------------------------"
rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "        Advanced Allocation Metrics Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
