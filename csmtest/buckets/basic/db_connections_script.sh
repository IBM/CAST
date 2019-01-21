#================================================================================
#   
#    buckets/basic/db_connection.sh
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

#--------------------------------------------
# Bucket for CSM DB utility scripts
# DB Connections Script
# list_all_current_connections (if exist with all users)
# list_specifc_user_connection(s)
# kill all connection (each process) with no
# kill specific user user(s) session(s)
# kill specific PID session(s)
#--------------------------------------------

OPTERR=0

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

LOG=${LOG_PATH}/buckets/basic/db_connections_script.log
TEMP_LOG=${LOG_PATH}/buckets/basic/db_connections_script_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/db_connections_script_flag.log

DB_PATH=/opt/ibm/csm/db
DB_NAME="fvttestdb2"
DB_USER="fvttestdb2"

#---------------------------------------------
# Local Test Variables
#---------------------------------------------
#LOG=/tmp/db_connections_script.log
#TEMP_LOG=/tmp/db_connections_script_tmp.log
#FLAG_LOG=/tmp/db_connections_script_flag.log


if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting DB Connections Script Bucket" >> ${LOG}
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

#psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME 2>>/dev/null
su -c "psql -d $DB_NAME -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  Checking $DB_NAME exists"
#echo "Test Case 2:  Checking $DB_NAME exists"

#------------------------------------------------------------------------------
# Test Case 3: Create fvttestdb user
#------------------------------------------------------------------------------
su -c "psql -d $DB_NAME -c \"CREATE USER $DB_USER\"" postgres > ${TEMP_LOG} 2>&1
#psql -t -q -d $DB_NAME -c "CREATE USER $DB_USER" 2>&1
check_return_exit $? 0 "Test Case 3:  Creating FVT test DB user: $DB_USER"
#echo "Test Case 3:  Creating FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 4: Grant Privileges to fvttestdb user
#------------------------------------------------------------------------------
psql -t -q -U postgres -d postgres -c "GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO $DB_USER;" 2>&1
check_return_exit $? 0 "Test Case 4:  Granting privileges to FVT test DB user: $DB_USER"
#echo "Test Case 4:  Granting privileges to FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 5: Checking fvttestdb user exists
#------------------------------------------------------------------------------

psql -U postgres -t -c '\du' | cut -d \| -f 1 | grep -qw $DB_USER
check_return_exit $? 0 "Test Case 5:  Check the existence of FVT test DB user: $DB_USER"
#echo "Test Case 5:  Check the existence of FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 6: Calling csm_db_connections_script.sh -l
# Basic check to see if the functionality of the list of DB's connected
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_connections_script.sh -l > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 6:  Calling csm_db_connections_script.sh -l"
#echo "Test Case 6:  Calling csm_db_connections.sh -l"

#------------------------------------------------------------------------------
# Test Case 7: Calling csm_db_connections_script.sh -l -u
# Basic check to see if the functionality of the list of users connected
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_connections_script.sh -l -u $DB_USER > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Test Case 7:  Calling csm_db_connections_script.sh -l -u $DB_USER"
#echo "Test Case 7:  Calling csm_db_connections.sh -l -u $DB_USER"

#------------------------------------------------------------------------------
# Test Case 8: Calling csm_db_connections_script.sh -k $DB_NAME
# Kill All sessions with the "NO" command.
# 1. Count all DB connections
# 2. Run the -k command with "NO" option
# 3. Count all the "NO's" in the the temp file?
#------------------------------------------------------------------------------

db_count=`psql -t -q -d $DB_NAME -c "select count(*) from pg_stat_activity" postgres`
printf '%-3d\n' $db_count > ${TEMP_LOG} 2>&1
db_count_2=`cat "${TEMP_LOG}"`

    #----------------------------------
    # First process the "NO" responses
    #----------------------------------
    yes 'n\'| ${DB_PATH}/csm_db_connections_script.sh -k > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 8:  Calling csm_db_connections_script.sh -k (all connections) with (NO) response"
    #echo "Test Case 8:  Calling csm_db_connections.sh -k (all connections) with (NO) response"
    res_check=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify all "NO" responses
    #---------------------------------
    db_count_after=`psql -t -q -d $DB_NAME -c "select count(*) from pg_stat_activity" postgres`
    
    printf '%-3d\n' $db_count_after > ${TEMP_LOG} 2>&1
    db_count_after_2=`cat "${TEMP_LOG}"`

        if [[ "$db_count_2" -eq "$res_check" ]] && [[ "$db_count_2" -eq "$db_count_after_2" ]]; then
            #echo "Test Case 9:  Verify that all connections still exist after the (NO) response"
            check_return_exit $? 0 "Test Case 9:  Verify that all connections still exist after the (NO) response"
        fi

