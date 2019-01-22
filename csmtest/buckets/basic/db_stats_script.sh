#================================================================================
#   
#    buckets/basic/db_stats_script.sh
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

#--------------------------------------------
# Bucket for CSM DB utility scripts
# DB Stats Script
# table_data, index_info, index_analysis
# lock_info, schema_version, db_connections
# db_usernames, postgresql_version,
# arvhive_count, history_delete_count,
# db_vacuum_status
#--------------------------------------------

#------------------------------------------------------------------------------
# Try to source the configuration file to get global configuration variables
#------------------------------------------------------------------------------

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/basic/db_stats_scripts.log
TEMP_LOG=${LOG_PATH}/buckets/basic/db_stats_scripts_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/db_stats_scripts_flag.log

DB_PATH=/opt/ibm/csm/db
DB_NAME="fvttestdb2"

#-----------------------------------------------
# Local Test Variables
#-----------------------------------------------
#LOG=/tmp/db_stats_scripts.log
#TEMP_LOG=/tmp/db_stats_scripts_tmp.log
#FLAG_LOG=/tmp/db_stats_scripts_flag.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting DB Stats Script Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

#------------------------------------------------------------------------------
# Test Case 1: Calling csm_db_script.sh -n $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -n $DB_NAME -x > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1:  Calling csm_db_script.sh -n $DB_NAME"
#echo "Test Case 1:  Calling csm_db_script.sh -n $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 2: Checking $DB_NAME exists
#------------------------------------------------------------------------------

su -c "psql -d $DB_NAME -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  Checking $DB_NAME exists"
#echo "Test Case 2:  Checking $DB_NAME exists"

#------------------------------------------------------------------------------
# Test Case 3: Calling csm_db_stats.sh -t $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -t $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3:  Calling csm_db_stats.sh -t $DB_NAME"
#echo "Test Case 3:  Calling csm_db_stats.sh -t $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 4: Calling csm_db_stats.sh -i $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -i $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4:  Calling csm_db_stats.sh -i $DB_NAME"
#echo "Test Case 4:  Calling csm_db_stats.sh -i $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 5: Calling csm_db_stats.sh -x $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -x $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5:  Calling csm_db_stats.sh -x $DB_NAME"
#echo "Test Case 5:  Calling csm_db_stats.sh -x $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 6: Calling csm_db_stats.sh -l $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -l $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6:  Calling csm_db_stats.sh -l $DB_NAME"
#echo "Test Case 6:  Calling csm_db_stats.sh -l $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 7: Calling csm_db_stats.sh -s $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -s $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7:  Calling csm_db_stats.sh -s $DB_NAME"
#echo "Test Case 7:  Calling csm_db_stats.sh -s $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 8: Calling csm_db_stats.sh -c $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -c $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8:  Calling csm_db_stats.sh -c $DB_NAME"
#echo "Test Case 8:  Calling csm_db_stats.sh -c $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 9: Calling csm_db_stats.sh -u $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -u $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 9:  Calling csm_db_stats.sh -u $DB_NAME"
#echo "Test Case 9:  Calling csm_db_stats.sh -u $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 10: Calling csm_db_stats.sh -v $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -v $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 10: Calling csm_db_stats.sh -v $DB_NAME"
#echo "Test Case 10: Calling csm_db_stats.sh -v $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 11: Calling csm_db_stats.sh -a $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -a $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 11: Calling csm_db_stats.sh -a $DB_NAME"
#echo "Test Case 11: Calling csm_db_stats.sh -a $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 12: Calling csm_db_stats.sh -d $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -d $DB_NAME 1 > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 12: Calling csm_db_stats.sh -d $DB_NAME"
#echo "Test Case 12: Calling csm_db_stats.sh -d $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 13: Calling csm_db_stats.sh -k $DB_NAME
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_stats.sh -k $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 13: Calling csm_db_stats.sh -k $DB_NAME"
#echo "Test Case 13: Calling csm_db_stats.sh -k $DB_NAME"


#------------------------------------------------------------------------------
# Test Case 14: Calling csm_db_script.sh -d $DB_NAME
#------------------------------------------------------------------------------

echo "y" | ${DB_PATH}/csm_db_script.sh -d $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 14: Calling csm_db_script.sh -d $DB_NAME"
#echo "Test Case 14: Calling csm_db_script.sh -d $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 15: Checking $DB_NAME was deleted
#------------------------------------------------------------------------------

su -c "psql -d $DB_NAME -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 2 "Test Case 15: Checking $DB_NAME was deleted"
#echo "Test Case 15: Checking $DB_NAME was deleted"

rm -f ${TEMP_LOG}
#echo "Removed temp file"
echo "------------------------------------------------------------" >> ${LOG}
echo "CSM DB Stats Script Bucket PASSED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
