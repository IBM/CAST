#================================================================================
#   
#    buckets/basic/csm_ctrl_cmd.sh
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

# Bucket for csm_ctrl_cmd testing

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/csm_ctrl_cmd.log
TEMP_LOG=${LOG_PATH}/buckets/basic/csm_ctrl_cmd_tmp.log
TEMP2_LOG=${LOG_PATH}/buckets/basic/csm_ctrl_cmd_tmp2.log
FLAG_LOG=${LOG_PATH}/buckets/basic/csm_ctrl_cmd_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "            Starting Basic csm_ctrl_cmd Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Get Target Compute Node
target_node=`nodels compute_B | head -1`

# Start with Fresh CSM environment
${BASH_SOURCE%/*}/../../tools/dual_aggregator/shutdown_daemons.sh
${BASH_SOURCE%/*}/../../tools/dual_aggregator/start_daemons.sh

# Pre-bucket Check: Validate 2 Active Aggregators
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_all_output "Aggregators:2" "Unresponsive Aggregators: 0"
check_return_exit $? 0 "Pre-bucket Check: Validate 2 Active Aggregators"

# Setup Step 1: Shutdown Aggregator B
xdsh ${AGGREGATOR_B} "systemctl stop csmd-aggregator" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Setup Step 1: Shutdown Aggregator B"

# Sleep for default heartbeat interval
sleep 15

# Setup Step 1: Validate 1 Active Aggregator, 1 Unresponsive
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_all_output "Aggregators:2" "Unresponsive Aggregators: 1" "AGGREGATOR: ${AGGREGATOR_B} (bounced=1; version=n/a)" 
check_return_exit $? 0 "Setup Step 1: Validate 1 Active Aggregator, 1 Unresponsive"

# Crop TEMP_LOG to include only the health check output for Aggregator A up until target node entry
sed -n -e "/AGGREGATOR: ${AGGREGATOR_A}/,/COMPUTE: ${target_node}/ p" ${TEMP_LOG} > ${TEMP2_LOG}
cat ${TEMP2_LOG} > ${TEMP_LOG} 2>&1

# Setup Step 1: Validate Target Compute Node is linked to Primary under Aggregator A
check_all_output "${target_node}.*PRIMARY"
check_return_exit $? 0 "Setup Step 1: Validate Target Compute Node is linked to Primary under Aggregator A"

# Setup Step 2: Start Aggregator B
xdsh ${AGGREGATOR_B} "systemctl start csmd-aggregator" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Setup Step 2: Start Aggregator B"

# Sleep for default heartbeat interval
sleep 15

# Setup Step 2: Validate 2 Active Aggregators
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_all_output "Aggregators:2" "Unresponsive Aggregators: 0"
check_return_exit $? 0 "Setup Step 2: Validate 2 Active Aggregators"

# Crop TEMP_LOG to include only the health check output for Aggregator A up until target node entry
sed -n -e "/AGGREGATOR: ${AGGREGATOR_A}/,/COMPUTE: ${target_node}/ p" ${TEMP_LOG} > ${TEMP2_LOG}
cat ${TEMP2_LOG} > ${TEMP_LOG} 2>&1

# Setup Step 2: Validate Target Compute Node is still linked to Primary under Aggregator A
check_all_output "${target_node}.*PRIMARY"
check_return_exit $? 0 "Setup Step 2: Validate Target Compute Node is still linked to Primary under Aggregator A"

# Test Case 1: call csm_ctrl_cmd --agg.reset on Target Compute Node
xdsh ${target_node} "/opt/ibm/csm/sbin/csm_ctrl_cmd --agg.reset" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: call csm_ctrl_cmd --agg.reset on Target Compute Node"

# Test Case 1: call Health Check
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: call Health Check"

# Crop TEMP_LOG to include only the health check output for Aggregator A up until target node entry
sed -n -e "/AGGREGATOR: ${AGGREGATOR_A}/,/COMPUTE: ${target_node}/ p" ${TEMP_LOG} > ${TEMP2_LOG}
cat ${TEMP2_LOG} > ${TEMP_LOG} 2>&1

# Test Case 1: Validate Target Compute Node has moved back to Secondary link under Aggregator A
check_all_output "${target_node}.*SECONDARY"
check_return_exit $? 0 "Test Case 1: Validate Target Compute Node has moved back to Secondary link under Aggregator A"

# Health check and Crop to include only the health check output for Aggregator B up until target node entry
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
sed -n -e "/AGGREGATOR: ${AGGREGATOR_B}/,/COMPUTE: ${target_node}/ p" ${TEMP_LOG} > ${TEMP2_LOG}
cat ${TEMP2_LOG} > ${TEMP_LOG} 2>&1

# Test Case 1: Validate Target Compute Node has moved back to Primary link under Aggregator B
check_all_output "${target_node}.*PRIMARY"
check_return_exit $? 0 "Test Case 1: Validate Target Compute Node has moved back to Primary link under Aggregator B"

rm -f ${TEMP_LOG}
rm -f ${TEMP2_LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "           Basic csm_ctrl_cmd Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Cleanup CSM environment
${BASH_SOURCE%/*}/../../tools/dual_aggregator/shutdown_daemons.sh
${BASH_SOURCE%/*}/../../tools/dual_aggregator/start_daemons.sh

exit 0