#------------------------------------------------------------------------------
# Test Case 10: Calling csm_db_connections_script.sh -k with $DB_NAME
# Kill All sessions with the specified DB name.
# 1. Count all DB connections
# 2. Run the -k command with "NO" option
# 3. Count all the "NO's" in the the temp file?
#------------------------------------------------------------------------------

fvt_db_conn=`psql -qtA -P  pager=off -X -U $DB_USER -d $DB_NAME -v ON_ERROR_STOP=1 << THE_END 2>/dev/null &
SET client_min_messages TO WARNING;
select pg_sleep(20);
THE_END` &
fvt_db_count=`psql -t -q -d postgres -c "select count(*) from pg_stat_activity where datname = '$DB_USER'" postgres` 2>&1
printf '%-3d\n' $fvt_db_count > ${TEMP_LOG} 2>&1
fvt_db_count_2=`cat "${TEMP_LOG}"`

    #---------------------------------
    # First process the "NO" response
    #---------------------------------
    if [[ "$fvt_db_count_2" -gt 0 ]]; then
        yes 'n\'| ${DB_PATH}/csm_db_connections_script.sh -k -u $DB_USER > ${TEMP_LOG} 2>/dev/null
    fi
        check_return_exit $? 0 "Test Case 10: Calling csm_db_connections_script.sh -k -u $DB_USER with (NO) response"
        #echo "Test Case 10: Calling csm_db_connections_script.sh -k -u $DB_USER with (NO) response"

    #-----------------------------------
    # Verify if user is still connected
    #-----------------------------------
    fvt_db_user_exists=`psql -t -q -d postgres -c "select usename from pg_stat_activity \
    where datname = '$DB_USER'" postgres` 2>&1
        if [[ "$fvt_db_user_exists" -eq "$DB_USER" ]]; then
            #echo "Test Case 11: Verify if $DB_USER is still connected"
            check_return_exit $? 0 "Test Case 11: Verify if $DB_USER is still connected"
        fi

    #-----------------------------------
    # Second process the "YES" response
    #-----------------------------------
    if [[ "$fvt_db_count_2" -gt 0 ]]; then
        yes | ${DB_PATH}/csm_db_connections_script.sh -k -u $DB_USER > ${TEMP_LOG} 2>/dev/null
    fi
        check_return_exit $? 0 "Test Case 12: Calling csm_db_connections_script.sh -k -u $DB_USER"
        #echo "Test Case 12: Calling csm_db_connections_script.sh -k -u $DB_USER"

    #------------------------------------
    # Verify if connection is terminated
    #------------------------------------
    killed_fvt_db_count=`psql -t -q -d postgres -c "select count(*) from pg_stat_activity where datname = '$DB_NAME'" postgres`
    printf '%-3d\n' $killed_fvt_db_count > ${TEMP_LOG} 2>&1
    killed_fvt_db_count_2=`cat "${TEMP_LOG}"`

        if [[ "$killed_fvt_db_count_2" -eq 0 ]]; then
            #echo "Test Case 13: Verify $DB_USER has been terminated"
            check_return_exit $? 0 "Test Case 13: Verify $DB_USER has been terminated"
        fi

#------------------------------------------------------------------------------
# Test Case 14: Calling csm_db_connections_script.sh -k with PID
# Kill user session with the specified PID number.
# 1. Count all DB connections
# 2. Run the -k command with specific PID number
# 3. Check to see if specific session was terminated successfully.
#------------------------------------------------------------------------------

