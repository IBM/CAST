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

# Local variables for the CSM DB record checks
DB_NAME="csmdb"
DB_USER="csmdb"

# Local variables for step ids passed into the functions
sid_1=1
sid_2=2
sid_3=3

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
check_return_exit $? 0 "Test Case 1:  Calling csm_allocation_create                    (allocation_id: created     )" 

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 2: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  Calling csm_allocation_query_active_all"

# Test Case 3: Calling csm_allocation_step_begin
${CSM_PATH}/csm_allocation_step_begin --step_id 1 --allocation_id ${allocation_id} --executable "my_executable" --working_directory "my_workding_directory" --argument "my_argument" --environment_variable "my_environment_variable" --num_processors 1 --num_gpus 1 --projected_memory 512 --num_tasks 1 --compute_nodes ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3:  Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: 1)"

# Test Case 4: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 4:  Calling csm_allocation_step_query_active_all"

# Test Case 5: Calling csm_allocation_step_end
${CSM_PATH}/csm_allocation_step_end -a ${allocation_id} -s 1 -e 1 -E "error" -c "cpu_good" -t 0.0 -T 1.5 -n "t_num_threads_good" -G "gpu_s_good" -m "mem_sts_good" -M 512 -i "io_sts_good" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5:  Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: 1)"

# Test Case 6: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "No matching records found."
check_return_flag_value $? 0 "Test Case 6:  Calling csm_allocation_step_query_active_all"

# Test Case 7: Calling csm_allocation_step_query
${CSM_PATH}/csm_allocation_step_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 7:  Calling csm_allocation_step_query"

# Test Case 8: Calling csm_allocation_step_query_details
${CSM_PATH}/csm_allocation_step_query_details -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 8:  Calling csm_allocation_step_query_details"

# Test Case 9: Check csm_allocation_step_query_details record count
rcount_0=`grep Total_Records: ${TEMP_LOG} | awk -F': ' '{print $2}'`
csm_step_hist_ct_0=`psql -qtA -U $DB_USER -d $DB_NAME -c "select count(*) from csm_step_history where step_id=1 and allocation_id=${allocation_id}"`

    if [[ "$csm_step_hist_ct_0" -eq "$rcount_0" ]]; then
        check_return_flag_value $? 0 "Test Case 9:  Check   csm_allocation_step_query_details        (Total_Records:${rcount_0})"
    else
        check_return_exit $? 0 "Test Case 9: Check   csm_allocation_step_query_details        (Total_Records:${rcount_0})"
    fi

# Test Case 10: Calling csm_allocation_delete
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 10:  Calling csm_allocation_delete"

# Test Case 11: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_all_output "No matching records found"
check_return_flag_value $? 0 "Test Case 11: Calling csm_allocation_query_active_all"

# Additional tests to handle multiple records returned from csm_allocation_step_query_details
echo "------------------------------------------------------------" >> ${LOG}
echo "             Additional Basic Bucket Tests Set #2"            >> ${LOG}
echo "  CSM Allocation Step Query Details (multiple same steps)"    >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

#-----------------------------------------------------------------
# Functions for allocation_step_begin & allocation_step_end
#-----------------------------------------------------------------

function allocation_step_begin()
{
${CSM_PATH}/csm_allocation_step_begin --step_id $@ --allocation_id ${allocation_id} --executable "my_executable" --working_directory "my_workding_directory" --argument "my_argument" --environment_variable "my_environment_variable" --num_processors 1 --num_gpus 1 --projected_memory 512 --num_tasks 1 --compute_nodes ${SINGLE_COMPUTE}
}

function allocation_step_end()
{
${CSM_PATH}/csm_allocation_step_end -a ${allocation_id} -s $@ -e 1 -E "error" -c "cpu_good" -t 0.0 -T 1.5 -n "t_num_threads_good" -G "gpu_s_good" -m "mem_sts_good" -M 512 -i "io_sts_good"
}

