#================================================================================
#   
#    buckets/error_injection/allocation.sh
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

# Error Injection Bucket for allocation testing
# Allocation create, query, query_details, update_state, and delete with bad input, behavior

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/error_injection/allocation.log
CSM_PATH=/opt/ibm/csm/bin
TEMP_LOG=${LOG_PATH}/buckets/error_injection/allocation_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/error_injection/allocation_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
	exit 1
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "        Starting Error Injected Allocation Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: csm_allocation_create NO ARGS
${CSM_PATH}/csm_allocation_create > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 1: csm_allocation_create NO ARGS"

# Test Case 2: csm_allocation_create node is out of service
# set and verify node is out of service
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -s "OUT_OF_SERVICE" > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 2: csm_allocation_create node is out of service - update to OUT_OF_SERVICE"
${CSM_PATH}/csm_node_attributes_query -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_all_output "OUT_OF_SERVICE"
check_return_flag $? "Test Case 2: csm_allocation_create node is out of service - verify OUT_OF_SERVICE"
# create
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 48 "Test Case 2: csm_allocation_create node is out of service"
# set and verify back in service
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -s "IN_SERVICE" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: csm_allocation_create node is out of service - set back to IN_SERVICE"
${CSM_PATH}/csm_node_attributes_query -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_all_output "IN_SERVICE"
check_return_exit $? 0 "Test Case 2: csm_allocation_create node is out of service - verify back IN_SERVICE"

# Test Case 3: csm_allocation_create permission denied
su -c "${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE}" plundgr > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Test Case 3: csm_allocation_create permission denied"

# Test Case 4: csm_allocation_create prolog error 255
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/prologs/privileged_prolog_255 /opt/ibm/csm/prologs/privileged_prolog
check_return_exit $? 0 "Test Case 4: csm_allocation_create prolog error 255 - copy prolog"
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 49 "Test Case 4: csm_allocation_create prolog error 255"
check_all_output "Privileged script execution failure detected. Invalid allocation flags"
check_return_flag $? "Test Case 4: csm_allocation_create prolog error 255 - verify error message"

# Test Case 5: csm_allocation_create prolog error generic
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/prologs/privileged_prolog_generic /opt/ibm/csm/prologs/privileged_prolog
check_return_exit $? 0 "Test Case 5: csm_allocation_create prolog error generic - copy prolog"
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 17 "Test Case 5: csm_allocation_create prolog error generic"
check_all_output "Privileged script execution failure detected. Error code received: 2"
check_return_flag $? "Test Case 5: csm_allocation_create prolog error generic - verify error message"

# Test Case 6: csm_allocation_create prolog error timeout
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/prologs/privileged_prolog_timeout /opt/ibm/csm/prologs/privileged_prolog
check_return_exit $? 0 "Test Case 6: csm_allocation_create prolog error timeout - copy prolog"
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 5 "Test Case 6: csm_allocation_create prolog error timeout"
check_all_output "Request timeout detected"
check_return_flag $? "Test Case 6: csm_allocation_create prolog error timeout - verify error message"
# set node back to IN_SERVICE after timeout
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -s "IN_SERVICE" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6: csm_allocation_create prolog error timeout - set node back to IN_SERVICE after timeout"
${CSM_PATH}/csm_node_attributes_query -n ${SINGLE_COMPUTE} -s "IN_SERVICE" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6: csm_allocation_create prolog error timeout - verify node back IN_SERVICE after timeout"
# restore working prolog
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/prologs/privileged_prolog /opt/ibm/csm/prologs/privileged_prolog
check_return_exit $? 0 "Test Case 6: csm_allocation_create prolog error timeout - restore working prolog"

# Test Case 2: csm_allocation_create success
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: csm_allocation_create success"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 3: csm_allocation_create node does not exist
${CSM_PATH}/csm_allocation_create -j 1 -n doesnotexist > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 46 "Test Case 3: csm_allocation_create node does not exist"

# Test Case 4: csm_allocation_create allocation already exists
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 47 "Test Case 4: csm_allocation_create allocation already exists"

# Test Case 5: csm_allocation_create new job, but node is busy
${CSM_PATH}/csm_allocation_create -j 2 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 47 "Test Case 5: csm_allocation_create new job, but node is busy"

# Test Case 6: csm_allocation_create invalid -j input
${CSM_PATH}/csm_allocation_create -j xxx -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 6: csm_allocation_create invalid -j input"

# Test Case 7: csm_allocation_create invalid option
${CSM_PATH}/csm_allocation_create -l > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 7: csm_allocation_create invalid option"

