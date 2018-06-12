#================================================================================
#   
#    buckets/error_injection/bb.sh
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

# Error Injection Bucket for BB testing
# VG & LV, Query & Delete

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/error_injection/bb.log
CSM_PATH=/opt/ibm/csm/bin
TEMP_LOG=${LOG_PATH}/buckets/error_injection/bb_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/error_injection/bb_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
        exit 1
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "            Starting Error Injected Node Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# VG QUERY Section
# Test Case 1: csm_bb_vg_query NO ARGS
${CSM_PATH}/csm_bb_vg_query > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 1: csm_bb_vg_query NO ARGS"

# Test Case 2: csm_bb_vg_query vg does not exist
${CSM_PATH}/csm_bb_vg_query -V doesnotexist > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 2: csm_bb_vg_query vg does not exist"

# Test Case 3: csm_bb_vg_query no vg name input
${CSM_PATH}/csm_bb_vg_query -V > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 3: csm_bb_vg_query no vg name input"

# Test Case 4: csm_bb_vg_query node does not exist
${CSM_PATH}/csm_bb_vg_query -n doesnotexist > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 4: csm_bb_vg_query node does not exist"

# Test Case 5: csm_bb_vg_query no node name input
${CSM_PATH}/csm_bb_vg_query -n > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 5: csm_bb_vg_query no node name input"

# Test Case 6: csm_bb_vg_query invalid option
${CSM_PATH}/csm_bb_vg_query -x invalidoption > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 6: csm_bb_vg_query invalid option"

# LV QUERY Section
# Test Case 7: csm_bb_lv_query NO ARGS
${CSM_PATH}/csm_bb_lv_query > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 7: csm_bb_lv_query NO ARGS"

# Test Case 8: csm_bb_lv_query lv does not exist
${CSM_PATH}/csm_bb_lv_query -L doesnotexist > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 8: csm_bb_lv_query lv does not exist"

# Test Case 9: csm_bb_lv_query no lv name input
${CSM_PATH}/csm_bb_lv_query -L > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 9: csm_bb_lv_query no lv name input"

# Test Case 10: csm_bb_lv_query node does not exist
${CSM_PATH}/csm_bb_lv_query -n doesnotexist > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 10: csm_bb_lv_query node does not exist"

# Test Case 11: csm_bb_lv_query no node name input
${CSM_PATH}/csm_bb_lv_query -n > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 11: csm_bb_lv_query no node name input"

# Test Case 12: csm_bb_lv_query allocation does not exist
${CSM_PATH}/csm_bb_lv_query -a 123456789 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 4 "Test Case 12: csm_bb_lv_query allocation does not exist"

# Test Case 13: csm_bb_lv_query no allocation input
${CSM_PATH}/csm_bb_lv_query -a > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 13: csm_bb_lv_query no allocation input"

# Test Case 14: csm_bb_lv_query invalid allocation input type
${CSM_PATH}/csm_bb_lv_query -a badinputtype > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 14: csm_bb_lv_query invalid allocation input type"

# Test Case 15: csm_bb_lv_query invalid option
${CSM_PATH}/csm_bb_lv_query -x invalidoption > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 15: csm_bb_lv_query invalid option"

# VG DELETE Section
# Test Case 16: csm_bb_vg_delete NO ARGS
${CSM_PATH}/csm_bb_vg_delete > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 16: csm_bb_vg_delete NO ARGS"

# Test Case 17: csm_bb_vg_delete vg name does not exist
${CSM_PATH}/csm_bb_vg_delete -V doesnotexist -n ${SINGLE_COMPUTE} > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 17: csm_bb_vg_delete vg name does not exist"

# Test Case 18: csm_bb_vg_delete no vg input
${CSM_PATH}/csm_bb_vg_delete -V > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 18: csm_bb_vg_delete no vg input"

# Test Case 19: csm_bb_vg_delete invalid option
${CSM_PATH}/csm_bb_vg_delete -x invalidoption > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 19: csm_bb_vg_delete invalid option"

# LV DELETE Section
# Test Case 20: csm_bb_lv_delete NO ARGS
${CSM_PATH}/csm_bb_lv_delete > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 8 "Test Case 20: csm_bb_lv_delete NO ARGS"

# Test Case 21: csm_bb_lv_delete allocation does not exist
${CSM_PATH}/csm_bb_lv_delete -a 123456789 -l lv_01 -n ${SINGLE_COMPUTE} -r 1 -w 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 21: csm_bb_lv_delete allocation does not exist"

# Test Case 22: csm_bb_lv_delete invalid allocation input type
${CSM_PATH}/csm_bb_lv_delete -a invalidinputtype -l lv_01 -n ${SINGLE_COMPUTE} -r 1 -w 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 22: csm_bb_lv_delete invalid allocation input type"

# Test Case 23: csm_bb_lv_delete no allocation input
${CSM_PATH}/csm_bb_lv_delete -a -l lv_01 -n ${SINGLE_COMPUTE} -r 1 -w 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 23: csm_bb_lv_delete no allocation input"

# Test Case 24: csm_bb_lv_delete lv does not exist
${CSM_PATH}/csm_bb_lv_delete -a 1 -l doesnotexist -n ${SINGLE_COMPUTE} -r 1 -w 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 24: csm_bb_lv_delete lv does not exist"

# Test Case 25: csm_bb_lv_delete node name does not exist
${CSM_PATH}/csm_bb_lv_delete -a 1 -l lv_01 -n doesnotexist -r 1 -w 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 25 "Test Case 25: csm_bb_lv_delete node name does not exist"

# Test Case 26: csm_bb_lv_delete invalid read input type
${CSM_PATH}/csm_bb_lv_delete -a 1 -l lv_01 -n ${SINGLE_COMPUTE} -r invalidinputtype -w 1 > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 26: csm_bb_lv_delete invalid read input type"

# Test Case 27: csm_bb_lv_delete invalid write input type
${CSM_PATH}/csm_bb_lv_delete -a 1 -l lv_01 -n ${SINGLE_COMPUTE} -r 1 -w invalidinputtype > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 27: csm_bb_lv_delete invalid write input type"

# Test Case 28: csm_bb_lv_delete invalid option
${CSM_PATH}/csm_bb_lv_delete -x invalidoption > ${TEMP_LOG} 2>&1
check_return_flag_nz $? 9 "Test Case 28: csm_bb_lv_delete invalid option"

echo "------------------------------------------------------------" >> ${LOG}
echo "           Error Injected Node Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

rm -f ${TEMP_LOG}
exit 0
