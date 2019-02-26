#================================================================================
#
#    buckets/basic/csm_soft_failure_recovery.sh
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

# Bucket for basic testing of csm_soft_failure_recovery CSM API
# Run through success paths of csm_soft_failure_recovery

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

# Local Variables
LOG=${LOG_PATH}/buckets/basic/soft_failure_recovery.log
TEMP_LOG=${LOG_PATH}/buckets/basic/soft_failure_recovery_tmp.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "           Starting Soft Failure Recovery Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Recover single node - Restart Daemons
${FVT_PATH}/tools/dual_aggregator/shutdown_daemons.sh > ${TEMP_LOG} 2>&1
${FVT_PATH}/tools/dual_aggregator/start_daemons.sh > ${TEMP_LOG} 2>&1
sleep 5

# Test Case 1: Recover single node - Put ${SINGLE_COMPUTE} in Soft Failure
xdsh ${SINGLE_COMPUTE} "systemctl restart csmd-compute" > ${TEMP_LOG} 2>&1
sleep 5

# Test Case 1: Recover single node - Verify ${SINGLE_COMPUTE} in Soft Failure state 
${CSM_PATH}/csm_node_attributes_query -n ${SINGLE_COMPUTE} -s SOFT_FAILURE > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Recover single node - Verify ${SINGLE_COMPUTE} in Soft Failure state"

# Test Case 1: Recover single node - Run API
${CSM_PATH}/csm_soft_failure_recovery > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Recover single node - Run API"

# Test Case 1: Recover single node - Verify ${SINGLE_COMPUTE} recovered
${CSM_PATH}/csm_node_attributes_query -n ${SINGLE_COMPUTE} -s IN_SERVICE > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Recover single node - Verify ${SINGLE_COMPUTE} recovered"

# Test Case 2: Recover multiple nodes - Restart Daemons
${FVT_PATH}/tools/dual_aggregator/shutdown_daemons.sh > ${TEMP_LOG} 2>&1
${FVT_PATH}/tools/dual_aggregator/start_daemons.sh > ${TEMP_LOG} 2>&1
sleep 5

# Test Case 2: Recover multiple nodes - Put Compute Nodes in Soft Failure
xdsh ${COMPUTE_NODES} "systemctl restart csmd-compute" > ${TEMP_LOG} 2>&1
sleep 5

# Test Case 2: Recover multiple nodes - Verify Compute Nodes in Soft Failure state
${CSM_PATH}/csm_node_attributes_query -n ${COMPUTE_NODES} -s SOFT_FAILURE > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Recover multiple nodes - Verify Compute Nodes in Soft Failure state"

# Test Case 2: Recover multiple nodes - Run API
${CSM_PATH}/csm_soft_failure_recovery > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Recover multiple nodes - Run API"

# Test Case 2: Recover multiple nodes - Verify Compute Nodes recovered
${CSM_PATH}/csm_node_attributes_query -n ${COMPUTE_NODES} -s IN_SERVICE > ${TEMP_LOG} 2>&1 
check_return_exit $? 0 "Test Case 2: Recover multiple nodes - Verify Compute Nodes recovered"

# Test Case 3: 1 in allocation - Restart Daemons
${FVT_PATH}/tools/dual_aggregator/shutdown_daemons.sh > ${TEMP_LOG} 2>&1
${FVT_PATH}/tools/dual_aggregator/start_daemons.sh > ${TEMP_LOG} 2>&1
sleep 5

# Test Case 3: 1 in allocation - Create allocation
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -s IN_SERVICE > ${TEMP_LOG} 2>&1
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: 1 in allocation - Create allocation"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 3: 1 in allocation - Put Computes in Soft Failure
xdsh ${COMPUTE_NODES} "systemctl restart csmd-compute" > ${TEMP_LOG} 2>&1
sleep 5

# Test Case 3: 1 in allocation - Verify Computes in Soft Failure
${CSM_PATH}/csm_node_attributes_query -n ${COMPUTE_NODES} -s SOFT_FAILURE > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: 1 in allocation - Verify Computes in Soft Failure"

# Test Case 3: 1 in allocation - Run API
${CSM_PATH}/csm_soft_failure_recovery > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: 1 in allocation - Run API"

# Test Case 3: 1 in allocation - Verify free nodes recovered
${CSM_PATH}/csm_node_attributes_query -n ${COMPUTE_NODES} -s IN_SERVICE > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 3: 1 in allocation - Verify free nodes recovered"

# Test Case 3: 1 in allocation - Verify allocated node in Soft Failure
${CSM_PATH}/csm_node_attributes_query -n ${COMPUTE_NODES} -s SOFT_FAILURE > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 3: 1 in allocation - Verify allocated node in Soft Failure"

# Test Case 3: 1 in allocation - Cleanup 
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
${CSM_PATH}/csm_soft_failure_recovery > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: 1 in allocation - Cleanup"

rm -f ${TEMP_LOG} 
echo "------------------------------------------------------------" >> ${LOG}
echo "         Soft Failure Recovery Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
