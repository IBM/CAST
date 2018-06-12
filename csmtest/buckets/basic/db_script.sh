#================================================================================
#   
#    buckets/basic/db_script.sh
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

# Bucket for CSM DB utility scripts
# DB Create, Delete, Stats, Archiving

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/db_scripts.log
TEMP_LOG=${LOG_PATH}/buckets/basic/db_scripts_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/db_scripts_flag.log
DB_PATH=/opt/ibm/csm/db

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting DB Scripts Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Test Case 1: Calling csm_db_script.sh -n fvttestdb
${DB_PATH}/csm_db_script.sh -n fvttestdb -x > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Calling csm_db_script.sh -n fvttestdb"

# Test Case 2: Checking fvttestdb exists
su -c "psql -d fvttestdb -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Checking fvttestdb exists"

# Test Case 3: Calling csm_db_script.sh -d fvttestdb
echo "y" | ${DB_PATH}/csm_db_script.sh -d fvttestdb > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: Calling csm_db_script.sh -d fvttestdb"

# Test Case 4: Checking fvttestdb was deleted
su -c "psql -d fvttestdb -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 2 "Test Case 4: Checking fvttestdb was deleted"

rm -f ${TEMP_LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "CSM DB Scripts Bucket PASSED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
