#================================================================================
#   
#    buckets/basic/jsrun_cmd.sh
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

# Bucket for jsrun_cmd testing

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/jsrun_cmd.log
TEMP_LOG=${LOG_PATH}/buckets/basic/jsrun_cmd_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/jsrun_cmd_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "              Starting Basic jsrun_cmd Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Create allocation
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} --isolated_cores 2 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Create Allocation"

# Grab & store allocation ID
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Send jsrun_cmd_test_script.sh to compute node
xdcp ${SINGLE_COMPUTE} ${FVT_PATH}/include/jsrun_cmd_test_script.sh /tmp/jsrun_cmd_test_script.sh > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Send jsrun_cmd_test_script.sh to compute node"

# Modify jsrun_cmd_test_script.sh to run on compute node
xdsh ${SINGLE_COMPUTE} "chmod a+x /tmp/jsrun_cmd_test_script.sh" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Modify jsrun_cmd_test_script.sh to run on compute node"

# Test Case 1: Call jsrun_cmd
${CSM_PATH}/csm_jsrun_cmd -a ${allocation_id} -p /tmp/jsrun_cmd_test_script.sh > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Call jsrun_cmd"

sleep 5

# Test Case 2: Validate jsrun_test file output
xdsh ${SINGLE_COMPUTE} "cat /tmp/jsrun_test" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: /tmp/jsrun_test exists on ${SINGLE_COMPUTE}"
check_all_output "CSM_ALLOCATION_ID=${allocation_id}" "CSM_JSM_ARGS" "cpuset:/allocation_${allocation_id}"
check_return_flag_value $? 0 "Test Case 2: Validating jsrun_test file output"

# Clean up allocation
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Clean up allocation"
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Validating no active allocations"

# Clean up test script
xdsh ${SINGLE_COMPUTE} "rm -f /tmp/jsrun_cmd_test_script.sh" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Clean up test script"
xdsh ${SINGLE_COMPUTE} "cat /tmp/jsrun_cmd_test_script.sh" > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Validating no test script on ${SINGLE_COMPUTE}"

# Clean up output file
xdsh ${SINGLE_COMPUTE} "rm -f /tmp/jsrun_test" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Clean up output file"
xdsh ${SINGLE_COMPUTE} "cat /tmp/jsrun_test" > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Validating no output file on ${SINGLE_COMPUTE}"

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "           Basic jsrun_cmd Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
