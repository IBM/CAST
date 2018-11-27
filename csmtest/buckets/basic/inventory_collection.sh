#================================================================================
#   
#    buckets/basic/inventory_collection.sh
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

# Basic Bucket for Inventory Collection Testing
# standalone_ib_and_switch_collection for ib, switch, ib & switch inventory data

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/inventory_collection.log
TEMP_LOG=${LOG_PATH}/buckets/basic/inventory_collection_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/inventory_collection_flags.log
SBIN_PATH=/opt/ibm/csm/sbin

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "          Starting Inventory Collection Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Check for SSL key
ls /etc/ibm/csm/csm_ufm_ssl_key.txt > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Check for SSL key"

# Clean table data
su -c "psql -d csmdb -c 'DELETE FROM csm_ib_cable ;'" postgres > /dev/null 2>&1
su -c "psql -d csmdb -c 'DELETE FROM csm_switch ;'" postgres > /dev/null 2>&1

# Test Case 1: standalone_ib_and_switch_collection fails without SSL key
rm -f /etc/ibm/csm/csm_ufm_ssl_key.txt > ${TEMP_LOG} 2>&1
ls /etc/ibm/csm/csm_ufm_ssl_key.txt > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 2 "Test Case 1: standalone_ib_and_switch_collection fails without SSL key - remove SSL key"
${SBIN_PATH}/standalone_ib_and_switch_collection -c /etc/ibm/csm/csm_master/cfg -t 3 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 1 "Test Case 1: standalone_ib_and_switch_collection fails without SSL key"
cp -f ${SSL_KEY} /etc/ibm/csm/csm_ufm_ssl_key.txt > ${TEMP_LOG} 2>&1
ls /etc/ibm/csm/csm_ufm_ssl_key.txt > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 1: standalone_ib_and_switch_collection fails without SSL key - restore SSL key"

# Test Case 2: standalone_ib_and_switch_collection collects IB cable data
${SBIN_PATH}/standalone_ib_and_switch_collection -c /etc/ibm/csm/csm_master.cfg -t 1 > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 2: standalone_ib_and_switch_collection collects IB cable data"

# Test Case 3: Checking IB cable data populated
su -c "psql -d csmdb -c 'SELECT * FROM csm_ib_cable LIMIT 1;'" postgres | grep "1 row" > ${TEMP_LOG}
check_return_flag $? "Test Case 3: Checking IB cable data populated"

# Clean ib cable table data
su -c "psql -d csmdb -c 'DELETE FROM csm_ib_cable ;'" postgres > /dev/null 2>&1

# Test Case 4: standalone_ib_and_switch_collection collects Switch data
${SBIN_PATH}/standalone_ib_and_switch_collection -c /etc/ibm/csm/csm_master.cfg -t 2 > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 4: standalone_ib_and_switch_collection collects Switch data"

# Test Case 5: Checking Switch data populated
su -c "psql -d csmdb -c 'SELECT * FROM csm_switch LIMIT 1;'" postgres | grep "1 row" > ${TEMP_LOG}
check_return_flag $? "Test Case 5: Checking Switch data populated"

# Clean switch table data
su -c "psql -d csmdb -c 'DELETE FROM csm_switch ;'" postgres > /dev/null 2>&1

# Test Case 6: standalone_ib_and_switch_collection collects IB and Switch data
${SBIN_PATH}/standalone_ib_and_switch_collection -c /etc/ibm/csm/csm_master.cfg -t 3 > ${TEMP_LOG} 2>&1
check_return_flag $? "Test Case 6: standalone_ib_and_switch_collection collects IB and Switch data"

# Test Case 7: Checking IB cable data populated
su -c "psql -d csmdb -c 'SELECT * FROM csm_ib_cable LIMIT 1;'" postgres | grep "1 row" > /dev/null 2>&1
check_return_flag $? "Test Case 7: Checking IB cable data populated"

# Test Case 8: Checking Switch data populated
su -c "psql -d csmdb -c 'SELECT * FROM csm_switch LIMIT 1;'" postgres | grep "1 row" > ${TEMP_LOG}
check_return_flag $? "Test Case 8: Checking Switch data populated"

# File / DB Cleanup
rm -f ${TEMP_LOG}
su -c "psql -d csmdb -c 'DELETE FROM csm_ib_cable ;'" postgres > /dev/null 2>&1
su -c "psql -d csmdb -c 'DELETE FROM csm_switch ;'" postgres > /dev/null 2>&1
rm -f output_file.txt
rm -f temp_file.txt

echo "------------------------------------------------------------" >> ${LOG}
echo "             Basic IB Inventory API Passed" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0

