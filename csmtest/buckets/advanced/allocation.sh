#================================================================================
#   
#    buckets/advanced/allocation.sh
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

# Advanced Bucket for allocation testing
# Allocation create, update, query, delete with cgroups, staging-in
# DEBUG: set -x

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/advanced/allocation.log
TEMP_LOG=${LOG_PATH}/buckets/advanced/allocation_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/advanced/allocation_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "            Starting Advanced Allocation Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Enable IRQ Affinity setting on all computes
xdsh ${COMPUTE_NODES} "sed -i '/irq_affinity/s/false/true/' /etc/ibm/csm/csm_compute.cfg" > ${TEMP_LOG} 2>&1
xdsh ${COMPUTE_NODES} "cat /etc/ibm/csm/csm_compute.cfg | grep irq_affinity" > ${TEMP_LOG} 2>&1
check_all_output "true"
check_return_exit $? 0 "Enable IRQ Affinity setting on all computes"

# Restart and recover compute nodes
xdsh ${COMPUTE_NODES} "systemctl restart csmd-compute" > ${TEMP_LOG} 2>&1
sleep 5
${CSM_PATH}/csm_soft_failure_recovery > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Restart and recover compute nodes"

# Section A - create staging-in, no cgroup
echo "Section A BEGIN" >> ${LOG}
# Test Case 1: Calling csm_allocation_create at staging-in, no core isolation
${CSM_PATH}/csm_allocation_create -j 1 -n ${COMPUTE_NODES} -s "staging-in" 2>&1 > ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1:  (Section A) Calling csm_allocation_create at staging-in, no core isolation"

# Grab & Store Allocation ID from temp log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 2: Validating allocation state staging-in
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-in"
check_return_flag_value $? 0 "Test Case 2:  (Section A) Validating allocation state staging-in"

# Test Case 3: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3:  (Section A) Calling csm_allocation_query_active_all"

# Test Case 4: Calling csm_allocation_update_state -s running on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "running" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4:  (Section A) Calling csm_allocation_update_state -s running on Allocation ID = ${allocation_id}"

# Test Case 4a: SSH to a compute node in the allocation, does get put into the CSM system cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 4a: (Section A) SSH to node in the allocation, does get put into the CSM system cgroup"

# Test Case 4b: SSH to a compute node in the allocation, does it get put into the allocation cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 4b: (Section A) SSH to node in the allocation, does not get put into the allocation cgroup"

# Test Case 5: Validating allocation state running
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "running"
check_return_flag_value $? 0 "Test Case 5:  (Section A) Validating allocation state running"

# Test Case 6: Validating IRQ Balance daemon is active"
xdsh ${COMPUTE_NODES} "ps -ef | grep 'irqbalanc[e]'" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 6:  (Section A) Validating IRQ Balance daemon is active"

# Test Case 7: Calling csm_allocation_query
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7:  (Section A) Calling csm_allocation_query"

# Test Case 8: Calling csm_allocation_update_state -s staging-out on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8:  (Section A) Calling csm_allocation_update_state -s staging-out on Allocation ID = ${allocation_id}"

# Test Case 9: Checking allocation state staging-out
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-out"
check_return_flag_value $? 0 "Test Case 9:  (Section A) Checking allocation state staging-out"

# Test Case 10: Calling csm_allocation_delete on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 10: (Section A) Calling csm_allocation_delete on Allocation ID = ${allocation_id}"

# Test Case 11: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 11: (Section A) Calling csm_allocation_query_active_all"

# Test Case 12: Checking allocation state complete
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "complete"
check_return_flag_value $? 0 "Test Case 12: (Section A) Checking allocation state complete"

echo "Section A COMPLETED!" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Section B - create running with cgroup
echo "Section B BEGIN" >> ${LOG}

# Test Case 1: Calling csm_allocation_create
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} -u ${USER} --isolated_cores 1 --smt_mode 4 2>&1 > ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1:  (Section B) Calling csm_allocation_create"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 1a: SSH to a compute node in the allocation, does not get put into the CSM system cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 1a: (Section B) SSH to node in the allocation, does not get put into the CSM system cgroup"