# Test Case 12: Calling csm_allocation_create
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 12: Calling csm_allocation_create                    (allocation_id: created     )"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 13: Calling csm_allocation_step_begin
allocation_step_begin ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 13: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 14: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 14: Calling csm_allocation_step_query_active_all"

# Test Case 15: Calling csm_allocation_step_end
allocation_step_end ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 15: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 16: Calling csm_allocation_step_begin
allocation_step_begin ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 16: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 17: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 17: Calling csm_allocation_step_query_active_all"

# Test Case 18: Calling csm_allocation_step_end
allocation_step_end ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 18: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 19: Calling csm_allocation_step_begin
allocation_step_begin ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 19: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 20: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 20: Calling csm_allocation_step_query_active_all"

# Test Case 21: Calling csm_allocation_step_end
allocation_step_end ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 21: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_1})"

#-------------------------------------------------------------------------------------------------------------------------
# Creating an additional step to handle the edge case.
#-------------------------------------------------------------------------------------------------------------------------

# Test Case 22: Calling csm_allocation_step_begin
allocation_step_begin ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 22: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 23: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 23: Calling csm_allocation_step_query_active_all"

# Test Case 24: Calling csm_allocation_step_end
allocation_step_end ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 24: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 25: Calling csm_allocation_step_begin
allocation_step_begin ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 25: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 26: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 26: Calling csm_allocation_step_query_active_all"

# Test Case 27: Calling csm_allocation_step_end
allocation_step_end ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 27: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 28: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "No matching records found."
check_return_flag_value $? 0 "Test Case 28: Calling csm_allocation_step_query_active_all"

# Test Case 29: Calling csm_allocation_step_query
${CSM_PATH}/csm_allocation_step_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 29: Calling csm_allocation_step_query"

# Test Case 30: Calling csm_allocation_step_query_details
${CSM_PATH}/csm_allocation_step_query_details -a ${allocation_id} -s 1  > ${TEMP_LOG} 2>&1 
check_return_flag_value $? 1 "Test Case 30: Calling csm_allocation_step_query_details        (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 31: Check csm_allocation_step_query_details record count
rcount_1=`grep Total_Records: ${TEMP_LOG} | awk -F': ' '{print $2}'`
csm_step_hist_ct_1=`psql -qtA -U $DB_USER -d $DB_NAME -c "select count(*) from csm_step_history where step_id=1 and allocation_id=${allocation_id}"`

    if [[ "$csm_step_hist_ct_1" -eq "$rcount_1" ]]; then
        check_return_flag_value $? 0 "Test Case 31: Check   csm_allocation_step_query_details        (Total_Records:${rcount_1})"
    else
        check_return_exit $? 0 "Test Case 31: Check   csm_allocation_step_query_details        (Total_Records:${rcount_1})"
    fi

# Test Case 32: Calling csm_allocation_step_query_details
${CSM_PATH}/csm_allocation_step_query_details -a ${allocation_id} -s 2  > ${TEMP_LOG} 2>&1 
check_return_flag_value $? 1 "Test Case 32: Calling csm_allocation_step_query_details        (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 33: Check csm_allocation_step_query_details record count
rcount_1a=`grep Total_Records: ${TEMP_LOG} | awk -F': ' '{print $2}'`
csm_step_hist_ct_1a=`psql -qtA -U $DB_USER -d $DB_NAME -c "select count(*) from csm_step_history where step_id=2 and allocation_id=${allocation_id}"`

    if [[ "$csm_step_hist_ct_1a" -eq "$rcount_1a" ]]; then
        check_return_flag_value $? 0 "Test Case 33: Check   csm_allocation_step_query_details        (Total_Records:${rcount_1a})"
    else
        check_return_exit $? 0 "Test Case 33: Check   csm_allocation_step_query_details        (Total_Records:${rcount_1a})"
    fi

# Test Case 34: Calling csm_allocation_delete
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 34: Calling csm_allocation_delete"

