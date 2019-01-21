#================================================================================
#   
#    buckets/basic/db_ras_type_script.sh
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
# DB RAS type script
# Checks the functionality of the script
# which loads, updates and or removes
# records from the csm_ras_type and
# csm_ras_type_audit tables
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

LOG=${LOG_PATH}/buckets/basic/db_ras_type_script.log
TEMP_LOG=${LOG_PATH}/buckets/basic/db_ras_type_script_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/db_ras_type_script_flag.log

CSV_FILE_1=${SQL_DIR}/csm_ras_type_data_test_1.csv
CSV_FILE_2=${SQL_DIR}/csm_ras_type_data_test_2.csv
CSV_FILE_3=${SQL_DIR}/csm_ras_type_data_test_3.csv

DB_PATH=/opt/ibm/csm/db
DB_NAME="fvttestdb2"
DB_USER="fvttestdb2"

#-------------------------------------------------------
# Local Testing Variables
#-------------------------------------------------------
#LOG=/tmp/db_ras_type_script.log
#TEMP_LOG=/tmp/db_ras_type_script_tmp.log
#FLAG_LOG=/tmp/db_ras_type_script_flag.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting DB RAS Type Script Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

#------------------------------------------------------------------------------
# Test Case 1: Calling csm_db_script.sh -n [$DB_NAME]
# populating DB with RAS csv data
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -n $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1:  Calling csm_db_script.sh -n $DB_NAME"
#echo "Test Case 1:  Calling csm_db_script.sh -n $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 2: Checking [$DB_NAME] exists
#------------------------------------------------------------------------------

#psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME 2>>/dev/null
su -c "psql -d $DB_NAME -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2:  Checking $DB_NAME exists"
#echo "Test Case 2:  Checking $DB_NAME exists"

#------------------------------------------------------------------------------
# Test Case 3: Create [fvttestdb] user
#------------------------------------------------------------------------------
su -c "psql -d $DB_NAME -c \"CREATE USER $DB_USER\"" postgres > ${TEMP_LOG} 2>&1
#psql -t -q -d $DB_NAME -c "CREATE USER $DB_USER" 2>&1
check_return_exit $? 0 "Test Case 3:  Creating FVT test DB user: $DB_USER"
#echo "Test Case 3:  Creating FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 4: Grant Privileges to [fvttestdb] user
#------------------------------------------------------------------------------
psql -t -q -U postgres -d postgres -c "GRANT SELECT, INSERT, UPDATE, DELETE \
ON ALL TABLES IN SCHEMA public TO $DB_USER;" 2>&1
check_return_exit $? 0 "Test Case 4:  Granting privileges to FVT test DB user: $DB_USER"
#echo "Test Case 4:  Granting privileges to FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 5: Checking [fvttestdb] user exists
#------------------------------------------------------------------------------

psql -U postgres -t -c '\du' | cut -d \| -f 1 | grep -qw $DB_USER
check_return_exit $? 0 "Test Case 5:  Check the existence of FVT test DB user: $DB_USER"
#echo "Test Case 5:  Check the existence of FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 6: Calling csm_db_ras_type_script.sh -l [no_db] [no_file]
# Basic check to see if the functionality of the load process
# with no given DB or file name.
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_ras_type_script.sh -l > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Test Case 6:  Calling csm_db_ras_type_script.sh -l with [no_db] [no_file]"
#echo "Test Case 6:  Calling csm_db_ras_type_script.sh -l with [no_db] [no_file]"

#------------------------------------------------------------------------------
# Test Case 7: Calling csm_db_ras_type_script.sh -l [$DB_NAME] with [no_file]
# Basic check to see if the functionality of the load process
# with a valid DB name and no file name (blank)
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Test Case 7:  Calling csm_db_ras_type_script.sh -l $DB_NAME with [no_file]"
#echo "Test Case 7:  Calling csm_db_ras_type_script.sh -l $DB_NAME with [no_file]"

#----------------------------------------------------------------------------------
# Test Case 8: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [non_existing_file]
# Basic check to see if the functionality of the load process
# with a valid DB name and non existing file
#----------------------------------------------------------------------------------