# Test Case 8: csm_allocation_resources_query NO ARGS
${CSM_PATH}/csm_allocation_resources_query > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 8: csm_allocation_resources_query NO ARGS"

# Test Case 9: csm_allocation_resources_query allocation does not exist / is inactive
${CSM_PATH}/csm_allocation_resources_query -a 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 9: csm_allocation_resources_query allocation does not exist / is inactive"

# Test Case 10: csm_allocation_resources_query invalid option
${CSM_PATH}/csm_allocation_resources_query -x 123 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 10: csm_allocation_resources_query invalid option"

# Test Case 11: csm_allocation_resources_query invalid -a input
${CSM_PATH}/csm_allocation_resources_query -a abc > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 11: csm_allocation_resources_query invalid -a input"

# Test Case 12: csm_allocation_query NO ARGS
${CSM_PATH}/csm_allocation_query > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 12: csm_allocation_query NO ARGS"

# Test Case 13: csm_allocation_query allocation does not exist
${CSM_PATH}/csm_allocation_query -a 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 13: csm_allocation_query allocation does not exist"

# Test Case 14: csm_allocation_query job id does not exist
${CSM_PATH}/csm_allocation_query -j 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 14: csm_allocation_query job id does not exist"

# Test Case 15: csm_allocation_query more than 1 allocation with job_id=1
${CSM_PATH}/csm_allocation_query -j 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 0 "Test Case 15: csm_allocation_query more than 1 allocation with job_id=1"

# Test Case 16: csm_allocation_query invalid -a option
${CSM_PATH}/csm_allocation_query -a xxx > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 16: csm_allocation_query invalid -a option"

# Test Case 17: csm_allocation_query invalid -j option
${CSM_PATH}/csm_allocation_query -j xxx > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 17: csm_allocation_query invalid -j option"

# Test Case 18: csm_allocation_query invalid -J option
${CSM_PATH}/csm_allocation_query -J xxx > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 18: csm_allocation_query invalid -J option"

# Test Case 19: csm_allocation_query -J option can not stand alone in valid query
${CSM_PATH}/csm_allocation_query -J 0 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 19: csm_allocation_query -J option can not stand alone in valid query"

# Test Case 20: csm_allocation_query invalid option
${CSM_PATH}/csm_allocation_query -l > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 20: csm_allocation_query invalid option"

# Test Case 21: csm_allocation_query_details NO ARGS
${CSM_PATH}/csm_allocation_query_details > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 21: csm_allocation_query_details NO ARGS"

# Test Case 22: csm_allocation_query_details allocation does not exist
${CSM_PATH}/csm_allocation_query_details -a 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 22: csm_allocation_query_details allocation does not exist"

# Test Case 23: csm_allocation_query_details invalid -a input
${CSM_PATH}/csm_allocation_query_details -a xxx > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 23: csm_allocation_query_details invalid -a input"

# Test Case 24: csm_allocation_query_details invalid option
${CSM_PATH}/csm_allocation_query_details -l 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 24: csm_allocation_query_details invalid option" 

# Test Case 25: csm_allocation_update_state NO ARGS
${CSM_PATH}/csm_allocation_update_state > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 25: csm_allocation_update_state NO ARGS"

# Test Case 26: csm_allocation_update_state invalid -s input
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "invalid" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 26: csm_allocation_update_state invalid -s input"

# Test Case 27: csm_allocation_update_state running->running not allowed
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "running" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 27: csm_allocation_update_state running->running not allowed"

# Test Case 28: Checking allocation state still running
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "running"
check_return_flag $? "Test Case 28: Checking allocation state still running"

# Test Case 29: csm_allocation_update_state allocation does not exist
${CSM_PATH}/csm_allocation_update_state -a 123456789 -s "running" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 29: csm_allocation_update_state allocation does not exist"

# Test Case 30: csm_allocation_update_state invalid -a input
${CSM_PATH}/csm_allocation_update_state -a xxx -s "running" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 30: csm_allocation_update_state invalid -a input"

# Test Case 31: csm_allocation_update_state invalid option
${CSM_PATH}/csm_allocation_update_state -l 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 31: csm_allocation_update_state invalid option"

# Test Case 32: csm_allocation_delete NO ARGS
${CSM_PATH}/csm_allocation_delete > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 32: csm_allocation_delete NO ARGS"

# Test Case 33: csm_allocation_delete allocation does not exist
${CSM_PATH}/csm_allocation_delete -a 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 50 "Test Case 33: csm_allocation_delete allocation does not exist"

