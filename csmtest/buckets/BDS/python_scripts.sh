#================================================================================
#   
#    buckets/BDS/python_scripts.sh
# 
#  © Copyright IBM Corporation 2015-2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

# Bucket for BDS - python testing
# Allocation query, update, delete

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/BDS/python_scripts.log
TEMP_LOG=${LOG_PATH}/buckets/BDS/python_scripts_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/BDS/python_scripts_flag.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "           Starting BDS - python_scripts Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

#Test Case 1 setup:
${CSM_PATH}/csm_allocation_create -j 1 -n ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "PRE SETUP: Calling csm_allocation_create"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

rm -f ${TEMP_LOG}
echo "allocation_id: " $allocation_id >> ${LOG}

# Test Case 1: findJobsRunning 
# switch to csm path later
/opt/ibm/csm/bigdata/python/findJobsRunning.py > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 0 "Test Case 1 : Calling findJobsRunning.py "

# Grab & Store number of jobs found from findJobsRunning.py.
# we will need later for logical tests
foundJobs=`grep found ${TEMP_LOG} | awk -F' ' '{print $3}'`
echo "foundJobs: " $foundJobs >> ${LOG}

rm -f ${TEMP_LOG}

# Clean Up allocation
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Clean up allocation"
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 4 "Validating no active allocations"

rm -f ${TEMP_LOG}

# Test Case 2: findJobKeys
# switch to csm path later
/opt/ibm/csm/bigdata/python/findJobKeys.py > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 0 "Test Case 2 : Calling findJobKeys.py "

rm -f ${TEMP_LOG}

# Test Case 3: findJobMetrics
# switch to csm path later
/opt/ibm/csm/bigdata/python/findJobMetrics.py -f "host" > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 0 "Test Case 3 : Calling findJobMetrics.py "

rm -f ${TEMP_LOG}

# Test Case 3a: findJobMetrics
# switch to csm path later
/opt/ibm/csm/bigdata/python/findJobMetrics.py > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 2 "Test Case 3a: Calling findJobMetrics.py without required fields"

rm -f ${TEMP_LOG}

# Test Case 4: findJobsInRange
# switch to csm path later
/opt/ibm/csm/bigdata/python/findJobsInRange.py > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 0 "Test Case 4 : Calling findJobsInRange.py "

rm -f ${TEMP_LOG}

# Test Case 5: findJobTimeRange
# switch to csm path later
/opt/ibm/csm/bigdata/python/findJobTimeRange.py -a 1 > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 0 "Test Case 5 : Calling findJobTimeRange.py "

rm -f ${TEMP_LOG}

# Test Case 5a: findJobTimeRange with no id
# switch to csm path later
/opt/ibm/csm/bigdata/python/findJobTimeRange.py > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 2 "Test Case 5a: Calling findJobTimeRange.py without either allocation_id or job_id"

rm -f ${TEMP_LOG}

# Test Case 6: findUserJobs
# switch to csm path later
/opt/ibm/csm/bigdata/python/findUserJobs.py -u "root" > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 0 "Test Case 6 : Calling findUserJobs.py "

rm -f ${TEMP_LOG}

# Test Case 6a: findUserJobs
# switch to csm path later
/opt/ibm/csm/bigdata/python/findUserJobs.py > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 2 "Test Case 6a: Calling findUserJobs.py without required field -u user"

rm -f ${TEMP_LOG}

# Test Case 7: findWeightedErrors
# switch to csm path later
/opt/ibm/csm/bigdata/python/findWeightedErrors.py --errormap "/opt/ibm/csm/bigdata/python/sampleWeightedErrorMap.json" > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 0 "Test Case 7 : Calling findWeightedErrors.py "

rm -f ${TEMP_LOG}

# Test Case 7a: findWeightedErrors
# switch to csm path later
/opt/ibm/csm/bigdata/python/findWeightedErrors.py > ${TEMP_LOG} 2>&1
#echo ${CSM_PATH}
check_return_exit $? 2 "Test Case 7a: Calling findWeightedErrors.py without --errormap required field"

rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "          BDS - python_scripts Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

rm -f ${TEMP_LOG}

exit 0
