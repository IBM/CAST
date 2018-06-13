#================================================================================
#   
#    buckets/basic/compute_node.sh
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

# Basic Bucket for remote API calls from compute node
# Step (begin, end, query active all, query) Allocation (query active all, query, query details)

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/compute_node.log
TEMP_LOG=${LOG_PATH}/buckets/basic/compute_node_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/compute_node_flags.log


if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "          Starting Basic Compute Node Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Step Section - create allocation on master. Step begin, query active all, step end, query from compute. allocation delete on master
echo "Step Section BEGIN:" >> ${LOG}
# Test Case 1: Calling csm_allocation_create from master
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Calling csm_allocation_create from master"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 2: csm_allocation_step_begin from compute node
xdsh ${SINGLE_COMPUTE} "${CSM_PATH}/csm_allocation_step_begin -a ${allocation_id} -r test -c ${SINGLE_COMPUTE} -e test -x test -g 1 -m 512 -p 1 -t 1 -S 1 -w test" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: csm_allocation_step_begin from compute node"

# Test Case 3: csm_allocation_step_query_active_all from compute node
xdsh ${SINGLE_COMPUTE} "${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id}" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: csm_allocation_step_begin from compute node"

# Test Case 4: csm_allocation_step_end from compute node
xdsh ${SINGLE_COMPUTE} "${CSM_PATH}/csm_allocation_step_end -a ${allocation_id} -c test -e 1 -E test -G test -i test -m test -M 512 -n test -s 1 -t 0.0 -T 1.5" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4: csm_allocation_step_end from compute node"

# Test Case 4: Validating step ended with csm_allocation_step_query_active_all output
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "Total_Records: 0" "# No matching records found."
check_return_flag $? "Test Case 4: Validating step ended with csm_allocation_step_query_active_all output..."

# Test Case 5: csm_allocation_step_query from compute node
xdsh ${SINGLE_COMPUTE} "${CSM_PATH}/csm_allocation_step_query -a ${allocation_id} -s 1" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5: csm_allocation_step_query from compute node"

# Test Case 6: csm_allocation_query_active_all from compute node
xdsh ${SINGLE_COMPUTE} "${CSM_PATH}/csm_allocation_query_active_all" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6: csm_allocation_query_active_all from compute node"

# Test Case 7: csm_allocation_query from compute node
xdsh ${SINGLE_COMPUTE} "${CSM_PATH}/csm_allocation_query -a ${allocation_id}" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7: csm_allocation_query from compute node"

# Clean up allocation
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Clean up allocation"
rm -f ${TEMP_LOG}

# Test Case 8: csm_allocation_query_details from compute node
xdsh ${SINGLE_COMPUTE} "${CSM_PATH}/csm_allocation_query_details -a ${allocation_id}" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8: csm_allocation_query_details from compute node"

echo "------------------------------------------------------------" >> ${LOG}
echo "           Basic Compute Node Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

rm -f ${TEMP_LOG}

exit 0
