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

# Bucket for node testing
# resources/attributes querys, update, attributes_query_details & history

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/node.log
TEMP_LOG=${LOG_PATH}/buckets/basic/node_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/node_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting node Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}


# Get and Print Git Commit # of Build
git=`head -1 /var/log/ibm/csm/csm_master.log | awk -F 'Build: ' '{print $2}' | cut -f1 -d ';'`
echo "Git Commit for Build: ${git}" >> ${LOG}

# Get and Print RPM version of Build
rpm_ver=`rpm -qa | grep ibm-csm-core | awk -F 'core-' '{print $2}' | awk -F '.ppc64le' '{print $1}'`
echo "RPM version for Build: ${rpm_ver}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

echo "CALLING csm_infrastructure_health_check..." >> ${LOG}
/opt/ibm/csm/bin/csm_infrastructure_health_check -v >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: csm_node_resources_query_all and checking ready=n
${CSM_PATH}/csm_node_resources_query_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: csm_node_resources_query_all"
check_all_output "ready: .* n"
check_return_flag $? "Test Case 1: check node_ready=n"

rm -f ${TEMP_LOG}
# Test Case 2: csm_node_resources_query on 1 node
${CSM_PATH}/csm_node_resources_query -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Calling csm_node_resources_query on 1 node"

rm -f ${TEMP_LOG}
# Test Case 3: csm_node_resources_query on all nodes
${CSM_PATH}/csm_node_resources_query -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: csm_node_resources_query on all nodes"

rm -f ${TEMP_LOG}
# Test Case 4: csm_node_attributes_query on 1 node
${CSM_PATH}/csm_node_attributes_query -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4: Calling csm_node_attributes_query on 1 node"

rm -f ${TEMP_LOG}
# Test Case 5: csm_node_attribute_query on all nodes
${CSM_PATH}/csm_node_attributes_query -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5: Calling csm_node_attributes_query on all nodes"

rm -f ${TEMP_LOG}
# Test Case 6: csm_node_attribtes_update to state=IN_SERVICE on all nodes
${CSM_PATH}/csm_node_attributes_update -n ${COMPUTE_NODES} -s IN_SERVICE > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6: Calling csm_node_attributes_update to state=IN_SERVICE on all nodes" 

rm -f ${TEMP_LOG}
# Test Case 7: csm_node_attributes_query on all nodes and check for state=IN_SERVICE
${CSM_PATH}/csm_node_attributes_query -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7: Calling csm_node_attributes_query on all nodes"
check_all_output "state: .* IN_SERVICE"
check_return_flag $? "Test Case 7: Checking for state=IN_SERVICE"

rm -f ${TEMP_LOG}
# Test Case 8: csm_node_query_state_history on 1 node and check for IN_SERVICE and CSM API
${CSM_PATH}/csm_node_query_state_history -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8: Calling csm_node_query_state_history on 1 node"
check_all_output "IN_SERVICE" "CSM API"
check_return_flag $? "Test Case 8: Checking for state=IN_SERVICE and CSM_API"

rm -f ${TEMP_LOG}
# Test Case 9: csm_node_attributes_query_details on 1 node
${CSM_PATH}/csm_node_attributes_query_details -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 9: Calling csm_node_attributes_query_details on 1 node"

rm -f ${TEMP_LOG}
# Test Case 10: csm_node_attributes_query_details on all nodes (error expected)
${CSM_PATH}/csm_node_attributes_query_details -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 10: csm_node_attributes_query_details on all nodes (error expected)"

rm -f ${TEMP_LOG}
# Test Case 11: csm_node_attributes_query_history on 1 node
${CSM_PATH}/csm_node_attributes_query_history -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 11: Calling csm_node_attributes_query_history on 1 node"

rm -f ${TEMP_LOG}
# Test Case 12: csm_node_attributes_query_history on all nodes (expected error)
${CSM_PATH}/csm_node_attributes_query_history -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 12: Calling csm_node_attributes_query_history on all nodes (error expected)"

rm -f ${TEMP_LOG}
# Test Case 13: csm_node_delete
${CSM_PATH}/csm_node_delete -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 13: calling csm_node_delete"

echo "RECOVERING ${SINGLE_COMPUTE}..." >> ${LOG}
xdsh ${SINGLE_COMPUTE} "systemctl stop csmd-compute"
sleep 5
xdsh ${SINGLE_COMPUTE} "systemctl start csmd-compute"
sleep 5
${CSM_PATH}/csm_node_resources_query_all | grep ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
RC=$?
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -s IN_SERVICE > ${TEMP_LOG} 2>&1 
if [ ${RC} -eq 0 ]
then
	echo "SUCCESS" >> ${LOG}
else
	echo "FAILED" >> ${LOG}
fi

rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "                node Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0