${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME invalid.csv > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8:  Calling csm_db_ras_type_script.sh -l $DB_NAME with [non_existing_file]"
#echo "Test Case 8:  Calling csm_db_ras_type_script.sh -l $DB_NAME with [non_existing_file]"

#----------------------------------------------------------------------------------------
# Test Case 9: Calling csm_db_ras_type_script.sh -l [invalid_db] [csm_ras_type_data.csv]
# Basic check to see if the functionality of the load process
# with a invalid DB name and valid csv file
#----------------------------------------------------------------------------------------

${DB_PATH}/csm_db_ras_type_script.sh -l invalid_db csm_ras_type_data.csv > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Test Case 9:  Calling csm_db_ras_type_script.sh -l invalid_db csm_ras_type_data.csv [valid_file]"
#echo "Test Case 9:  Calling csm_db_ras_type_script.sh -l invalid_db csm_ras_type_data.csv [valid_file]"

#---------------------------------------------------------------------------------------
# Test Case 10: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [valid_file]
# Basic check to see if the functionality of the load process
# with a valid DB name and valid csv file with [NO] response
# same file in the directory that will be shipped
# (no changes to csm_ras_type and csm_ras_type audit tables)
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#-----------------------------------------------------------------------------------------

db_rtc_no_10=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
db_rtac_no_10=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $db_rtc_no_10 > ${TEMP_LOG} 2>&1
db_rtc_no_10_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $db_rtac_no_10 > ${TEMP_LOG} 2>&1
db_rtac_no_10_2=`cat "${TEMP_LOG}"`

    #----------------------------------
    # First process the [NO] response
    #----------------------------------
    yes 'n\'| ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME csm_ras_type_data.csv > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 10: Calling csm_db_ras_type_script.sh -l $DB_NAME [valid_file] [NO] response"
    #echo "Test Case 10: Calling csm_db_ras_type_script.sh -l $DB_NAME [valid_file] [NO] response"
    res_check_no=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify all [NO] responses
    #---------------------------------
    db_rtca_no_10=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    db_rtaca_no_10=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`
    
    printf '%-3d\n' $db_rtca_no_10 > ${TEMP_LOG} 2>&1
    db_rtca_no_10_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $db_rtaca_no_10 > ${TEMP_LOG} 2>&1
    db_rtaca_no_10_2=`cat "${TEMP_LOG}"`
    
        if [[ "$db_rtc_no_10_2" -eq "$db_rtca_no_10_2" ]] && \
        [[ "$db_rtac_no_10_2" -eq "$db_rtaca_no_10_2" ]]; then
            check_return_exit $? 0 "Test Case 11: Verify that rec count remains the same after [NO] response"
            #echo "Test Case 11: Verify that rec count remains the same after [NO] response"
        fi

#--------------------------------------------------------------------------------------
# Test Case 12: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [valid_file]
# Basic check to see if the functionality of the load process
# same file in the directory that will be shipped
# with a valid DB name and valid csv file with [YES] response
# (no changes to csm_ras_type and csm_ras_type audit tables)
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type record import count:     744 
# csm_ras_type record update count:     0
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#--------------------------------------------------------------------------------------

rtcb_yes_12=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_yes_12=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_yes_12 > ${TEMP_LOG} 2>&1
rtcb_yes_12_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_yes_12 > ${TEMP_LOG} 2>&1
rtacb_yes_12_2=`cat "${TEMP_LOG}"`

    #-----------------------------------
    # Second process the [YES] response
    #-----------------------------------
    yes | ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME csm_ras_type_data.csv > ${TEMP_LOG} 2>&1
    
    ras_rc_before_12=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_12=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_update_rc_from_csv_12=`awk '/Record update count from/ {print $8}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_12=`awk '/csm_ras_type live row count after script execution:/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_12=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    
    check_return_exit $? 0 "Test Case 12: Calling csm_db_ras_type_script.sh -l $DB_NAME [valid_file] [YES] response"
    #echo "Test Case 12: Calling csm_db_ras_type_script.sh -l $DB_NAME [valid_file] [YES] response"
    res_check_yes=`grep -c "User response: y" ${TEMP_LOG}`

    #---------------------------------
    # Verify [YES] response
    #---------------------------------
    rtca_yes_12=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_yes_12=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_yes_12 > ${TEMP_LOG} 2>&1
    rtca_yes_12_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_yes_12 > ${TEMP_LOG} 2>&1
    rtaca_yes_12_2=`cat "${TEMP_LOG}"`
    
        if [[ "$rtcb_yes_12_2" == "$rtca_yes_12_2" ]] && \
        [[ "$rtacb_yes_12_2" == "$rtaca_yes_12_2" ]] && \
        [[ "$rtcb_yes_12_2" == "$ras_rc_from_csv_12" ]]; then
            check_return_exit $? 0 "Test Case 13: Verify that record count remains the same after the [YES] response"
            check_return_exit $? 0 "Test Case 13: ----------------------------------------------------------------------"
            check_return_exit $? 0 "Test Case 13: csm_ras_type record count before script execution     | $ras_rc_before_12"
            check_return_exit $? 0 "Test Case 13: csm_ras_type_audit record ct before script execution  | $ras_rc_before_12"
            check_return_exit $? 0 "Test Case 13: Record import count from csv file                     | $ras_rc_from_csv_12"
            check_return_exit $? 0 "Test Case 13: Record update count from csv file                     | $ras_update_rc_from_csv_12"
            check_return_exit $? 0 "Test Case 13: csm_ras_type live row count after script execution    | $ras_type_live_rc_after_imp_12"
            check_return_exit $? 0 "Test Case 13: csm_ras_type_audit live row count                     | $ras_type_audit_after_rc_12"    
            check_return_exit $? 0 "Test Case 13: ----------------------------------------------------------------------"
            #echo "Test Case 13: Verify that record count remains the same after [YES] response"
        fi
    
#------------------------------------------------------------------------------
# Test Case 14: Calling csm_db_script.sh -f [$DB_NAME]
# populating DB with refresh RAS csv data
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -f $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 14: Calling csm_db_script.sh -f $DB_NAME"
#echo "Test Case 14: Calling csm_db_script.sh -f $DB_NAME"

#-----------------------------------------------------------------------------------------
# Test Case 15: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [test_1.csv]
# Basic check to see if the functionality of the load process
# with a valid DB name and valid csv file with [NO] response
# different file in the directory (this could be a file that contains an "updated"
# message type that currently exists in the DB
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#-----------------------------------------------------------------------------------------

db_rtc_u_no_15=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
db_rtac_u_no_15=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $db_rtc_u_no_15 > ${TEMP_LOG} 2>&1
db_rtc_u_no_15_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $db_rtac_u_no_15 > ${TEMP_LOG} 2>&1
db_rtac_u_no_15_2=`cat "${TEMP_LOG}"`

    #----------------------------------
    # First process the [NO] response
    #----------------------------------
    yes 'n\'| ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME ${CSV_FILE_1} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 15: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_1.csv] [NO] response"
    #echo "Test Case 15: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_1.csv] [NO] response"
    res_check_no=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify all [NO] responses
    #---------------------------------
    db_rtca_u_no_15=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    db_rtaca_u_no_15=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`
    
    printf '%-3d\n' $db_rtca_u_no_15 > ${TEMP_LOG} 2>&1
    db_rtca_u_no_15_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $db_rtaca_u_no_15 > ${TEMP_LOG} 2>&1
    db_rtaca_u_no_15_2=`cat "${TEMP_LOG}"`
    
        if [[ "$db_rtc_no_15_2" == "$db_rtca_no_15_2" ]] && \
        [[ "$db_rtac_no_15_2" == "$db_rtaca_no_15_2" ]]; then
            #echo "Test Case 16: Verify that rec count remains the same after [NO] response"
            check_return_exit $? 0 "Test Case 16: Verify that rec count remains the same after [NO] response"
        fi

#---------------------------------------------------------------------------------------
# Test Case 17: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [valid_file]
# Basic check to see if the functionality of the load process
# with a valid DB name and valid csv file with [YES] response
# different file in the directory (this could be a file that contains an "updated"
# message type that currently exists in the DB
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type record import count:     1 
# csm_ras_type record update count:     1
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   745
#---------------------------------------------------------------------------------------

rtcb_u_yes_17=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_u_yes_17=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_u_yes_17 > ${TEMP_LOG} 2>&1
rtcb_u_yes_17_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_u_yes_17 > ${TEMP_LOG} 2>&1
rtacb_u_yes_17_2=`cat "${TEMP_LOG}"`

csv_file_lc_17=`awk 'END {print NR-1}' /var/lib/pgsql/csmdb/fvt_db/csm_ras_type_data_test_1.csv`

    #-----------------------------------
    # Second process the [YES] response
    #-----------------------------------
    yes | ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME ${CSV_FILE_1} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 17: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_1.csv] [YES] response"
    #echo "Test Case 17: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_1.csv] [YES] response"

    ras_rc_before_17=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_17=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_update_rc_from_csv_17=`awk '/Record update count from/ {print $8}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_17=`awk '/csm_ras_type live row count after script execution:/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_17=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    
    #---------------------------------
    # Verify [YES] response
    #---------------------------------
    rtca_u_yes_17=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_u_yes_17=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_u_yes_17 > ${TEMP_LOG} 2>&1
    rtca_u_yes_17_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_u_yes_17 > ${TEMP_LOG} 2>&1
    rtaca_u_yes_17_2=`cat "${TEMP_LOG}"`
    
    uc_after_17=$((rtaca_u_yes_17_2-rtacb_u_yes_17_2))

        if [[ "$rtcb_u_yes_17_2" == "$rtca_u_yes_17_2" ]] && [[ "$rtcb_u_yes_17_2" == "$rtca_u_yes_17_2" ]] &&
        [[ "$csv_file_lc_17" == "$uc_after_17" ]]; then
            check_return_exit $? 0 "Test Case 18: Verify that record count has changed after the [YES] response"
            check_return_exit $? 0 "Test Case 18: ----------------------------------------------------------------------"
            check_return_exit $? 0 "Test Case 18: csm_ras_type record count before script execution     | $ras_rc_before_17"
            check_return_exit $? 0 "Test Case 18: csm_ras_type_audit record ct before script execution  | $rtacb_u_yes_17_2"
            check_return_exit $? 0 "Test Case 18: Record import count from csv file                     | $ras_rc_from_csv_17"
            check_return_exit $? 0 "Test Case 18: Record update count from csv file                     | $ras_update_rc_from_csv_17"
            check_return_exit $? 0 "Test Case 18: csm_ras_type live row count after script execution    | $ras_type_live_rc_after_imp_17"
            check_return_exit $? 0 "Test Case 18: csm_ras_type_audit live row count                     | $ras_type_audit_after_rc_17"    
            check_return_exit $? 0 "Test Case 18: ----------------------------------------------------------------------"
            #echo "Test Case 18: Verify that record count has changed after [YES] response"
        fi
    
#------------------------------------------------------------------------------
# Test Case 19: Calling csm_db_script.sh -f [$DB_NAME]
# populating DB with refresh RAS csv data
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -f $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 19: Calling csm_db_script.sh -f $DB_NAME"
#echo "Test Case 19: Calling csm_db_script.sh -f $DB_NAME"

#-----------------------------------------------------------------------------------------
# Test Case 20: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [test_2.csv]
# Basic check to see if the functionality of the load process
# with a valid DB name and valid csv file with [NO] response
# different file in the directory (this could be a file that contains a "new"
# message type that currently does not exist in the DB
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#-----------------------------------------------------------------------------------------

db_rtc_i_no_20=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
db_rtac_i_no_20=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $db_rtc_i_no_20 > ${TEMP_LOG} 2>&1
db_rtc_i_no_20_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $db_rtac_i_no_20 > ${TEMP_LOG} 2>&1
db_rtac_i_no_20_2=`cat "${TEMP_LOG}"`

    #----------------------------------
    # First process the [NO] response
    #----------------------------------
    yes 'n\'| ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME ${CSV_FILE_2} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 20: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_2.csv] [NO] response"
    #echo "Test Case 20: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_2.csv] [NO] response"
    res_check_no=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify all [NO] responses
    #---------------------------------
    db_rtca_i_no_20=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    db_rtaca_i_no_20=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`
    
    printf '%-3d\n' $db_rtca_i_no_20 > ${TEMP_LOG} 2>&1
    db_rtca_i_no_20_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $db_rtaca_i_no_20 > ${TEMP_LOG} 2>&1
    db_rtaca_i_no_20_2=`cat "${TEMP_LOG}"`
    
        if [[ "$db_rtc_no_20_2" == "$db_rtca_no_20_2" ]] && [[ "$db_rtac_no_20_2" == "$db_rtaca_no_20_2" ]]; then
            check_return_exit $? 0 "Test Case 21: Verify that rec count remains the same after [NO] response"
            #echo "Test Case 21: Verify that rec count remains the same after [NO] response"
        fi

#---------------------------------------------------------------------------------------
# Test Case 22: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [valid_file]
# Basic check to see if the functionality of the load process
# with a valid DB name and valid csv file with (YES) response
# different file in the directory (this could be a file that contains a "new"
# message type that currently does not exist in the DB
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type record import count:     5 
# csm_ras_type record update count:     0
# csm_ras_type rc after script:         749
# csm_ras_type_audit rc after script:   749
#---------------------------------------------------------------------------------------

rtcb_i_yes_22=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_i_yes_22=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_i_yes_22 > ${TEMP_LOG} 2>&1
rtcb_i_yes_22_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_i_yes_22 > ${TEMP_LOG} 2>&1
rtacb_i_yes_22_2=`cat "${TEMP_LOG}"`

csv_file_lc_22=`awk 'END {print NR-1}' /var/lib/pgsql/csmdb/fvt_db/csm_ras_type_data_test_2.csv`

    #-----------------------------------
    # Second process the [YES] response
    #-----------------------------------
    yes | ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME ${CSV_FILE_2} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 22: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_2.csv] [YES] response"
    #echo "Test Case 22: Calling csm_db_ras_type_script.sh -l $DB_NAME [test_2.csv] [YES] response"

    ras_rc_before_22=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_22=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_update_rc_from_csv_22=`awk '/Record update count from/ {print $8}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_22=`awk '/csm_ras_type live row count after script execution/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_22=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    
    #---------------------------------
    # Verify [YES] response
    #---------------------------------
    rtca_i_yes_22=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_i_yes_22=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_i_yes_22 > ${TEMP_LOG} 2>&1
    rtca_i_yes_22_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_i_yes_22 > ${TEMP_LOG} 2>&1
    rtaca_i_yes_22_2=`cat "${TEMP_LOG}"`
    
    uc_after_22=$((rtca_i_yes_22_2-rtcb_i_yes_22_2))
    
        if [[ "$rtcb_i_yes_22_2" == "$rtacb_i_yes_22_2" ]] && [[ "$rtca_i_yes_22_2" == "$rtaca_i_yes_22_2" ]] && \
        [[ "$csv_file_lc_22" == "$uc_after_22" ]]; then
            check_return_exit $? 0 "Test Case 23: Verify that record count has changed after the [YES] response"
            check_return_exit $? 0 "Test Case 23: ----------------------------------------------------------------------"
            check_return_exit $? 0 "Test Case 23: csm_ras_type record count before script execution     | $ras_rc_before_22"
            check_return_exit $? 0 "Test Case 23: csm_ras_type_audit rec count before script execution  | $rtacb_i_yes_22_2"
            check_return_exit $? 0 "Test Case 23: Record import count from csv file                     | $ras_rc_from_csv_22"
            check_return_exit $? 0 "Test Case 23: Record update count from csv file                     | $ras_update_rc_from_csv_22"
            check_return_exit $? 0 "Test Case 23: csm_ras_type live row count after script execution    | $ras_type_live_rc_after_imp_22"
            check_return_exit $? 0 "Test Case 23: csm_ras_type_audit live row count                     | $ras_type_audit_after_rc_22"    
            check_return_exit $? 0 "Test Case 23: ----------------------------------------------------------------------"
            #echo "Test Case 23: Verify that record count has changed after [YES] response"
        fi
    
#------------------------------------------------------------------------------
# Test Case 24: Calling csm_db_script.sh -f [$DB_NAME]
# populating DB with refresh RAS csv data
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -f $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 24: Calling csm_db_script.sh -f $DB_NAME"
#echo "Test Case 24: Calling csm_db_script.sh -f $DB_NAME"

#-------------------------------------------------------------------------------------
# Test Case 25: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [non_compatible_file]
# Basic check to see if the functionality of the load process
# with a non compatible csv file
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#-------------------------------------------------------------------------------------

db_rtc_no_25=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
db_rtac_no_25=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $db_rtc_no_25 > ${TEMP_LOG} 2>&1
db_rtc_no_25_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $db_rtac_no_25 > ${TEMP_LOG} 2>&1
db_rtac_no_25_2=`cat "${TEMP_LOG}"`

    #----------------------------------
    # First process the [NO] response
    #----------------------------------
    yes 'n\'| ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME ${CSV_FILE_3} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 25: Calling csm_db_ras_type_script.sh -l $DB_NAME with [non_compatible_file]"
    #echo "Test Case 25: Calling csm_db_ras_type_script.sh -l $DB_NAME with [non_compatible_file]"
    res_check_no_25=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify all [NO] responses
    #---------------------------------
    db_rtca_no_25=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    db_rtaca_no_25=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`
    
    printf '%-3d\n' $db_rtca_no_25 > ${TEMP_LOG} 2>&1
    db_rtca_no_25_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $db_rtaca_no_25 > ${TEMP_LOG} 2>&1
    db_rtaca_no_25_2=`cat "${TEMP_LOG}"`

        if [[ "$db_rtc_no_25_2" == "$db_rtca_no_25_2" ]] && [[ "$db_rtac_no_25_2" == "$db_rtaca_no_25_2" ]]; then
            #echo "Test Case 26: Verify that rec count remains the same after [NO] response"
            check_return_exit $? 0 "Test Case 26: Verify that rec count remains the same after [NO] response"
        fi

#----------------------------------------------------------------------------------------------
# Test Case 27: Calling csm_db_ras_type_script.sh -l [$DB_NAME] [test_3.csv]
# Basic check to see if the functionality of the load process
# same file in the directory that will be shipped
# with a valid DB name and non_valid csv file with [YES] response
# (no changes to csm_ras_type and csm_ras_type audit tables)
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type record import count:     
# csm_ras_type record update count:     
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#----------------------------------------------------------------------------------------------

rtcb_yes_27=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_yes_27=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_yes_27 > ${TEMP_LOG} 2>&1
rtcb_yes_27_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_yes_27 > ${TEMP_LOG} 2>&1
rtacb_yes_27_2=`cat "${TEMP_LOG}"`

    #-----------------------------------
    # Second process the [YES] response
    #-----------------------------------
    yes | ${DB_PATH}/csm_db_ras_type_script.sh -l $DB_NAME ${CSV_FILE_3} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 0 "Test Case 27: Calling csm_db_ras_type_script.sh -l $DB_NAME [non_compatible.csv] [YES] response"
    #echo "Test Case 27: Calling csm_db_ras_type_script.sh -l $DB_NAME [non_compatible.csv] [YES] response"
    
    ras_rc_before_27=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_27=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_update_rc_from_csv_27=`awk '/Record update count from/ {print $8}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_27=`awk '/csm_ras_type live row count after script execution:/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_27=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    res_check_yes=`grep -c "User response: y" ${TEMP_LOG}`

    #---------------------------------
    # Verify [YES] response
    #---------------------------------
    rtca_yes_27=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_yes_27=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_yes_27 > ${TEMP_LOG} 2>&1
    rtca_yes_27_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_yes_27 > ${TEMP_LOG} 2>&1
    rtaca_yes_27_2=`cat "${TEMP_LOG}"`
    
        if [[ "$rtcb_yes_27_2" == "$rtca_yes_27_2" ]] && [[ "$rtacb_yes_27_2" == "$rtaca_yes_27_2" ]] && \
        [[ "$ras_rc_from_csv_27" -eq 0 ]] && [[ "$ras_update_rc_from_csv_27" -eq 0 ]]; then
            check_return_exit $? 0 "Test Case 28: Verify that record count remains the same after the [YES] response"
            check_return_exit $? 0 "Test Case 28: ----------------------------------------------------------------------"
            check_return_exit $? 0 "Test Case 28: csm_ras_type record count before script execution     | $rtcb_yes_27_2"
            check_return_exit $? 0 "Test Case 28: csm_ras_type_audit rec count before script execution  | $rtacb_yes_27_2"
            check_return_exit $? 0 "Test Case 28: Record delete count from the csm_ras_type table       | $ras_rc_delete_27"
            check_return_exit $? 0 "Test Case 28: Record import count from csv file                     | $ras_rc_from_csv_27"
            check_return_exit $? 0 "Test Case 28: csm_ras_type live row count after script execution    | $rtca_yes_27_2"
            check_return_exit $? 0 "Test Case 28: csm_ras_type_audit live row count                     | $rtaca_yes_27_2"    
            check_return_exit $? 0 "Test Case 28: ----------------------------------------------------------------------"
            #echo "Test Case 28: Verify that record count remains the same after [YES] response"
        fi

#------------------------------------------------------------------------------
# Test Case 29: Calling csm_db_ras_type_script.sh -r [no_db] [no_file]
# Basic check to see if the functionality of the remove and load process
# with no given DB or file name.
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_ras_type_script.sh -r > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 29: Calling csm_db_ras_type_script.sh -r with [no_db] [no_file]"
#echo "Test Case 29: Calling csm_db_ras_type_script.sh -r with [no_db] [no_file]"

#----------------------------------------------------------------------------------------
# Test Case 30: Calling csm_db_ras_type_script.sh -r [$DB_NAME] with [non_existing_file]
# Basic check to see if the functionality of the remove and load process
# with a valid DB name and [non_existing_file_name]
#----------------------------------------------------------------------------------------

${DB_PATH}/csm_db_ras_type_script.sh -r $DB_NAME non_existing_file.csv > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 30: Calling csm_db_ras_type_script.sh -r $DB_NAME with [non_existing_file_name]"
#echo "Test Case 30: Calling csm_db_ras_type_script.sh -r $DB_NAME with [non_existing_file_name]"

#------------------------------------------------------------------------------
# Test Case 31: Calling csm_db_ras_type_script.sh -r [non_existing_db_name] with [valid_file]
# Basic check to see if the functionality of the remove and load process
# with a [non_existing_db_name] and [valid_file]
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_ras_type_script.sh -r test_db ${CSV_FILE_1} > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Test Case 31: Calling csm_db_ras_type_script.sh -r [non_existing_db_name] with [valid_file]"
#echo "Test Case 31: Calling csm_db_ras_type_script.sh -r [non_existing_db_name] with [valid_file]"

#------------------------------------------------------------------------------
# Test Case 32: Calling csm_db_script.sh -f [$DB_NAME]
# populating DB with refresh RAS csv data
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -f $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 32: Calling csm_db_script.sh -f $DB_NAME"
#echo "Test Case 32: Calling csm_db_script.sh -f $DB_NAME"

#---------------------------------------------------------------------------------
# Test Case 33: Calling csm_db_ras_type_script.sh -r [$DB_NAME] with [valid_file]
# Basic check to see if the functionality of the remove and load process
# with a valid DB name and valid file name [NO] response
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#---------------------------------------------------------------------------------

rtcb_no_33=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_no_33=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_no_33 > ${TEMP_LOG} 2>&1
rtcb_no_33_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_no_33 > ${TEMP_LOG} 2>&1
rtacb_no_33_2=`cat "${TEMP_LOG}"`

    #-----------------------------------
    # Second process the [NO] response
    #-----------------------------------
    yes 'n/'| ${DB_PATH}/csm_db_ras_type_script.sh -r $DB_NAME ${CSV_FILE_1} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 1 "Test Case 33: Calling csm_db_ras_type_script.sh -r $DB_NAME with [valid_file_name] [NO] response"
    #echo "Test Case 33: Calling csm_db_ras_type_script.sh -r $DB_NAME with [valid_file_name] [NO] response"
    
    ras_rc_before_33=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_33=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_update_rc_from_csv_33=`awk '/Record update count from/ {print $8}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_33=`awk '/csm_ras_type live row count after script execution:/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_33=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    res_check_no=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify [NO] response
    #---------------------------------
    rtca_no_33=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_no_33=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_no_33 > ${TEMP_LOG} 2>&1
    rtca_no_33_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_no_33 > ${TEMP_LOG} 2>&1
    rtaca_no_33_2=`cat "${TEMP_LOG}"`
    
        if [[ "$rtcb_no_33_2" == "$rtca_no_33_2" ]] && [[ "$rtacb_no_33_2" == "$rtaca_no_33_2" ]]; then
            check_return_exit $? 0 "Test Case 34: Verify that record count remains the same after the [NO] response"
            #echo "Test Case 34: Verify that record count remains the same after [NO] response"
        fi
    
#----------------------------------------------------------------------------------------------
# Test Case 35: Calling csm_db_ras_type_script.sh -r [$DB_NAME] [test_1.csv]
# Basic check to see if the functionality of remove and the load process
# with a valid DB name and valid csv file with [YES] response
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type delete count:            744
# csm_ras_type record import count:     1
# csm_ras_type rc after script:         1
# csm_ras_type_audit rc after script:   1489
#----------------------------------------------------------------------------------------------

rtcb_yes_35=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_yes_35=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_yes_35 > ${TEMP_LOG} 2>&1
rtcb_yes_35_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_yes_35 > ${TEMP_LOG} 2>&1
rtacb_yes_35_2=`cat "${TEMP_LOG}"`

csv_file_lc_35=`awk 'END {print NR-1}' /var/lib/pgsql/csmdb/fvt_db/csm_ras_type_data_test_1.csv`
dc_after_35=$((rtcb_yes_35_2*2))
dc_after_35_2=$(($dc_after_35+$csv_file_lc_35))

    #-----------------------------------
    # Second process the [YES] response
    #-----------------------------------
    yes | ${DB_PATH}/csm_db_ras_type_script.sh -r $DB_NAME ${CSV_FILE_1} > ${TEMP_LOG} 2>&1
    
    ras_rc_before_35=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_35=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_rc_delete_35=`awk '/Record delete count from the/ {print $10}' ${TEMP_LOG}`
    ras_rc_after_remove_35=`awk '/csm_ras_type live row count:/ {print $7}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_35=`awk '/csm_ras_type live row count after script execution/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_35=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    
    rtca_yes_35=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_yes_35=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_yes_35 > ${TEMP_LOG} 2>&1
    rtca_yes_35_2=`cat "${TEMP_LOG}"`

    printf '%-3d\n' $rtaca_yes_35 > ${TEMP_LOG} 2>&1
    rtaca_yes_35_2=`cat "${TEMP_LOG}"`
    
    check_return_exit $? 0 "Test Case 35: Calling csm_db_ras_type_script.sh -r $DB_NAME [test_1.csv] [YES] response"
    #echo "Test Case 35: Calling csm_db_ras_type_script.sh -r $DB_NAME [test_1.csv] [YES] response"
    res_check_yes_35=`grep -c "User response: y" ${TEMP_LOG}`

    #---------------------------------
    # Verify [YES] response
    #---------------------------------
    rtca_yes_35=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_yes_35=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_yes_35 > ${TEMP_LOG} 2>&1
    rtca_yes_35_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_yes_35 > ${TEMP_LOG} 2>&1
    rtaca_yes_35_2=`cat "${TEMP_LOG}"`
    
        if [[ "$rtcb_yes_35_2" == "$ras_rc_delete_35" ]] && [[ "$rtcb_yes_35_2" == "$rtacb_yes_35_2" ]] && \
        [[ "$ras_rc_from_csv_35" == "$csv_file_lc_35" ]] && [[ "$dc_after_35_2" == "$rtaca_yes_35_2" ]]; then
            check_return_exit $? 0 "Test Case 36: Verify that record count remains the same after the [YES] response"
            check_return_exit $? 0 "Test Case 36: ----------------------------------------------------------------------"
            check_return_exit $? 0 "Test Case 36: csm_ras_type record count before script execution     | $rtcb_yes_35_2"
            check_return_exit $? 0 "Test Case 36: csm_ras_type_audit rec count before script execution  | $rtacb_yes_35_2"
            check_return_exit $? 0 "Test Case 36: Record delete count from the csm_ras_type table       | $ras_rc_delete_35"
            check_return_exit $? 0 "Test Case 36: Record import count from csv file                     | $ras_rc_from_csv_35"
            check_return_exit $? 0 "Test Case 36: csm_ras_type live row count after script execution    | $rtca_yes_35_2"
            check_return_exit $? 0 "Test Case 36: csm_ras_type_audit live row count                     | $rtaca_yes_35_2"    
            check_return_exit $? 0 "Test Case 36: ----------------------------------------------------------------------"
            #echo "Test Case 36: Verify that record count remains the same after [YES] response"
        fi

#------------------------------------------------------------------------------
# Test Case 37: Calling csm_db_script.sh -f [$DB_NAME]
# populating DB with refresh RAS csv data
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -f $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 37: Calling csm_db_script.sh -f $DB_NAME"
#echo "Test Case 37: Calling csm_db_script.sh -f $DB_NAME"

#---------------------------------------------------------------------------------
# Test Case 38: Calling csm_db_ras_type_script.sh -r [$DB_NAME] with [test_2.csv]
# Basic check to see if the functionality of the remove and load process
# with a valid DB name and valid file name [NO] response
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc after script:   744
#----------------------------------------------------------------------------------------------

rtcb_no_38=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_no_38=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_no_38 > ${TEMP_LOG} 2>&1
rtcb_no_38_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_no_38 > ${TEMP_LOG} 2>&1
rtacb_no_38_2=`cat "${TEMP_LOG}"`

    #-----------------------------------
    # Second process the [NO] response
    #-----------------------------------
    yes 'n/'| ${DB_PATH}/csm_db_ras_type_script.sh -r $DB_NAME ${CSV_FILE_2} > ${TEMP_LOG} 2>&1
    
    check_return_exit $? 1 "Test Case 38: Calling csm_db_ras_type_script.sh -r $DB_NAME with [test_2.csv] [NO] response"
    #echo "Test Case 38: Calling csm_db_ras_type_script.sh -r $DB_NAME with [test_2.csv] [NO] response"
    
    ras_rc_before_38=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_38=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_rc_delete_38=`awk '/Record delete count from the/ {print $10}' ${TEMP_LOG}`
    ras_rc_after_remove_38=`awk '/csm_ras_type live row count:/ {print $7}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_38=`awk '/csm_ras_type live row count after script execution/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_38=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    res_check_no_38=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify [NO] response
    #---------------------------------
    rtca_no_38=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_no_38=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_no_38 > ${TEMP_LOG} 2>&1
    rtca_no_38_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_no_38 > ${TEMP_LOG} 2>&1
    rtaca_no_38_2=`cat "${TEMP_LOG}"`

        if [[ "$rtcb_no_38_2" == "$rtca_no_38_2" ]] && [[ "$rtacb_no_38_2" == "$rtaca_no_38_2" ]]; then
            check_return_exit $? 0 "Test Case 39: Verify that record count remains the same after the [NO] response"
            #echo "Test Case 39: Verify that record count remains the same after [NO] response"
        fi

#----------------------------------------------------------------------------------------------
# Test Case 40: Calling csm_db_ras_type_script.sh -r [$DB_NAME] [test_2.csv]
# Basic check to see if the functionality of remove and the load process
# with a valid DB name and valid csv file with [YES] response
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type delete count:            744
# csm_ras_type record import count:     5
# csm_ras_type rc after script:         5
# csm_ras_type_audit rc after script:   1493
#----------------------------------------------------------------------------------------------

rtcb_yes_40=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_yes_40=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_yes_40 > ${TEMP_LOG} 2>&1
rtcb_yes_40_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_yes_40 > ${TEMP_LOG} 2>&1
rtacb_yes_40_2=`cat "${TEMP_LOG}"`

csv_file_lc_40=`awk 'END {print NR-1}' /var/lib/pgsql/csmdb/fvt_db/csm_ras_type_data_test_2.csv`
dc_after_40=$((rtcb_yes_40_2*2))
dc_after_40_2=$(($dc_after_40+$csv_file_lc_40))

    #-----------------------------------
    # Second process the [YES] response
    #-----------------------------------
    yes | ${DB_PATH}/csm_db_ras_type_script.sh -r $DB_NAME ${CSV_FILE_2} > ${TEMP_LOG} 2>&1
   
    check_return_exit $? 0 "Test Case 40: Calling csm_db_ras_type_script.sh -r $DB_NAME [test_2.csv] [YES] response"
    #echo "Test Case 40: Calling csm_db_ras_type_script.sh -r $DB_NAME [test_2.csv] [YES] response"
    
    ras_rc_before_40=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_40=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_rc_delete_40=`awk '/Record delete count from the/ {print $10}' ${TEMP_LOG}`
    ras_rc_after_remove_40=`awk '/csm_ras_type live row count:/ {print $7}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_40=`awk '/csm_ras_type live row count after script execution/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_40=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    res_check_yes=`grep -c "User response: y" ${TEMP_LOG}`

    #---------------------------------
    # Verify [YES] response
    #---------------------------------
    rtca_yes_40=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_yes_40=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_yes_40 > ${TEMP_LOG} 2>&1
    rtca_yes_40_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_yes_40 > ${TEMP_LOG} 2>&1
    rtaca_yes_40_2=`cat "${TEMP_LOG}"`

        if [[ "$rtcb_yes_40_2" == "$ras_rc_delete_40" ]] && [[ "$rtacb_yes_40_2" == "$rtcb_yes_40_2" ]] && \
        [[ "$rtca_yes_40_2" -eq "$csv_file_lc_40" ]] && [[ "$rtaca_yes_40_2" == "$dc_after_40_2" ]]; then
            check_return_exit $? 0 "Test Case 41: Verify that record count has changed after the [YES] response"
            check_return_exit $? 0 "Test Case 41: ----------------------------------------------------------------------"
            check_return_exit $? 0 "Test Case 41: csm_ras_type record count before script execution     | $rtcb_yes_40_2"
            check_return_exit $? 0 "Test Case 41: csm_ras_type_audit rec count before script execution  | $rtacb_yes_40_2"
            check_return_exit $? 0 "Test Case 41: Record delete count from the csm_ras_type table       | $ras_rc_delete_40"
            check_return_exit $? 0 "Test Case 41: Record import count from csv file                     | $ras_rc_from_csv_40"
            check_return_exit $? 0 "Test Case 41: csm_ras_type live row count after script execution    | $rtca_yes_40_2"
            check_return_exit $? 0 "Test Case 41: csm_ras_type_audit live row count                     | $rtaca_yes_40_2"    
            check_return_exit $? 0 "Test Case 41: ----------------------------------------------------------------------"
            #echo "Test Case 41: Verify that record count has changed after [YES] response"
        fi
        
#------------------------------------------------------------------------------
# Test Case 42: Calling csm_db_script.sh -f [$DB_NAME]
# populating DB with refresh RAS csv data
#------------------------------------------------------------------------------

${DB_PATH}/csm_db_script.sh -f $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 42: Calling csm_db_script.sh -f $DB_NAME"
#echo "Test Case 42: Calling csm_db_script.sh -f $DB_NAME"

#---------------------------------------------------------------------------------
# Test Case 43: Calling csm_db_ras_type_script.sh -r [$DB_NAME] with [test_3.csv]
# Basic check to see if the functionality of the remove and load process
# with a valid DB name and non_compatible_file [NO] response
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type rc after script:         744
# csm_ras_type_audit rc before script:  744
#-----------------------------------------------------------------------------

rtcb_no_43=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_no_43=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_no_43 > ${TEMP_LOG} 2>&1
rtcb_no_43_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_no_43 > ${TEMP_LOG} 2>&1
rtacb_no_43_2=`cat "${TEMP_LOG}"`

    #-----------------------------------
    # Second process the [NO] response
    #-----------------------------------
    yes 'n/'| ${DB_PATH}/csm_db_ras_type_script.sh -r $DB_NAME ${CSV_FILE_3} > ${TEMP_LOG} 2>&1
   
    check_return_exit $? 1 "Test Case 43: Calling csm_db_ras_type_script.sh -r $DB_NAME with [non_comp_file] [NO] response"
    #echo "Test Case 43: Calling csm_db_ras_type_script.sh -r $DB_NAME with [non_comp_file] [NO] response"
    
    ras_rc_before_43=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_43=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_rc_delete_43=`awk '/Record delete count from the/ {print $10}' ${TEMP_LOG}`
    ras_rc_after_remove_43=`awk '/csm_ras_type live row count:/ {print $7}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_43=`awk '/csm_ras_type live row count after script execution/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_43=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`
    res_check_no=`grep -c "User response: n" ${TEMP_LOG}`

    #---------------------------------
    # Verify [NO] response
    #---------------------------------
    rtca_no_43=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_no_43=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_no_43 > ${TEMP_LOG} 2>&1
    rtca_no_43_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_no_43 > ${TEMP_LOG} 2>&1
    rtaca_no_43_2=`cat "${TEMP_LOG}"`
    
        if [[ "$rtcb_no_43_2" == "$rtca_no_43_2" ]] && [[ "$rtacb_no_43_2" == "$rtaca_no_43_2" ]]; then
            check_return_exit $? 0 "Test Case 44: Verify that record count remains the same after the [NO] response"
            #echo "Test Case 44: Verify that record count remains the same after [NO] response"
        fi

#-----------------------------------------------------------------------------
# Test Case 45: Calling csm_db_ras_type_script.sh -r [$DB_NAME] [test_3.csv]
# Basic check to see if the functionality of remove and the load process
# with a valid DB name and valid csv file with [YES] response
#-----------------------------------------------------------------------------
# Expected DB results:
#-----------------------------------------------------------------------------
# csm_ras_type rc before script:        744
# csm_ras_type_audit rc before script:  744
# csm_ras_type delete count:            744
# csm_ras_type record import count:     0
# csm_ras_type rc after script:         0
# csm_ras_type_audit rc before script:  1488
#-----------------------------------------------------------------------------

rtcb_yes_45=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
rtacb_yes_45=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

printf '%-3d\n' $rtcb_yes_45 > ${TEMP_LOG} 2>&1
rtcb_yes_45_2=`cat "${TEMP_LOG}"`

printf '%-3d\n' $rtacb_yes_45 > ${TEMP_LOG} 2>&1
rtacb_yes_45_2=`cat "${TEMP_LOG}"`

dc_after_45=$((rtcb_yes_45_2*2))
csv_file_lc_45=`awk 'END {print NR-1}' /var/lib/pgsql/csmdb/fvt_db/csm_ras_type_data_test_3.csv`

    #-----------------------------------
    # Second process the [YES] response
    #-----------------------------------
    yes | ${DB_PATH}/csm_db_ras_type_script.sh -r $DB_NAME ${CSV_FILE_3} > ${TEMP_LOG} 2>&1
   
    check_return_exit $? 0 "Test Case 45: Calling csm_db_ras_type_script.sh -r $DB_NAME [non_comp_file] [YES] response"
    #echo "Test Case 45: Calling csm_db_ras_type_script.sh -r $DB_NAME [non_comp_file] [YES] response"
    
    ras_rc_before_45=`awk '/csm_ras_type record count before script execution:/ {print $9}' ${TEMP_LOG}`
    ras_rc_from_csv_45=`awk '/Record import count from/ {print $8}' ${TEMP_LOG}`
    ras_rc_delete_45=`awk '/Record delete count from the/ {print $10}' ${TEMP_LOG}`
    ras_rc_after_remove_45=`awk '/csm_ras_type live row count:/ {print $7}' ${TEMP_LOG}`
    ras_type_live_rc_after_imp_45=`awk '/csm_ras_type live row count after script execution/ {print $10}' ${TEMP_LOG}`
    ras_type_audit_after_rc_45=`awk '/csm_ras_type_audit live row count:/ {print $7}' ${TEMP_LOG}`

    #res_check_yes=`grep -c "User response: y" ${TEMP_LOG}`

    #---------------------------------
    # Verify [YES] response
    #---------------------------------
    rtca_yes_45=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type" postgres`
    rtaca_yes_45=`psql -t -q -d $DB_NAME -c "select count(*) from csm_ras_type_audit" postgres`

    printf '%-3d\n' $rtca_yes_45 > ${TEMP_LOG} 2>&1
    rtca_yes_45_2=`cat "${TEMP_LOG}"`
    
    printf '%-3d\n' $rtaca_yes_45 > ${TEMP_LOG} 2>&1
    rtaca_yes_45_2=`cat "${TEMP_LOG}"`
    
        #-------------------------------------------------
        # Verify that the results are valid
        #-------------------------------------------------
        
        if [[ "$rtcb_yes_45_2" == "$ras_rc_delete_45" ]] && [[ "$rtacb_yes_45_2" == "$rtcb_yes_45_2" ]] && \
        [[ "$rtca_yes_45_2" -eq 0 ]] && [[ "$dc_after_45" == "$rtaca_yes_45_2" ]]; then
            check_return_exit $? 0 "Test Case 46: Verify that record count has changed after the [YES] response"
            check_return_exit $? 0 "Test Case 46: ----------------------------------------------------------------------"
            check_return_exit $? 0 "Test Case 46: csm_ras_type record count before script execution     | $rtcb_yes_45_2"
            check_return_exit $? 0 "Test Case 46: csm_ras_type_audit rec count before script execution  | $rtacb_yes_45_2"
            check_return_exit $? 0 "Test Case 46: Record delete count from the csm_ras_type table       | $ras_rc_delete_45"
            check_return_exit $? 0 "Test Case 46: Record import count from csv file                     | $ras_rc_from_csv_45"
            check_return_exit $? 0 "Test Case 46: csm_ras_type live row count after script execution    | $rtca_yes_45_2"
            check_return_exit $? 0 "Test Case 46: csm_ras_type_audit live row count                     | $rtaca_yes_45_2"    
            check_return_exit $? 0 "Test Case 46: ----------------------------------------------------------------------"
            #echo "Test Case 46: Verify that record count has changed after [YES] response"
        fi

#------------------------------------------------------------------------------
# Test Case 47: Calling csm_db_script.sh -d [$DB_NAME]
#------------------------------------------------------------------------------

echo "y" | ${DB_PATH}/csm_db_script.sh -d $DB_NAME > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 47: Calling csm_db_script.sh -d $DB_NAME"
#echo "Test Case 47: Calling csm_db_script.sh -d $DB_NAME"

#------------------------------------------------------------------------------
# Test Case 48: Drop the fvt DB user
#------------------------------------------------------------------------------
wait
psql -t -q -U postgres -d postgres -c "DROP USER $DB_USER;"
check_return_exit $? 0 "Test Case 48: Dropping DB User: $DB_USER"
#echo "Test Case 48: Dropping FVT test DB user: $DB_USER"

#------------------------------------------------------------------------------
# Test Case 49: Checking [$DB_NAME] was deleted
#------------------------------------------------------------------------------
wait
#psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME 2>>/dev/null
su -c "psql -d $DB_NAME -c \"\q\"" postgres > ${TEMP_LOG} 2>&1
check_return_exit $? 2 "Test Case 49: Checking $DB_NAME was deleted"
#check_return_exit $? 1 "Test Case 49: Checking $DB_NAME was deleted"
#echo "Test Case 49: Checking $DB_NAME was deleted"

rm -f ${TEMP_LOG}
#echo "Removed temp file"
echo "------------------------------------------------------------" >> ${LOG}
echo "CSM DB RAS Type Script Bucket PASSED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
