#================================================================================
#   
#    buckets/error_injection/messaging.sh
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

# Error Injection bucket for daemon to daemon messaging protocol

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/error_injection/messaging.log
CSM_PATH=/opt/ibm/csm/bin
TEMP_LOG=${LOG_PATH}/buckets/error_injection/messaging_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/error_injection/messaging_flags.log
ERROR_INJECT=${CAST_PATH}/.build/csmd/src/daemon/tests/error_inject

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
	. "${BASH_SOURCE%/*}/../../include/functions.sh"
else
	echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "       Starting Error Injected Messaging Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Check for .build directory
ls ${ERROR_INJECT} > /dev/null 2>&1
check_return_exit $? 0 "Checking for error inject command in CAST/.build/"

# Test Case 1: Small message protocol - return code
${ERROR_INJECT} -m 100 > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 1: Small message protocol - return code"

# Test Case 1: Small message protocol - logging
check_all_output "Recieve Payload len = 100"
check_return_flag $? "Test Case 1: Small message protocol - return code" 

# Test Case 2: Truncated message protocol - return code
${ERROR_INJECT} -m 16777143 > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 2: Truncated message protocol - return code"

# Test Case 2: Truncated message protocol - logging
check_all_output "Partial total_data=16777143; this part=229304" "Recieve Payload len = 16777143"
check_return_flag $? "Test Case 2: Truncated message protocol - return code" 

# Test Case 3: Large message protocol - return code
${ERROR_INJECT} -m 16777144 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 3 "Test Case 3: Large message protocol - return code"

# Test Case 3: Large message protocol - logging
check_all_output "ERROR: errcode = 22 errmsg = ERROR: Network error detected." "ERROR: errcode = 12 errmsg = Send or Recv Error"
check_return_flag $? "Test Case 3: Large message protocol - return code" 

echo "------------------------------------------------------------" >> ${LOG}
echo "       Error Injected Messaging Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Cleanup
rm -f ${TEMP_LOG}
exit 0
