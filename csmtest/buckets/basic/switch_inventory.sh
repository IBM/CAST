#================================================================================
#   
#    buckets/basic/switch_inventory.sh
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

# Basic Bucket for Switch Inventory API Testing
# switch attributes query, switch attributes  update, switch attributes query history, switch attributes query details

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/switch_inventory.log
TEMP_LOG=${LOG_PATH}/buckets/basic/switch_inventory_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/switch_inventory_flags.log
SQL_FILE=${SQL_DIR}/switch.sql

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "          Starting Switch Inventory API Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Add dummy switch data in to csmdb
su -c "psql -d csmdb -f ${SQL_FILE}" postgres > ${TEMP_LOG} 2>&1
su -c "psql -d csmdb -c 'select * from csm_switch ;'" postgres | grep switch_001 >> ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1: Add dummy switch data in to csmdb"

# Test Case 2: Calling switch_attributes_query
${CSM_PATH}/csm_switch_attributes_query -s switch_001 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Calling switch_attributes_query"

# Test Case 3: Validating switch_attributes_query output
check_all_output "switch_001"
check_return_exit $? 0 "Test Case 3: Validating switch_attributes_query output"

# Test Case 4: Calling switch_attributes_update
${CSM_PATH}/csm_switch_attributes_update -s switch_001 -c "Test Comment" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4: Calling switch_attributes_update"

# Test Case 5: Checking switch attributes for test comment
${CSM_PATH}/csm_switch_attributes_query -s switch_001 > ${TEMP_LOG} 2>&1
check_all_output "switch_001" "Test Comment"
check_return_exit $? 0 "Test Case 5: Checking switch attributes for test comment"

# Test Case 6: Calling switch_attributes_query_history
${CSM_PATH}/csm_switch_attributes_query_history -s switch_001 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6: Calling switch_attributes_query_history"

# Test Case 7: Validating query history output 
check_all_output "switch_001" "my comment"
check_return_flag_value $? 0 "Test Case 3: switch_attributes_query_history returns invalid data after update"

# Test Case 8: Calling switch_attributes_query_details
${CSM_PATH}/csm_switch_attributes_query_details -s switch_001 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8: Calling switch_attributes_query_details"

# Test Case 9: Validating switch_attributes_query_details output
check_all_output "switch_001" "Test Comment"
check_return_flag_value $? 0 "Test Case 9: Validating switch_attributes_query_details output"

rm -f ${TEMP_LOG}
su -c "psql -d csmdb -c 'DELETE FROM csm_switch ;'" postgres > /dev/null
su -c "psql -d csmdb -c 'DELETE FROM csm_switch_history ;'" postgres > /dev/null

echo "------------------------------------------------------------" >> ${LOG}
echo "             Basic Switch Inventory API Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