# Test Case 1b: SSH to a compute node in the allocation, does it get put into the allocation cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1b: (Section B) SSH to node in the allocation, does it get put into the allocation cgroup"

# Test Case 2: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  (Section B) Calling csm_allocation_query_active_all"

# Test Case 3: Validating allocation state running
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "running"
check_return_flag_value $? 0 "Test Case 3:  (Section B) Validating allocation state running"

# Test Case 4: Validating cgroup creation
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 4:  (Section B) Validating cgroup creation"

# Test Case 5: Validating cgroup assigned cpus correctly
xdsh ${SINGLE_COMPUTE} "cat /sys/fs/cgroup/cpuset/allocation_${allocation_id}/cpuset.cpus" > ${TEMP_LOG} 2>&1
check_all_output "4-87,92-175"
check_return_flag_value $? 0 "Test Case 5:  (Section B) Validating cgroup assigned cpus correctly"

# Test Case 6: Validating IRQ Balance daemon is not active"
xdsh ${SINGLE_COMPUTE} "ps -ef | grep 'irqbalanc[e]'" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 6:  (Section B) Validating IRQ Balance daemon is not active"

# Test Case 7: Calling csm_allocation_update_state -s staging-out on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7:  (Section B) Calling csm_allocation_update_state -s staging-out on Allocation ID = ${allocation_id}"

# Test Case 8: Validating allocation state staging-out
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-out"
check_return_flag_value $? 0 "Test Case 8:  (Section B) Validating allocation state staging-out"

# Test Case 9: Calling csm_allocation_delete on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 9:  (Section B) Calling csm_allocation_delete on Allocation ID = ${allocation_id}"

# Test Case 10: Checking allocation state complete
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "complete"
check_return_flag_value $? 0 "Test Case 10: (Section B) Checking allocation state complete"

# Test Case 11: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 11: (Section B) Calling csm_allocation_query_active_all"

# Test Case 12: Checking cgroup deletion
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_all_output "No such file or directory"
check_return_flag_value $? 0 "Test Case 12: (Section B) Checking cgroup deletion"

echo "Section B COMPLETED!" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Section C - create staging-in, with cgroup
echo "Section C BEGIN" >> ${LOG}

# Test Case 1: Calling csm_allocation_create at staging-in, with cgroup
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} -u ${USER} -s "staging-in" --isolated_cores 1 --smt_mode 1 2>&1 > ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1:  (Section C) Calling csm_allocation_create at staging-in, with cgroup"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 2: Checking allocation state staging-in
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-in"
check_return_flag_value $? 0 "Test Case 2:  (Section C) Checking allocation state staging-in"

# Test Case 3: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3:  (Section C) Calling csm_allocation_query_active_all"

# Test Case 4: Validating cgroup not created yet
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_all_output "No such file or directory"
check_return_flag_value $? 0 "Test Case 4:  (Section C) Validating cgroup not created yet"

# Test Case 5: Calling csm_allocation_update_state -s running on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s running > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5:  (Section C) Calling csm_allocation_update_state -s running on Allocation ID = ${allocation_id}"

# Test Case 6: Validating allocation state running
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "running"
check_return_flag_value $? 0 "Test Case 6:  (Section C) Validating allocation state running"

# Test Case 6a: SSH to a compute node in the allocation, does not get put into the CSM system cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 6a: (Section C) SSH to node in the allocation, does not get put into the CSM system cgroup"

# Test Case 6b: SSH to a compute node in the allocation, does it get put into the allocation cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 6b: (Section C) SSH to node in the allocation, does it get put into the allocation cgroup"

# Test Case 7: Validating cgroup creation
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 7:  (Section C) Validating cgroup creation"

# Test Case 8: Validating cgroup assigned cpus correctly
xdsh ${SINGLE_COMPUTE} "cat /sys/fs/cgroup/cpuset/allocation_${allocation_id}/cpuset.cpus" > ${TEMP_LOG} 2>&1
check_all_output "84,92"
check_return_flag_value $? 0 "Test Case 8:  (Section C) Validating cgroup assigned cpus correctly"

