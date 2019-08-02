#================================================================================
#   
#    buckets/basic/ib_inventory.sh
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

# Basic Bucket for IB Inventory API Testing
# ib cable query, ib cable update, ib cable query history

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/ib_inventory.log
TEMP_LOG=${LOG_PATH}/buckets/basic/ib_inventory_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/ib_inventory_flags.log
SQL_FILE=${SQL_DIR}/ib.sql

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "             Starting IB Inventory API Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Add dummy IB cable data in to csmdb
su -c "psql -d csmdb -f ${SQL_FILE}" postgres > ${TEMP_LOG} 2>&1
su -c "psql -d csmdb -c 'select * from csm_ib_cable ;'" postgres | grep ib_001 >> ${TEMP_LOG}
check_return_exit $? 0 "Test Case 1: Adding Dummy IB cable data in to csmdb"

# Test Case 2: Calling ib_cable_query
${CSM_PATH}/csm_ib_cable_query -s ib_001 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Calling ib_cable_query"

# Test Case 3: Validating returned data
check_all_output "ib_001"
check_return_exit $? 0 "Test Case 3: Validating returned data"

# Test Case 4: Calling ib_cable_update
${CSM_PATH}/csm_ib_cable_update -s ib_001 -c "Test Comment" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4: Calling ib_cable_update"

# Test Case 5: Checking DB entry for test comment
${CSM_PATH}/csm_ib_cable_query -s ib_001 > ${TEMP_LOG} 2>&1
check_all_output "ib_001" "Test Comment"
check_return_exit $? 0 "Test Case 5: Checking DB entry for test comment"

# Test Case 4: Calling ib_cable_query_history
${CSM_PATH}/csm_ib_cable_query_history -s ib_001 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4: Calling ib_cable_query_history"

# Test Case 5: Validating query history output
check_all_output "ib_001" 
check_return_flag_value $? 0 "Test Case 3: ib_cable_query_history returns invalid data after update" 

rm -f ${TEMP_LOG}
su -c "psql -d csmdb -c 'DELETE FROM csm_ib_cable ;'" postgres > /dev/null

echo "------------------------------------------------------------" >> ${LOG}
echo "             Basic IB Inventory API Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0

