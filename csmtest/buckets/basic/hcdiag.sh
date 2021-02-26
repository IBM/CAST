#================================================================================
#   
#    buckets/basic/hc_diag.sh
# 
#  © Copyright IBM Corporation 2015-2021. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

# Bucket for hcdiag testing
# run through hcdiag tests

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

# Local Variables
LOG=${LOG_PATH}/buckets/basic/hcdiag.log
TEMP_LOG=${LOG_PATH}/buckets/basic/hcdiag_temp.log
FLAG_LOG=${LOG_PATH}/buckets/basic/hcdiag_flags.log
HC_DIAG_PATH=/opt/ibm/csm/hcdiag
CLUST_CONF_DIR=/tmp/csm_fvt_hcdiag_backup/clustconf_yaml_backup
TEST_PROP_DIR=/tmp/csm_fvt_hcdiag_backup/test_properties_backup
CLUST_CONF_FILE=clustconf.yaml.org.*
TEST_PROP_FILE=test.properties*

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting hcdiag Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

#---------------------------------------------------------
# Setup the clustconf.yaml with the environment details
#---------------------------------------------------------
# 1. Run the setup script to set up the compute node
# 2. Run the test properties script for P8 or P9 nodes
#---------------------------------------------------------
${FVT_PATH}/include/hcdiag/csm_fvt_diag_clustconf_yaml_setup_p8_p9.sh >> ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 0: clustconf.yaml setup"

${FVT_PATH}/include/hcdiag/csm_fvt_diag_test_properties_setup_p8_p9.sh >> ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 0: test.properties setup"

# Copy included test.properties and clustconf.yaml over to active test.properties
echo "y" | cp ${FVT_PATH}/include/hcdiag/test.properties ${HC_DIAG_PATH}/etc/
xdcp ${COMPUTE_NODES} ${FVT_PATH}/include/hcdiag/test.properties ${HC_DIAG_PATH}/etc/
echo "y" | cp ${FVT_PATH}/include/hcdiag/clustconf.yaml ${HC_DIAG_PATH}/etc/
xdcp ${COMPUTE_NODES} ${FVT_PATH}/include/hcdiag/clustconf.yaml ${HC_DIAG_PATH}/etc/

# Test Case 1: ppping
retry_process -c 1 -m 3 -d 2 ${HC_DIAG_PATH}/bin/hcdiag_run.py --test ppping --target ${COMPUTE_NODES} >> ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 1: ppping"

# Get run id from temp log
run_id=`grep "run id" ${TEMP_LOG} | awk -F' ' '{print $6}' | awk -F',' '{print $1}'`

# Test Case 2: csm_diag_run_query
${CSM_PATH}/csm_diag_run_query -r ${run_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: csm_diag_run_query"

# Test Case 3: csm_diag_run_query_details
${CSM_PATH}/csm_diag_run_query_details -r ${run_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: csm_diag_run_query_details"

# Test Case 4: hcdiag_query.py
${HC_DIAG_PATH}/bin/hcdiag_query.py -r ${run_id} > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4: hcdiag_query.py"

# Test Case 5: chk-csm-health
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-csm-health --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 5: chk-csm-health"

# Test Case 6: chk-free-memory
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-free-memory --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 6: chk-free-memory"

# Test Case 7: chk-led
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-led --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 7: chk-led"

# Test Case 8: chk-boot-time
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-boot-time --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 8: chk-boot-time"

# Test Case 9: chk-noping
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-noping --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 9: chk-noping"

# Test Case 10: rpower
${HC_DIAG_PATH}/bin/hcdiag_run.py --test rpower --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 10: rpower"

# Test Case 11: chk-load-average
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-load-average --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 11: chk-load-average"

# Test Case 12: chk-load-cpu
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-load-cpu --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 12: chk-load-cpu"

# Test Case 13: chk-load-mem
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-load-mem --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 13: chk-load-mem"

# Test Case 14: chk-power
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-power --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 14: chk-power"

# Test Case 15: chk-smt
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-smt --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 15: chk-smt"

# Test Case 16: chk-temp
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-temp --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 16: chk-temp"

# Test Case 17: chk-zombies
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-zombies --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 17: chk-zombies"

# Test Case 18: test-simple
${HC_DIAG_PATH}/bin/hcdiag_run.py --test test-simple --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 18: test-simple"

# Test Case 19: chk-process
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-process --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 19: chk-process"

# Test Case 20: chk-nfs-mount
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-nfs-mount --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 20: chk-nfs-mount"

# Test Case 21: chk-cpu
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-cpu --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 21: chk-cpu"

# Test Case 22: chk-cpu-count
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-cpu-count --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 22: chk-cpu-count"

# Test Case 23: chk-sys-firmware
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-sys-firmware --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 23: chk-sys-firmware"

# Test Case 24: chk-memory
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-memory --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 24: chk-memory"

# Test Case 25: chk-aslr
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-aslr --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 25: chk-aslr"

# Test Case 26: chk-os
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-os --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 26: chk-os"

# Test Case 27: chk-temp
${HC_DIAG_PATH}/bin/hcdiag_run.py --test chk-temp --target ${COMPUTE_NODES} > ${TEMP_LOG} 2>&1
check_return_flag_value $? 0 "Test Case 27: chk-temp"

rm -f ${TEMP_LOG}

# Clean up the scripts if exists (default configuration templates)
# Clean up and replace the clustconf.yaml file
if [ -f  ${CLUST_CONF_DIR}/${CLUST_CONF_FILE} ] ; then
    \cp -rf ${CLUST_CONF_DIR}/${CLUST_CONF_FILE} ${FVT_PATH}/include/hcdiag/clustconf.yaml
    check_return_flag_value $? 0 "Cleanup     : clustconf.yaml file"
    #rm -f /tmp/csm_fvt_hcdiag_backup/clustconf_yaml_backup/clustconf.yaml.org.*
    rm -f ${CLUST_CONF_DIR}/${CLUST_CONF_FILE}
else
    RC=0
fi

# Clean up and replace the test.properties file
if [ -f  ${TEST_PROP_DIR}/${TEST_PROP_FILE} ] ; then
    \cp -rf ${TEST_PROP_DIR}/${TEST_PROP_FILE} ${FVT_PATH}/include/hcdiag/test.properties
    check_return_flag_value $? 0 "Cleanup     : test.properties file"
    rm -f ${TEST_PROP_DIR}/${TEST_PROP_FILE}
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                hcdiag Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
