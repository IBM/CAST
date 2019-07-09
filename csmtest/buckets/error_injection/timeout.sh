#================================================================================
#
#    buckets/error_injection/timeout.sh
#
#  © Copyright IBM Corporation 2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# Error Injection Bucket for timeout testing
# Using error inject tool to test timeout.

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/error_injection/timeout.log
CSM_PATH=/opt/ibm/csm/bin
TEMP_LOG=${LOG_PATH}/buckets/error_injection/timeout_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/error_injection/timeout_flags.log
ERROR_INJECT=${CAST_PATH}/.build/csmd/src/daemon/tests/error_inject

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
        exit 1
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "        Starting Error Injected Timeout Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: timeout recurring tasks - edit master config
sed -i -- "s/\"interval\".*/\"interval\" : \"00:00:15\",/g" /etc/ibm/csm/csm_master.cfg
grep "\"interval\" : \"00:00:15\"" /etc/ibm/csm/csm_master.cfg >> /dev/null
check_return_exit $? 0 "Test Case 1: timeout recurring tasks - edit master config"

# Test Case 1: timeout recurring tasks - shut down the daemons
${FVT_PATH}/tools/dual_aggregator/shutdown_daemons.sh > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: timeout recurring tasks - shut down the daemons"

# Test Case 1: timeout recurring tasks - start the daemons
${FVT_PATH}/tools/dual_aggregator/start_daemons.sh > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: timeout recurring tasks - start the daemons"

sleep 5

# Test Case 1: timeout recurring tasks - health check verification
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: timeout recurring tasks - health check verification"

# Test Case 1: timeout recurring tasks - error inject timeout
{ time ${ERROR_INJECT} -t 20 ; } > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: timeout recurring tasks - error inject timeout"

# Test Case 1: timeout recurring tasks - verify timeout result
time=`grep real ${TEMP_LOG} | awk -F'm' '{print $2}'`
sec=${time%.*}

if [ $sec -ge 20 ]
then
    bool=0
else
    bool=1
fi

check_return_exit $bool 0 "Test Case 1: timeout recurring tasks - verify timeout result"

# Clean up temp log
rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "      Error Injected Timeout Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}\n" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
