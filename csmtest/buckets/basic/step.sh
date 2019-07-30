#================================================================================
#   
#    buckets/basic/step.sh
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

# Bucket for basic CSM allocation step testings
# 1 cycle of the allocation step command line APIs
# 
# csm_allocation_step_begin   
# csm_allocation_step_query_active_all
# csm_allocation_step_end
# csm_allocation_step_query
# csm_allocation_step_query_details

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
	. "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
	echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../../csm_test.cfg", exitting."
	exit 1
fi

# Local variables used by this script
LOG=${LOG_PATH}/buckets/basic/step.log
TEMP_LOG=${LOG_PATH}/buckets/basic/step_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/step_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "             Starting Step Basic Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Calling csm_allocation_create
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Calling csm_allocation_create" 

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 2: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Calling csm_allocation_query_active_all"

# Test Case 3: Calling csm_allocation_step_begin
${CSM_PATH}/csm_allocation_step_begin --step_id 1 --allocation_id ${allocation_id} --executable "my_executable" --working_directory "my_workding_directory" --argument "my_argument" --environment_variable "my_environment_variable" --num_processors 1 --num_gpus 1 --projected_memory 512 --num_tasks 1 --compute_nodes ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: Calling csm_allocation_step_begin"

# Test Case 4: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 4: Calling csm_allocation_step_query_active_all"

# Test Case 5: Calling csm_allocation_step_end
${CSM_PATH}/csm_allocation_step_end -a ${allocation_id} -s 1 -e 1 -E "error" -c "cpu_good" -t 0.0 -T 1.5 -n "t_num_threads_good" -G "gpu_s_good" -m "mem_sts_good" -M 512 -i "io_sts_good" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5: Calling csm_allocation_step_end"

# Test Case 6: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "No matching records found."
check_return_flag_value $? 0 "Test Case 6: Calling csm_allocation_step_query_active_all"

# Test Case 7: Calling csm_allocation_step_query
${CSM_PATH}/csm_allocation_step_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 7: Calling csm_allocation_step_query"

# Test Case 8: Calling csm_allocation_step_query_details
${CSM_PATH}/csm_allocation_step_query_details -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 8: Calling csm_allocation_step_query_details"

# Test Case 9: Calling csm_allocation_delete
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 9: Calling csm_allocation_delete"

# Test Case 10: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_all_output "No matching records found"
check_return_flag_value $? 0 "Test Case 10: Calling csm_allocation_query_active_all"

# Remove temp log
rm -f ${TEMP_LOG}

if [ -z ${FLAGS+x} ];
then
	echo "------------------------------------------------------------" >> ${LOG}
	echo "             RAS Basic Bucket PASSED" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	echo "Additional Flags:" >> ${LOG}
	echo -e "${FLAGS}" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	exit 0
else
	echo "------------------------------------------------------------" >> ${LOG}
	echo "             RAS Basic Bucket FAILED" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	echo "Additional Flags:" >> ${LOG}
	echo -e "${FLAGS}" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	exit 1
fi
