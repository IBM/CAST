#================================================================================
#   
#    buckets/basic/bb.sh
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

# Bucket for BB API Testing
# vg query, lv query, lv delete, vg delete

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/bb.log
TEMP_LOG=${LOG_PATH}/buckets/basic/bb_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/bb_flags.log
#SQL_FILE=${SQL_DIR}/ssd.sql

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting BB API Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Adding ssd dummy data in to csmdb
su -c "psql -d csmdb -c \"INSERT INTO csm_ssd (serial_number, node_name, update_time, size, wear_lifespan_used, wear_total_bytes_written, wear_total_bytes_read, wear_percent_spares_remaining) VALUES('ssd_01','${SINGLE_COMPUTE}','now',500,1,1,1,1) ;\"" postgres > ${TEMP_LOG}
su -c "psql -d csmdb -c 'select * from csm_ssd ;'" postgres | grep ssd_01 >> ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1: Adding ssd dummy data in to csmdb"

# Test Case 2: Calling csm_bb_vg_create
${CSM_PATH}/csm_bb_vg_create -a 500 -n ${SINGLE_COMPUTE} -s ssd_01 -c t -S 500 -t 500 -V vg_01 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Calling csm_bb_vg_create"

# Test Case 3: Calling csm_bb_vg_query using vg name
${CSM_PATH}/csm_bb_vg_query -V vg_01 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: Calling csm_bb_vg_query using vg name"

# Test Case 4: Validating csm_bb_vg_query output
check_all_output "Total_Records: 1" "vg_name:.*vg_01" "node_name:.*${SINGLE_COMPUTE}" "total_size:.*500" "available_size:.*500" "scheduler:.*t"
check_return_flag_value $? 0 "Test Case 4: Validating csm_bb_vg_query output"

# Test Case 5: Calling csm_bb_vg_query using node name
${CSM_PATH}/csm_bb_vg_query -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5: Calling csm_bb_vg_query using node name"

# Test Case 6: Validating csm_bb_vg_query output
check_all_output "Total_Records: 1" "vg_name:.*vg_01" "node_name:.*${SINGLE_COMPUTE}" "total_size:.*500" "available_size:.*500" "scheduler:.*t"
check_return_exit $? 0 "Test Case 6: Validating csm_bb_vg_query output"

# Create allocation for LV
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`

# Test Case 7: Create lv_01
${CSM_PATH}/csm_bb_lv_create -a ${allocation_id} -c 500 -f mount -F type -l lv_01 -n ${SINGLE_COMPUTE} -s C -V vg_01 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7: Create lv_01"

# Test Case 8: Calling csm_bb_lv_query using allocation ID
${CSM_PATH}/csm_bb_lv_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8: Calling csm_bb_lv_query using allocation ID"

# Test Case 9: Validating csm_bb_lv_query output
check_all_output "logical_volume_name: lv_01" "allocation_id:       ${allocation_id}" "node_name:           ${SINGLE_COMPUTE}" "vg_name:             vg_01"
check_return_flag_value $? 0 "Test Case 9: Validating csm_bb_lv_query_output"

# Test Case 10: Calling csm_bb_lv_query using logical volume name
${CSM_PATH}/csm_bb_lv_query -L lv_01 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 10: Calling csm_bb_lv_query using logical volume name"

# Test Case 11: Validating csm_bb_lv_query output
check_all_output "logical_volume_name: lv_01" "allocation_id:       ${allocation_id}" "node_name:           ${SINGLE_COMPUTE}" "vg_name:             vg_01"
check_return_flag_value $? 0 "Test Case 11: Validating csm_bb_lv_query output"

# Test Case 12: Calling csm_bb_lv_query using node name
${CSM_PATH}/csm_bb_lv_query -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 12: Calling csm_bb_lv_query using node name"

# Test Case 13: Validating csm_bb_lv_query output
check_all_output "logical_volume_name: lv_01" "allocation_id:       ${allocation_id}" "node_name:           ${SINGLE_COMPUTE}" "vg_name:             vg_01"
check_return_flag_value $? 0 "Test Case 13: Validating csm_bb_lv_query output"

# Test Case 14: Calling csm_bb_lv_delete
${CSM_PATH}/csm_bb_lv_delete -a ${allocation_id} -l lv_01 -n ${SINGLE_COMPUTE} -r 1 -w 1 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 14: Calling csm_bb_lv_delete"

# Test Case 15: Calling csm_allocation_delete
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 15: calling csm_allocation_delete"

# Test Case 16: Cascading allocation/LV delete - create allocation
${CSM_PATH}/csm_allocation_create -j 1 -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
allocation_id=`grep allocation_id ${TEMP_LOG} | awk -F': ' '{print $2}'`
check_return_exit $? 0 "Test Case 16: Cascading allocation/LV delete - create allocation"

# Test Case 16: Cascading allocation/LV delete - create lv_02
${CSM_PATH}/csm_bb_lv_create -a ${allocation_id} -c 500 -f mount -F type -l lv_02 -n ${SINGLE_COMPUTE} -s C -V vg_01 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 16: Cascading allocation/LV delete - create lv_02"

# Test Case 16: Cascading allocation/LV delete - query lv_02
${CSM_PATH}/csm_bb_lv_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 16: Cascading allocation/LV delete - query lv_02"

# Test Case 16: Cascading allocation/LV delete - delete allocation
${CSM_PATH}/csm_allocation_delete -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 16: Cascading allocation/LV delete - delete allocation"

# Test Case 16: Cascading allocation/LV delete - verify lv_02 deleted
${CSM_PATH}/csm_bb_lv_query -a ${allocation_id} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 4 "Test Case 16: Cascading allocation/LV delete - verify lv_02 deleted"

# Test Case 16; Cascading allocation/LV delete - verify lv_02 in history table
su -c "psql -d csmdb -c 'select * from csm_lv_history ;'" postgres > ${TEMP_LOG} 2>&1
check_all_output "lv_02 .* D"
check_return_flag_value $? 0 "Test Case 16; Cascading allocation/LV delete - verify lv_02 in history table"

# Test Case 17: Calling csm_bb_vg_delete
${CSM_PATH}/csm_bb_vg_delete -n ${SINGLE_COMPUTE} -V vg_01 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 17: Calling csm_bb_vg_delete"

rm -f ${TEMP_LOG}
su -c "psql -d csmdb -c 'DELETE FROM csm_ssd ;'" postgres > /dev/null

echo "------------------------------------------------------------" >> ${LOG}
echo "           Basic Burst Buffer  Bucket Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
