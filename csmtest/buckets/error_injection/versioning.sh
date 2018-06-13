#================================================================================
#   
#    buckets/error_injection/versioning.sh
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

# Error Injection bucket for versioning checking
# install downlevel csm on a compute node

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/error_injection/versioning.log
CSM_PATH=/opt/ibm/csm/bin
OLD_INSTALL_DIR=/test/old_rpms
TEMP_LOG=${LOG_PATH}/buckets/error_injection/versioning_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/error_injection/versioning_flags.log

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
	. "${BASH_SOURCE%/*}/../../include/functions.sh"
else
	echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "       Starting Error Injected Versioning Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Fresh start
shutdown_daemons
check_return_exit $? 0 "Shutting down daemons"

# Delete current rpms from single compute node
xdsh ${SINGLE_COMPUTE} "rpm -e ibm-flightlog-1.* ibm-csm-api-* ibm-csm-core-* ibm-csm-hcdiag-*" > ${TEMP_LOG} 2>&1
check_return_flag $? "Deleting current rpms from ${SINGLE_COMPUTE}"

# Download old RPMs
xdcp ${SINGLE_COMPUTE} -R ${OLD_INSTALL_DIR} /root/ > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Downloading old RPMs to ${SINGLE_COMPUTE}"

# Install old RPMs
xdsh ${SINGLE_COMPUTE} "rpm -ivh /root/old_rpms/ibm-flightlog-1.*" > ${TEMP_LOG} 2>&1
check_return_flag $? "Installing old flightlog rpm on ${SINGLE_COMPUTE}"
xdsh ${SINGLE_COMPUTE} "rpm -ivh /root/old_rpms/ibm-csm-core-*" > ${TEMP_LOG} 2>&1
check_return_flag $? "Installing old csm-core rpm on ${SINGLE_COMPUTE}"
xdsh ${SINGLE_COMPUTE} "rpm -ivh /root/old_rpms/ibm-csm-api-*" > ${TEMP_LOG} 2>&1
check_return_flag $? "Installing old csm-api rpm on ${SINGLE_COMPUTE}"
xdsh ${SINGLE_COMPUTE} "rpm -ivh /root/old_rpms/ibm-csm-hcdiag-*" > ${TEMP_LOG} 2>&1
check_return_flag $? "Installing old csm-hcdiag rpm on ${SINGLE_COMPUTE}"

# Start daemons
systemctl start csmd-master > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Starting Master"
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Starting Aggregator"
xdsh utility "systemctl start csmd-utility" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Starting Utility"
xdsh ${SINGLE_COMPUTE} "systemctl start csmd-compute" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Starting Compute"

# Test Case 1: Check Master status
systemctl status csmd-master > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Check Master status"

sleep 5
# Test Case 2: Check Compute is not active
xdsh ${SINGLE_COMPUTE} "systemctl is-active csmd-compute" > ${TEMP_LOG} 2>&1
check_return_exit $? 1 "Test Case 2: Check Compute is not active"

# Test Case 3: Check Compute log for VERSION MISTMATCH
sleep 10
xdsh ${SINGLE_COMPUTE} "cat /var/log/ibm/csm/csm_compute.log" > ${TEMP_LOG} 2>&1
check_all_output "VERSION MISMATCH" "Transition from RUNMODE_CLEANUP to: RUNMODE_EXIT Reason: EXIT"
check_return_flag $? "Test Case 3: Check Compute log for VERSION MISTMATCH"

echo "------------------------------------------------------------" >> ${LOG}
echo "       Error Injected Versioning Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Cleanup
rm -f ${TEMP_LOG}
exit 0
