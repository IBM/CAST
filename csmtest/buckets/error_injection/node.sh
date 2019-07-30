#================================================================================
#   
#    buckets/error_injection/node.sh
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

# Error Injection Bucket for node testing
# node resources_query, attributes_query, attributes_update, attributes_query_details, attributes_query_history with bad input, behavior

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/error_injection/node.log
CSM_PATH=/opt/ibm/csm/bin
TEMP_LOG=${LOG_PATH}/buckets/error_injection/node_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/error_injection/node_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
        exit 1
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "            Starting Error Injected Node Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: csm_node_resources_query NO ARGS
${CSM_PATH}/csm_node_resources_query > ${TEMP_LOG} 2>&1
check_return_flag_value $? 8 "Test Case 1: csm_node_resources_query NO ARGS"

# Test Case 2: csm_node_resources_query node does not exist
${CSM_PATH}/csm_node_resources_query -n 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 4 "Test Case 2: csm_node_resources_query node does not exist"

# Test Case 3: csm_node_resources_query no usable input
${CSM_PATH}/csm_node_resources_query -n , > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 3: csm_node_resources_query no usable input"

# Test Case 4: csm_node_resources_query invalid option
${CSM_PATH}/csm_node_resources_query -x 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 4: csm_node_resources_query invalid option"

# Test Case 5: csm_node_attributes_query NO ARGS
${CSM_PATH}/csm_node_attributes_query > ${TEMP_LOG} 2>&1
check_return_flag_value $? 8 "Test Case 5: csm_node_attributes_query NO ARGS"

# Test Case 6: csm_node_attributes_query node does not exist
${CSM_PATH}/csm_node_attributes_query -n 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 4 "Test Case 5: csm_node_attributes_query NO ARGS"

# Test Case 7: csm_node_attributes_query no usable input
${CSM_PATH}/csm_node_attributes_query -n , > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 7: csm_node_attributes_query no usable input"

# Test Case 8: csm_node_attributes_query invalid option
${CSM_PATH}/csm_node_attributes_query -x 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 8: csm_node_attributes_query invalid option"

# Test Case 9: csm_node_delete NO ARGS
${CSM_PATH}/csm_node_delete > ${TEMP_LOG} 2>&1
check_return_flag_value $? 8 "Test Case 9: csm_node_delete NO ARGS"

# Test Case 10: csm_node_delete no usable input
${CSM_PATH}/csm_node_delete -n , > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 10: csm_node_delete no usable input"

# Test Case 11: csm_node_delete invalid option
${CSM_PATH}/csm_node_delete -x 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 11: csm_node_delete invalid option"

# Test Case 12: csm_node_delete node does not exist
${CSM_PATH}/csm_node_delete -n 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 26 "Test Case 12: csm_node_delete node does not exist"

# Test Case 13: csm_node_delete on multiple nodes (1 exists, one does not)
${CSM_PATH}/csm_node_delete -n ${SINGLE_COMPUTE},123456789 > ${TEMP_LOG} 2>&1
check_all_output "Could not delete 1 of the 2 record(s)"
check_return_flag_value $? 0 "Test Case 13: csm_node_delete on multiple nodes (1 exists, one does not)"

# Recover deleted node
xdsh ${SINGLE_COMPUTE} "systemctl stop csmd-compute"
sleep 10
xdsh ${SINGLE_COMPUTE} "systemctl start csmd-compute"
sleep 10
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -r y > ${TEMP_LOG} 2>&1

# Test Case 14: csm_node_attributes_update NO ARGS
${CSM_PATH}/csm_node_attributes_update > ${TEMP_LOG} 2>&1
check_return_flag_value $? 8 "Test Case 14: csm_node_attributes_update NO ARGS"

# Test Case 15: csm_node_attributes_update only 1 of 2 required inputs
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 8 "Test Case 15: csm_node_attributes_update only 1 of 2 required inputs"

# Test Case 16: csm_node_attributes_update node does not exist, valid input otherwise
${CSM_PATH}/csm_node_attributes_update -n 123456789 -s IN_SERVICE > ${TEMP_LOG} 2>&1
check_return_flag_value $? 27 "Test Case 16: csm_node_attributes_update node does not exist, valid input otherwise"

# Test Case 17: csm_node_attributes_update no usable -n input
${CSM_PATH}/csm_node_attributes_update -n , -r y > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 17: csm_node_attributes_update no usable -n input"

# Test Case 18: csm_node_attributes_update invalid option
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -x 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 18: csm_node_attributes_update invalid option"

# Test Case 19: csm_node_attributes_update invalid secondary input (-r)
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -r 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 19: csm_node_attributes_update invalid secondary input (-r)"

# Test Case 20: csm_node_attributes_update invalid -s input
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -s 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 20: csm_node_attributes_update invalid -s input"

# Test Case 21: csm_node_attributes_query_history NO ARGS
${CSM_PATH}/csm_node_attributes_query_history > ${TEMP_LOG} 2>&1
check_return_flag_value $? 8 "Test Case 21: csm_node_attributes_query_history NO ARGS"

# Test Case 22: csm_node_attributes_query_history multiple nodes / node does not exist
${CSM_PATH}/csm_node_attributes_query_history -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 4 "Test Case 22: csm_node_attributes_query_history multiple nodes / node does not exist"

# Test Case 23: csm_node_attributes_query_history invalid option
${CSM_PATH}/csm_node_attributes_query_history -x 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 23: csm_node_attributes_query_history invalid option"

# Test Case 24: csm_node_attributes_query_details NO ARGS
${CSM_PATH}/csm_node_attributes_query_details > ${TEMP_LOG} 2>&1
check_return_flag_value $? 8 "Test Case 24: csm_node_attributes_query_details NO ARGS"

# Test Case 25: csm_node_attributes_query_details multiple nodes / node does not exist
${CSM_PATH}/csm_node_attributes_query_details -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 4 "Test Case 25: csm_node_attributes_query_details multiple nodes / node does not exist"

# Test Case 26: csm_node_attributes_query_details invalid option
${CSM_PATH}/csm_node_attributes_query_details -x 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 9 "Test Case 26: csm_node_attributes_query_details invalid option"

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "           Error Injected Node Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