# Test Case 9: Calling csm_allocation_update_state -s staging-out on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 9:  (Section C) Calling csm_allocation_update_state -s staging-out on Allocation ID = ${allocation_id}"

# Test Case 10: Validating allocation state staging-out
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-out"
check_return_flag_value $? 0 "Test Case 10: (Section C) Validating allocation state staging-out"

# Test Case 11: Calling csm_allocation_delete on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 11: (Section C) Calling csm_allocation_delete on Allocation ID = ${allocation_id}"

# Test Case 12: Checking cgroup deletion
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_all_output "No such file or directory"
check_return_flag_value $? 0 "Test Case 12: (Section C) Checking cgroup deletion"

# Test Case 13: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 13: (Section C) Calling csm_allocation_query_active_all"

# Test Case 14: Validating allocation state complete
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "complete"
check_return_flag_value $? 0 "Test Case 14: (Section C) Validating allocation state complete"

echo "Section C COMPLETED!" >> ${LOG}

echo "------------------------------------------------------------" >> ${LOG}
# Section D - Permissions checks for non-root users
echo "Section D BEGIN" >> ${LOG}

# Test Case 1: Calling csm_allocation_create for user ${USER} at staging-in
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} -s "staging-in" -u ${USER} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1:  (Section D) Calling csm_allocation_create for user ${USER} at staging-in"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 2: Checking Allocation ${allocation_id} created for user ${USER}
check_all_output "${USER}"
check_return_exit $? 0 "Test Case 2:  (Section D) Checking Allocation ${allocation_id} created for user ${USER}"

# Test Case 3: Attempt csm_allocation_update_state as secondary non-root user (error expected)
su -c "${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s running" ${SECOND_USER} > ${TEMP_LOG} 2>&1
check_return_exit $? 16 "Test Case 3:  (Section D) Attempt csm_allocation_update_state as secondary non-root user (error expected)"

# Test Case 3: Attempt csm_allocation_update_state as owner
su -c "${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s running" ${USER} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3:  (Section D) Attempt csm_allocation_update_state as owner"

# Test Case 3a: SSH to a compute node in the allocation, does not get put into the CSM system cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 3a: (Section D) SSH to node in the allocation, does not get put into the CSM system cgroup"

# Test Case 3b: SSH to a compute node in the allocation, does get put into the allocation cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 3b: (Section D) SSH to node in the allocation, does get put into the allocation cgroup"

# Test Case 3: Validate state transitions in csm_allocation_query_details
${CSM_PATH}/csm_allocation_query_details -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-in" "to-running" "running" "${USER}"
check_return_flag_value $? 0 "Test Case 3:  (Section D) Validate state transitions in csm_allocation_query_details"

# Test Case 4: Attempt csm_allocation_update_state as secondary non-root user (error expected)
su -c "${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s staging-out" ${SECOND_USER} > ${TEMP_LOG} 2>&1
check_return_exit $? 16 "Test Case 4:  (Section D) Attempt csm_allocation_update_state as secondary non-root user (error expected)"

# Test Case 4: Attempt csm_allocation_update_state as owner
su -c "${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s staging-out" ${USER} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4:  (Section D) Attempt csm_allocation_update_state as owner"

# Test Case 4: Validate state transitions in csm_allocation_query_details
${CSM_PATH}/csm_allocation_query_details -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-in" "to-running" "running" "to-staging-out" "staging-out" "${USER}"
check_return_flag_value $? 0 "Test Case 4:  (Section D) Validate state transitions in csm_allocation_query_details"

# Test Case 5: Calling csm_allocation_delete as user ${USER}
su -c "${CSM_PATH}/csm_allocation_delete -a ${allocation_id}" ${USER} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5:  (Section D) Calling csm_allocation_delete as user ${USER}"

# Test Case 5: Checking allocation deleted
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 5:  (Section D) Checking allocation deleted"

echo "Section D COMPLETED!" >> ${LOG}

echo "------------------------------------------------------------" >> ${LOG}
# Section E - Creating an allocation on a node with an active staging-out allocation
echo "Section E BEGIN" >> ${LOG}