# Test Case 35: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_all_output "No matching records found"
check_return_flag_value $? 0 "Test Case 35: Calling csm_allocation_query_active_all"

# Additional tests #3 to handle multiple records returned from csm_allocation_step_query_details
echo "------------------------------------------------------------" >> ${LOG}
echo "             Additional Basic Bucket Tests Set #3"            >> ${LOG}
echo "  CSM Allocation Step Query Details (multiple same steps)"    >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 36: Calling csm_allocation_create
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 36: Calling csm_allocation_create                    (allocation_id: created     )"

# Grab & Store Allocation ID from csm_allocation_create.log
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 37: Calling csm_allocation_step_begin
allocation_step_begin ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 37: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 38: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 38: Calling csm_allocation_step_query_active_all"

# Test Case 39: Calling csm_allocation_step_end
allocation_step_end ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 39: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 40: Calling csm_allocation_step_begin
allocation_step_begin ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 40: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 41: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 41: Calling csm_allocation_step_query_active_all"

# Test Case 42: Calling csm_allocation_step_end
allocation_step_end ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 42: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 43: Calling csm_allocation_step_begin
allocation_step_begin ${sid_3} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 43: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_3})"

# Test Case 44: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 44: Calling csm_allocation_step_query_active_all"

# Test Case 45: Calling csm_allocation_step_end
allocation_step_end ${sid_3} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 45: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_3})"

# Test Case 46: Calling csm_allocation_step_begin
allocation_step_begin ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 46: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 47: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 47: Calling csm_allocation_step_query_active_all"

# Test Case 48: Calling csm_allocation_step_end
allocation_step_end ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 48: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 49: Calling csm_allocation_step_begin
allocation_step_begin ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 49: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 50: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 50: Calling csm_allocation_step_query_active_all"

# Test Case 51: Calling csm_allocation_step_end
allocation_step_end ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 51: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 52: Calling csm_allocation_step_begin
allocation_step_begin ${sid_3} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 52: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_3})"

# Test Case 53: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 53: Calling csm_allocation_step_query_active_all"

# Test Case 54: Calling csm_allocation_step_end
allocation_step_end ${sid_3} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 54: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_3})"

# Test Case 55: Calling csm_allocation_step_begin
allocation_step_begin ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 55: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 56: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 56: Calling csm_allocation_step_query_active_all"