fvt_db_user=`psql -qtA -P  pager=off -X -U $DB_USER -d $DB_NAME -v ON_ERROR_STOP=1 << THE_END 2>/dev/null &
SET client_min_messages TO WARNING;
select pg_sleep(20);
THE_END` &
fvt_db_user_2=`psql -t -q -d postgres -c "select pid from pg_stat_activity where datname = '$DB_USER'" postgres` 2>&1
printf '%-3d\n' $fvt_db_user_2 > ${TEMP_LOG} 2>&1
fvt_db_user_2a=`cat "${TEMP_LOG}"`

    #---------------------------------
    # First process the "NO" response
    #---------------------------------
    if [[ "$fvt_db_user_2a" -gt 0 ]]; then
        yes 'n\'| ${DB_PATH}/csm_db_connections_script.sh -k -p $fvt_db_user_2a > ${TEMP_LOG} 2>/dev/null
    fi
        
        check_return_exit $? 0 "Test Case 14: Calling csm_db_connections_script.sh -k -p $fvt_db_user_2a with (NO) response"
        #echo "Test Case 14: Calling csm_db_connections_script.sh -k -p $fvt_db_user_2a with (NO) response"

    #-----------------------------------
    # Verify if PID is still connected
    #-----------------------------------
    fvt_db_pid_exists=`psql -t -q -d postgres -c "select pid from pg_stat_activity where datname = '$DB_USER'" postgres` 2>&1
    
    if [[ "$fvt_db_pid_exists" -eq "$fvt_db_user_2" ]]; then
        #echo "Test Case 15: Verify if$fvt_db_user_2 is still connected"
        check_return_exit $? 0 "Test Case 15: Verify if$fvt_db_user_2 is still connected"
    fi

    #-----------------------------------
    # Second process the "YES" response
    #-----------------------------------
    if [[ "$fvt_db_user_2a" -gt 0 ]]; then
        yes | ${DB_PATH}/csm_db_connections_script.sh -k -p $fvt_db_user_2a > ${TEMP_LOG} 2>/dev/null
    fi
        check_return_exit $? 0 "Test Case 16: Calling csm_db_connections_script.sh -k -p $fvt_db_user_2a"
        #echo "Test Case 16: Calling csm_db_connections_script.sh -k -p $fvt_db_user_2a"

    #-----------------------------------
    # Verify if terminated
    #-----------------------------------
    killed_fvt_db_pid_count=`psql -t -q -d postgres -c "select pid from pg_stat_activity where datname = '$DB_NAME'" postgres`
    printf '%-3d\n' $killed_fvt_db_pid_count > ${TEMP_LOG} 2>&1
    killed_fvt_db_pid_count_2=`cat "${TEMP_LOG}"`

        if [[ "$killed_fvt_db_pid_count_2" -eq 0 ]] || [ -z "$killed_fvt_db_pid_count_2" ]; then
            #echo "Test Case 17: Verify $fvt_db_user_2a has been terminated"
            check_return_exit $? 0 "Test Case 17: Verify $fvt_db_user_2a has been terminated"
        fi

#------------------------------------------------------------------------------
# Test Case 18: Calling csm_db_script.sh -d $DB_NAME
#------------------------------------------------------------------------------

echo "y" | ${DB_PATH}/csm_db_script.sh -d $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 18: Calling csm_db_script.sh -d $DB_NAME"
#echo "Test Case 18: Calling csm_db_script.sh -d $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 19: Drop the fvt DB user
#------------------------------------------------------------------------------

psql -t -q -U postgres -d postgres -c "DROP USER $DB_USER;"
check_return_exit $? 0 "Test Case 19: Dropping DB User: $DB_USER"
#echo "Test Case 19: Dropping FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 20: Checking $DB_NAME was deleted
#------------------------------------------------------------------------------

#psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME 2>>/dev/null
su -c "psql -d $DB_NAME -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 2 "Test Case 20: Checking $DB_NAME was deleted"
#check_return_exit $? 1 "Test Case 20: Checking $DB_NAME was deleted"
#echo "Test Case 20: Checking $DB_NAME was deleted"

rm -f ${TEMP_LOG}
#echo "Removed temp file"
echo "------------------------------------------------------------" >> ${LOG}
echo "CSM DB Connections Script Bucket PASSED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