# Test Case 1: Create first allocation
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1:  (Section E) Create first allocation"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 1a: SSH to a compute node in the allocation, does not get put into the CSM system cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1a: (Section E) SSH to node in the allocation, does get put into the CSM system cgroup"

# Test Case 1b: SSH to a compute node in the allocation, does not get put into the allocation cgroup?
xdsh ${SINGLE_COMPUTE} "cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 1b: (Section E) SSH to node in the allocation, does not get put into the allocation cgroup"

# Test Case 2: Update first allocation to staging-out
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  (Section E) Update first allocation to staging-out"

# Test Case 3: Create second allocation at staging-in
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} -s "staging-in" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3:  (Section E) Create second allocation at staging-in"

# Grab & Store Allocation ID from csm_allocation_create.log
second_allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 3: Validate 2 allocations active and in correct state with query
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_all_output "num_allocations: 2" "allocation_id: ${allocation_id}" "allocation_id: ${second_allocation_id}" "staging-in" "staging-out"
check_return_flag_value $? 0 "Test Case 3:  (Section E) Validate 2 allocations active and in correct state with query"

# Test Case 4: Update second allocation to running
${CSM_PATH}/csm_allocation_update_state -a ${second_allocation_id} -s "running" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4:  (Section E) Update second allocation to running"

# Test Case 4: Validate 2 allocations active and in correct state with query
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_all_output "num_allocations: 2" "allocation_id: ${allocation_id}" "allocation_id: ${second_allocation_id}" "running" "staging-out"
check_return_flag_value $? 0 "Test Case 4:  (Section E) Validate 2 allocations active and in correct state with query"