# Test Case 57: Calling csm_allocation_step_end
allocation_step_end ${sid_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 57: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 58: Calling csm_allocation_step_begin
allocation_step_begin ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 58: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 59: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 59: Calling csm_allocation_step_query_active_all"

# Test Case 60: Calling csm_allocation_step_end
allocation_step_end ${sid_2} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 60: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 61: Calling csm_allocation_step_begin
allocation_step_begin ${sid_3} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 61: Calling csm_allocation_step_begin                (allocation_id: ${allocation_id} step_id: ${sid_3})"

# Test Case 62: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 62: Calling csm_allocation_step_query_active_all"

# Test Case 63: Calling csm_allocation_step_end
allocation_step_end ${sid_3} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 63: Calling csm_allocation_step_end                  (allocation_id: ${allocation_id} step_id: ${sid_3})"

# Test Case 64: Calling csm_allocation_step_query_active_all
${CSM_PATH}/csm_allocation_step_query_active_all -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_all_output "No matching records found."
check_return_flag_value $? 0 "Test Case 64: Calling csm_allocation_step_query_active_all"

# Test Case 65: Calling csm_allocation_step_query
${CSM_PATH}/csm_allocation_step_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 65: Calling csm_allocation_step_query"

# Test Case 66: Calling csm_allocation_step_query_details
${CSM_PATH}/csm_allocation_step_query_details -a ${allocation_id} -s 1  > ${TEMP_LOG} 2>&1 
check_return_flag_value $? 1 "Test Case 66: Calling csm_allocation_step_query_details        (allocation_id: ${allocation_id} step_id: ${sid_1})"

# Test Case 67: Check csm_allocation_step_query_details record count
rcount_2=`grep Total_Records: ${TEMP_LOG} | awk -F': ' '{print $2}'`
csm_step_hist_ct_2=`psql -qtA -U $DB_USER -d $DB_NAME -c "select count(*) from csm_step_history where step_id=1 and allocation_id=${allocation_id}"`
    
    if [[ "$csm_step_hist_ct_2" -eq "$rcount_2" ]]; then
        check_return_flag_value $? 0 "Test Case 67: Check   csm_allocation_step_query_details        (Total_Records:${rcount_2})"
    else
        check_return_exit $? 0 "Test Case 67: Check   csm_allocation_step_query_details        (Total_Records:${rcount_2})"
    fi

# Test Case 68: Calling csm_allocation_step_query_details
${CSM_PATH}/csm_allocation_step_query_details -a ${allocation_id} -s 2  > ${TEMP_LOG} 2>&1 
check_return_flag_value $? 1 "Test Case 68: Calling csm_allocation_step_query_details        (allocation_id: ${allocation_id} step_id: ${sid_2})"

# Test Case 69: Check csm_allocation_step_query_details record count
rcount_3=`grep Total_Records: ${TEMP_LOG} | awk -F': ' '{print $2}'`
csm_step_hist_ct_3=`psql -qtA -U $DB_USER -d $DB_NAME -c "select count(*) from csm_step_history where step_id=2 and allocation_id=${allocation_id}"`
    
    if [[ "$csm_step_hist_ct_3" -eq "$rcount_3" ]]; then
        check_return_flag_value $? 0 "Test Case 69: Check   csm_allocation_step_query_details        (Total_Records:${rcount_3})"
    else
        check_return_exit $? 0 "Test Case 69: Check   csm_allocation_step_query_details        (Total_Records:${rcount_3})"
    fi

# Test Case 70: Calling csm_allocation_step_query_details
${CSM_PATH}/csm_allocation_step_query_details -a ${allocation_id} -s 3  > ${TEMP_LOG} 2>&1 
check_return_flag_value $? 1 "Test Case 70: Calling csm_allocation_step_query_details        (allocation_id: ${allocation_id} step_id: ${sid_3})"

# Test Case 71: Check csm_allocation_step_query_details record count
rcount_4=`grep Total_Records: ${TEMP_LOG} | awk -F': ' '{print $2}'`
csm_step_hist_ct_4=`psql -qtA -U $DB_USER -d $DB_NAME -c "select count(*) from csm_step_history where step_id=3 and allocation_id=${allocation_id}"`
    
    if [[ "$csm_step_hist_ct_4" -eq "$rcount_4" ]]; then
        check_return_flag_value $? 0 "Test Case 71: Check   csm_allocation_step_query_details        (Total_Records:${rcount_4})"
    else
        check_return_exit $? 0 "Test Case 71: Check   csm_allocation_step_query_details        (Total_Records:${rcount_4})"
    fi

# Test Case 72: Calling csm_allocation_delete
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 72: Calling csm_allocation_delete"

# Test Case 73: Calling csm_allocation_query_active_all
${CSM_PATH}/csm_allocation_query_active_all > ${TEMP_LOG} 2>&1
check_all_output "No matching records found"
check_return_flag_value $? 0 "Test Case 73: Calling csm_allocation_query_active_all"

# Remove temp log
rm -f ${TEMP_LOG}

if [ -z ${FLAGS+x} ];
then
	echo "------------------------------------------------------------" >> ${LOG}
	echo "             Step Basic Bucket PASSED" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	echo "Additional Flags:" >> ${LOG}
	echo -e "${FLAGS}" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	exit 0
else
	echo "------------------------------------------------------------" >> ${LOG}
	echo "             Step Basic Bucket FAILED" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	echo "Additional Flags:" >> ${LOG}
	echo -e "${FLAGS}" >> ${LOG}
	echo "------------------------------------------------------------" >> ${LOG}
	exit 1
fi