# Test Case 34: csm_allocation_delete primary job id does not exist
${CSM_PATH}/csm_allocation_delete -j 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 50 "Test Case 34: csm_allocation_delete primary job id does not exist"

# Test Case 34: csm_allocation_delete invalid -a input
${CSM_PATH}/csm_allocation_delete -a xxx > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 34: csm_allocation_delete invalid -a input"

# Test Case 35: csm_allocation_update_state running->staging-out success
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "staging-out" > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 35: csm_allocation_update_state running->staging-out success"

# Test Case 36: Checking allocation state updated to staging-out
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-out"
check_return_exit $? 0 "Test Case 36: Checking allocation state updated to staging-out"

# Test Case 37: csm_allocation_update_state staging-out->running not allowed
${CSM_PATH}/csm_allocation_update_state -a ${allocation_id} -s "running" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 37: csm_allocation_update_state staging-out->running not allowed"

# Test Case 38: Checking allocation state still staging-out
${CSM_PATH}/csm_allocation_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "staging-out"
check_return_exit $? 0 "Test Case 38: Checking allocation state still staging-out"

# Test Case 39: csm_allocation_update_history before allocation deleted (i.e. added to history table)
${CSM_PATH}/csm_allocation_update_history -a ${allocation_id} -c "test comment" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 39: csm_allocation_update_history before allocation deleted (i.e. added to history table)"

# Test Case 40: csm_allocation_delete permission denied
su -c "${CSM_PATH}/csm_allocation_delete -a ${allocation_id}" plundgr > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 16 "Test Case 40: csm_allocation_delete permission denied"

# Test Case 40: csm_allocation_delete success
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 40: csm_allocation_delete success"

# Test Case 41: csm_allocation_update_history NO ARGS
${CSM_PATH}/csm_allocation_update_history > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 41: csm_allocation_update_history NO ARGS"

# Test Case 42: csm_allocation_update_history allocation does not exist in history table
${CSM_PATH}/csm_allocation_update_history -a 123456789 -c "test comment" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 42: csm_allocation_update_history allocation does not exist in history table"

# Test Case 43: csm_allocation_update_history missing required argument
${CSM_PATH}/csm_allocation_update_history -c "test comment" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 43: csm_allocation_update_history missing required argument"

# Test Case 44: csm_allocation_update_history invalid option
${CSM_PATH}/csm_allocation_update_history -a ${allocation_id} -x "test comment" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 44: csm_allocation_update_history invalid option"

# Test Case 45: csm_allocation_update_history invalid input for required argument
${CSM_PATH}/csm_allocation_update_history -a testcase -c "test comment" > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 45: csm_allocation_update_history invalid input for required argument"

# Create allocation for next test case
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 46: csm_allocation_delete generic epilog error
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/prologs/privileged_epilog_generic /opt/ibm/csm/prologs/privileged_epilog
check_return_exit $? 0 "Test Case 46: csm_allocation_delete generic epilog error - copy epilog"
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 17 "Test Case 46: csm_allocation_delete generic epilog error"
check_all_output "Privileged script execution failure detected. Error code received: 2"
check_return_flag $? "Test Case 46: csm_allocation_delete generic epilog error - verify error message"

# Create allocation for next test case
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 46: csm_allocation_delete epilog timeout error
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/prologs/privileged_epilog_timeout /opt/ibm/csm/prologs/privileged_epilog
check_return_exit $? 0 "Test Case 46: csm_allocation_delete epilog timeout error - copy epilog"
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 5 "Test Case 46: csm_allocation_delete epilog timeout error"
check_all_output "Request timeout detected"
check_return_flag $? "Test Case 46: csm_allocation_delete epilog timeout error - verify error message"

# Verify allocation deleted
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Test Case 46: csm_allocation_delete epilog timeout error - verify no alloc after timeout"

# set node back to IN_SERVICE after timeout
${CSM_PATH}/csm_node_attributes_update -n ${SINGLE_COMPUTE} -s "IN_SERVICE" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 46: csm_allocation_delete epilog timeout error - set node back to IN_SERVICE"
${CSM_PATH}/csm_node_attributes_query -n ${SINGLE_COMPUTE} -s "IN_SERVICE" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 46: csm_allocation_delete epilog timeout error - verify node back IN_SERVICE"
# restore working epilog
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/prologs/privileged_epilog /opt/ibm/csm/prologs/privileged_epilog
check_return_exit $? 0 "Test Case 6: csm_allocation_delete epilog timeout error - restore working epilog"

# Clean up temp log so ${allocation_id} gets set correctly on next run
rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "      Error Injected Allocation Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}\n" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
