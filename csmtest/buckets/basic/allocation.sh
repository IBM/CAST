#================================================================================
#   
#    buckets/basic/node.sh
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

# Bucket for allocation testing
# Allocation query, update, delete

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/allocation.log
TEMP_LOG=${LOG_PATH}/buckets/basic/allocation_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/allocation_flag.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting allocation Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: csm_allocation_query_active_all (failure expected)
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 1:   Calling csm_allocation_query_active_all (failure expected)"

rm -f ${TEMP_LOG}
# Test Case 2: csm_allocation_create
${CSM_PATH}/csm_allocation_create -j 1 -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:   Calling csm_allocation_create"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 1a: SSH to a compute node in the allocation, does get put into the CSM system cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1a:  SSH to node in the allocation, does get put into the CSM system cgroup"

# Test Case 1b: SSH to a compute node in the allocation, does it get put into the allocation cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 1b:  SSH to node in the allocation, does not get put into the allocation cgroup"

rm -f ${TEMP_LOG}

# Test Case 2.1: Validating cgroup assigned allocation cpus correctly
xdsh ${SINGLE_COMPUTE} "cat /sys/fs/cgroup/cpuset/allocation_${allocation_id}/cpuset.cpus" > ${TEMP_LOG} 2>&1
check_all_output "0-175"
check_return_flag_value $? 0 "Test Case 2.1: Validating cgroup assigned allocation cpus correctly"

# Test Case 2.2: Validating cgroup assigned system cpus correctly
xdsh ${SINGLE_COMPUTE} "cat /sys/fs/cgroup/cpuset/csm_system/cpuset.cpus" > ${TEMP_LOG} 2>&1
check_all_output "0-175"
check_return_flag_value $? 0 "Test Case 2.2: Validating cgroup assigned system cpus correctly"

# Test Case 3: csm_allocation_query_active_all (success)
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3:   csm_allocation_query_active_all success"

rm -f ${TEMP_LOG}
# Test Case 4: csm_allocation_update_state staging out
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4:   Calling csm_allocation_update_state -s staging-out on Allocation ID = ${allocation_id}"

rm -f ${TEMP_LOG}
# Test Case 5: csm_allocation_query
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5:   Calling csm_allocation_query"

rm -f ${TEMP_LOG}
# Test Case 6: csm_allocation_resources_query
${CSM_PATH}/csm_allocation_resources_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6:   Calling csm_allocation_resources_query"

rm -f ${TEMP_LOG}
# Test Case 7: csm_allocation_delete
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7:   Calling csm_allocation_delete on Allocation ID = ${allocation_id}"

rm -f ${TEMP_LOG}
# Test Case 8: csm_allocation_query_active_all (failure after delete)
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 8:   Calling csm_allocation_query_active_all (failure after delete)"

rm -f ${TEMP_LOG}
# Test Case 9: csm_allocation_query_details
${CSM_PATH}/csm_allocation_query_details -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 9:   Calling csm_allocation_query_details on Allocation ID = ${allocation_id}"

# Test Case 10: check csm_allocation_query_details for state=complete
check_all_output "complete"
check_return_flag_value $? 0 "Test Case 10:  Checking csm_allocation_query_details for state=complete"

rm -f ${TEMP_LOG}
# Test Case 11: csm_allocation_update_history
${CSM_PATH}/csm_allocation_update_history -a ${allocation_id} -c "test_comment" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 11:  Calling csm_allocation_update_history on Allocation ID = ${allocation_id}"

rm -f ${TEMP_LOG}
# Test Case 12: check csm_allocation_query return and output for test_comment
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 12:  Calling csm_allocation_query on Allocation ID = ${allocation_id}"
check_all_output "test_comment"
check_return_flag_value $? 0 "Test Case 12:  Checking allocation history table updated with test_comment..."

# Test Case 13: csm_allocation_create with launch node input
# Get utility node name
utility_node=`nodels utility | head -1`
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} -l ${utility_node} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 13:  csm_allocation_create with launch node input"

# Test Case 13: Validate launch node in csm_allocation_query
# Get allocation ID
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "launch_node_name:               ${utility_node}"
check_return_flag_value $? 0 "Test Case 13:  Validate launch node in csm_allocation_query"

# Test Case 1a: SSH to a compute node in the allocation, does get put into the CSM system cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1a:  SSH to node in the allocation, does get put into the CSM system cgroup"

# Test Case 1b: SSH to a compute node in the allocation, does it get put into the allocation cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 1b:  SSH to node in the allocation, does not get put into the allocation cgroup"

# Clean Up allocation
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "               Clean up allocation"
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "               Validating no active allocations"

echo "------------------------------------------------------------" >> ${LOG}
echo "           Basic Allocation Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

rm -f ${TEMP_LOG}

exit 0