# Test Case 5: Update second allocation to staging-out
${CSM_PATH}/csm_allocation_update_state -a ${second_allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5:  (Section E) Update second allocation to staging-out"

# Test Case 6: Create a third allocation at running
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6:  (Section E) Create a third allocation at running"

# Grab & Store Allocation ID from csm_allocation_create.log
third_allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 6: Validate 3 allocations active and in correct state with query
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_all_output "num_allocations: 3" "allocation_id: ${allocation_id}" "allocation_id: ${second_allocation_id}" "allocation_id: ${third_allocation_id}" "running" "staging-out"
check_return_flag_value $? 0 "Test Case 6:  (Section E) Validate 3 allocations active and in correct state with query" 

# Test Case 7: Update third allocation to staging-out
${CSM_PATH}/csm_allocation_update_state -a ${third_allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7:  (Section E) Update third allocation to staging-out"

# Clean up allocations
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "              (Section E) Clean up Allocation ${allocation_id}"

${CSM_PATH}/csm_allocation_delete -a ${second_allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "              (Section E) Clean up Allocation ${second_allocation_id}"

${CSM_PATH}/csm_allocation_delete -a ${third_allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "              (Section E) Clean up Allocation ${third_allocation_id}"

${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_flag_value $? 4 "              (Section E) All allocations cleaned up"

echo "Section E COMPLETED!" >> ${LOG}

echo "------------------------------------------------------------" >> ${LOG}
# Section F - Create an allocation with SMT > 4 to test clamping mechanism
echo "Section F BEGIN" >> ${LOG}

# Test Case 1: Calling csm_allocation_create with 1 isolated core and smt_mode 8"
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} -u ${USER} --isolated_cores 1 --smt_mode 8 2>&1 > ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1:  (Section F) Calling csm_allocation_create with 1 isolated core and smt_mode 8"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 1a: SSH to a compute node in the allocation, does not get put into the CSM system cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 1a: (Section F) SSH to node in the allocation, does not get put into the CSM system cgroup"

# Test Case 1b: SSH to a compute node in the allocation, does it get put into the allocation cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1b: (Section F) SSH to node in the allocation, does it get put into the allocation cgroup"

# Test Case 2: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  (Section F) Calling csm_allocation_query_active_all"

# Test Case 3: Validating allocation state running
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "running"
check_return_flag_value $? 0 "Test Case 3:  (Section F) Validating allocation state running"

# Test Case 4: Validating cgroup creation
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 4:  (Section F) Validating cgroup creation"

# Test Case 5: Validating cgroup assigned cpus correctly
xdsh ${SINGLE_COMPUTE} "cat /sys/fs/cgroup/cpuset/allocation_${allocation_id}/cpuset.cpus" > ${TEMP_LOG} 2>&1
check_all_output "4-87,92-175"
check_return_flag_value $? 0 "Test Case 5:  (Section F) Validating cgroup assigned cpus correctly"

# Test Case 6: Validating IRQ Balance daemon is not active"
xdsh ${SINGLE_COMPUTE} "ps -ef | grep 'irqbalanc[e]'" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 6:  (Section F) Validating IRQ Balance daemon is not active"

# Test Case 7: Calling csm_allocation_delete on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7:  (Section F) Calling csm_allocation_delete on Allocation ID = ${allocation_id}"

# Test Case 8: Checking allocation state complete
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "complete"
check_return_flag_value $? 0 "Test Case 8:  (Section F) Checking allocation state complete"

# Test Case 9: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 9:  (Section F) Calling csm_allocation_query_active_all"

# Test Case 10: Checking cgroup deletion
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_all_output "No such file or directory"
check_return_flag_value $? 0 "Test Case 10: (Section F) Checking cgroup deletion"

echo "Section F COMPLETED!" >> ${LOG}

echo "------------------------------------------------------------" >> ${LOG}
# Section G - Create an allocation with no isolated cores and smt_mode 1
echo "Section G BEGIN" >> ${LOG}

# Test Case 1: Calling csm_allocation_create with no isolated cores and smt_mode 1"
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} -u ${USER} --isolated_cores 0 --smt_mode 1 2>&1 > ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1:  (Section G) Calling csm_allocation_create with no isolated cores and smt_mode 1"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 1a: SSH to a compute node in the allocation, does not get put into the CSM system cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep csm_system" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 1 "Test Case 1a: (Section G) SSH to node in the allocation, does not get put into the CSM system cgroup"

# Test Case 1b: SSH to a compute node in the allocation, does it get put into the allocation cgroup?
su - ${USER} -c "ssh ${SINGLE_COMPUTE} cat /proc/self/cgroup | egrep allocation" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1b: (Section G) SSH to node in the allocation, does it get put into the allocation cgroup"

# Test Case 2: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  (Section G) Calling csm_allocation_query_active_all"

# Test Case 3: Validating allocation state running
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "running"
check_return_flag_value $? 0 "Test Case 3:  (Section G) Validating allocation state running"

# Test Case 4: Validating cgroup creation
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 4:  (Section G) Validating cgroup creation"

# Test Case 5: Validating cgroup assigned cpus correctly
xdsh ${SINGLE_COMPUTE} "cat /sys/fs/cgroup/cpuset/allocation_${allocation_id}/cpuset.cpus" > ${TEMP_LOG} 2>&1
check_all_output "0,4,8"
check_return_flag_value $? 0 "Test Case 5:  (Section G) Validating cgroup assigned cpus correctly"

# Test Case 6: Validating IRQ Balance daemon is active"
xdsh ${SINGLE_COMPUTE} "ps -ef | grep 'irqbalanc[e]'" > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 6:  (Section G) Validating IRQ Balance daemon is active"

# Test Case 7: Calling csm_allocation_delete on Allocation ID = ${allocation_id}
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7:  (Section G) Calling csm_allocation_delete on Allocation ID = ${allocation_id}"

# Test Case 8: Checking allocation state complete
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "complete"
check_return_flag_value $? 0 "Test Case 8:  (Section G) Checking allocation state complete"

# Test Case 9: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 9:  (Section G) Calling csm_allocation_query_active_all"

# Test Case 10: Checking cgroup deletion
xdsh ${SINGLE_COMPUTE} "ls /sys/fs/cgroup/cpuset/allocation_${allocation_id}" > ${TEMP_LOG} 2>&1
check_all_output "No such file or directory"
check_return_flag_value $? 0 "Test Case 10: (Section G) Checking cgroup deletion"

echo "Section G COMPLETED!" >> ${LOG}
rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "         Advanced Allocation Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
