#================================================================================
#   
#    buckets/basic/ras.sh
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

# Bucket for basic CSM RAS testing
# 1 cycle of the RAS API command lines
# csm_ras_msg_type_create
# csm_ras_msg_type_query   
# csm_ras_msg_type_update 
# csm_ras_event_create     
# csm_ras_event_query      
# csm_ras_msg_type_delete  

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
	. "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
	echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../../csm_test.cfg", exitting."
	exit 1
fi

# Local variables used by this script
LOG=${LOG_PATH}/buckets/basic/ras.log
TEMP_LOG=${LOG_PATH}/buckets/basic/ras_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/ras_flags.log

CSMTEST_RAS_BASIC_MSG_ID="csmtest.ras.basic"
CSMTEST_RAS_BASIC_MSG="Test message for the CSM RAS basic test bucket."

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "             Starting RAS Basic Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Calling csm_allocation_create
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} 2>&1 > ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1: Calling csm_allocation_create"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 2: Calling csm_ras_msg_type_create
${CSM_PATH}/csm_ras_msg_type_create -m ${CSMTEST_RAS_BASIC_MSG_ID} -M ${CSMTEST_RAS_BASIC_MSG} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 2: Calling csm_ras_msg_type_create"

# Test Case 3: Calling csm_ras_msg_type_query
${CSM_PATH}/csm_ras_msg_type_query -m ${CSMTEST_RAS_BASIC_MSG_ID} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 3: Calling csm_ras_msg_type_query"

# Test Case 4: Calling csm_ras_msg_type_update
${CSM_PATH}/csm_ras_msg_type_update -m ${CSMTEST_RAS_BASIC_MSG_ID} -t 1 > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 4: Calling csm_ras_msg_type_update"

# Test Case 5: Calling csm_ras_event_create
${CSM_PATH}/csm_ras_event_create -m ${CSMTEST_RAS_BASIC_MSG_ID} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 5: Calling csm_ras_event_create"

sleep 1

# Test Case 6: Calling csm_ras_event_query
${CSM_PATH}/csm_ras_event_query -m ${CSMTEST_RAS_BASIC_MSG_ID} > ${TEMP_LOG} 2>&1 
check_return_flag_value $? 0 "Test Case 6: Calling csm_ras_event_query"

# Test Case 7: Calling csm_ras_event_query_allocation
${CSM_PATH}/csm_ras_event_query_allocation -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 7: Calling csm_ras_event_query_alloaction"

# Test Case 8: Calling csm_allocation_delete
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 8: Calling csm_allocation_delete"

# Test Case 9: Calling csm_ras_msg_type_delete
${CSM_PATH}/csm_ras_msg_type_delete -m ${CSMTEST_RAS_BASIC_MSG_ID} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 9: Calling csm_ras_msg_type_delete"

rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "             RAS Basic Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
