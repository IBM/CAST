#================================================================================
#   
#    buckets/error_injection/infrastructure.sh
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

# Error Injection Bucket for Infrastructure testing
# Daemon starts/stops and csm_infrastructure_health_checks for single and multiple aggregator setups

# DEVEL
# set -x

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/buckets/error_injection/infrastructure.log
CSM_PATH=/opt/ibm/csm/bin
TEMP_LOG=${LOG_PATH}/buckets/error_injection/infrastructure_tmp.log
FLAG_LOG=${LOG_PATH}/buckets/error_injection/infrastructure_flags.log
UTILIY=$(nodels utility)

if [ -f "${BASH_SOURCE%/*}/../../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "       Starting Error Injected Infrastructure Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

# Shut down daemons to start with clean slate
shutdown_daemons
check_return_exit $? 0 "Shut down daemons"

# Test Case 1: Start master daemon, health check, master daemon only daemon present
echo "Test Case 1: Test Case 1: Start master daemon, health check, master daemon only daemon present" >> ${LOG}
systemctl start csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 0 "Test Case 1: Master is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 1: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:0"
check_return_flag $? "Test Case 1: Validating Health Check output"

# Test Case 2: Start utility daemon, health check, utility daemon filled in
echo "Test Case 2: Start utility daemon, health check, utility daemon filled in" >> ${LOG}
xdsh utility "systemctl start csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 0 "Test Case 2: Utility is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 2: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:1"
check_return_flag $? "Test Case 2: Validating Health Check output"

# Test Case 3: health check from utility, same info, local listed as utility
echo "Test Case 3: health check from utility, same info, local listed as utility" >> ${LOG}
xdsh utility "${CSM_PATH}/csm_infrastructure_health_check -v" > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 3: Health check from utility"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:1" "Local_daemon: UTILITY: ${UTILITY}"
check_return_flag $? "Test Case 3: Validating Health Check output"

# Test Case 4: Stop utility daemon, health check, utility daemon unresponsive
echo "Test Case 4: Stop utility daemon, health check, utility daemon unresponsive" >> ${LOG}
xdsh utility "systemctl stop csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 1 "Test Case 4: Utility is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 4: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:1" "Unresponsive Utility Nodes: 1"
check_return_flag $? "Test Case 4: Validating Health Check output"

# Test Case 5: Start utility daemon, health check, utility daemon responsive and bounced=1
echo "Test Case 5: Start utility daemon, health check, utility daemon responsive and bounced=1" >> ${LOG}
xdsh utility "systemctl start csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 0 "Test Case 5: Utility is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 5: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:1" "UTILITY: .* (bounced=1" 
check_return_flag $? "Test Case 5: Validating Health Check output"

# Test Case 6: Stop master daemon, see utility going to disconnected
echo "Test Case 6: Stop master daemon, see utility going to disconnected" >> ${LOG}
systemctl stop csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 3 "Test Case 6: Master is inactive"
xdsh utility "cat /var/log/ibm/csm/csm_utility.log" > ${TEMP_LOG} 2>&1
check_all_output "Transition from RUNMODE_CONFIGURED to: RUNMODE_DISCONNECTED"
check_return_flag $? "Test Case 6: Utility transition to RUNMODE_DISCONNECTED"

# Test Case 7: Start master daemon, see utility coming back to life
echo "Test Case 7: Start master daemon, see utility coming back to life" >> ${LOG}
systemctl start csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 0 "Test Case 7: Master is active"
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 0 "Test Case 7: Utility is still active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 7: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:1" "UTILITY: .* (bounced=0;"
check_return_flag $? "Test Case 7: Validating Health Checkout output"

# Restart Daemons
echo "------------------------------------------------------------" >> ${LOG}
echo "Restarting Daemons..." >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
shutdown_daemons

# Test Case 8: Start master daemon, health check, nothing but master
echo "Test Case 8: Start master daemon, health check, nothing but master" >> ${LOG}
systemctl start csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 0 "Test Case 8: Master is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 8: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:0"
check_return_flag $? "Test Case 8: Validating Health Check output"

# Test Case 9: Start aggregator daemon, health check, aggregator daemon present
echo "Test Case 9: Start aggregator daemon, health check, aggregator daemon present" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 9: Aggregator is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 9: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:0"
check_return_flag $? "Test Case 9: Validating Health Check output"

# Test Case 10: Stop aggregator daemon, health check, aggregator daemon unresponsive
echo "Test Case 10: Stop aggregator daemon, health check, aggregator daemon unresponsive" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 1 "Test Case 10: Aggregator is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 10: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:0" "Unresponsive Aggregators: 1"
check_return_flag $? "Test Case 10: Validating Health Check output"

# Test Case 11: Start aggregator daemon, health check, aggregator daemon responsive and bounced=1
echo "Test Case 11: Start aggregator daemon, health check, aggregator daemon responsive and bounced=1" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 11: Aggregator is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 11: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:0" "AGGREGATOR: .* (bounced=1;"
check_return_flag $? "Test Case 11: Validating Health Check output"

# Test Case 12: Stop master daemon, see aggregator going to disconnected
echo "Test Case 12: Stop master daemon, see aggregator going to disconnected" >> ${LOG}
systemctl stop csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 3 "Test Case 12: Master is not active"
xdsh ${AGGREGATOR_A} "cat /var/log/ibm/csm/csm_aggregator.log" > ${TEMP_LOG} 2>&1
check_all_output "Transition from RUNMODE_CONFIGURED to: RUNMODE_DISCONNECTED"
check_return_flag $? "Test Case 12: Aggregator transition to RUNMODE_DISCONNECTED"

# Test Case 13: Start master daemon, see aggregator coming back to life
echo "Test Case 13: Start master daemon, see aggregator coming back to life" >> ${LOG}
systemctl start csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 0 "Test Case 13: Master is active"
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 13: Aggregator is still alive"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 13: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:0" "AGGREGATOR: .* (bounced=0;"
check_return_flag $? "Test Case 13: Validating Health Check output"

# Restart Daemons
echo "------------------------------------------------------------" >> ${LOG}
echo "Restarting Daemons..." >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
shutdown_daemons

# Test Case 14: Start master daemon, health check, nothing but master
echo "Test Case 14: Start master daemon, health check, nothing but master" >> ${LOG}
systemctl start csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 0 "Test Case 14: Master is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 14: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:0"
check_return_flag $? "Test Case 14: Validating Health Check output"

# Test Case 15: Start aggregator daemon, health check, aggregator daemon present
echo "Test Case 15: Start aggregator daemon, health check, aggregator daemon present" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 15: Aggregator is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 15: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:0"
check_return_flag $? "Test Case 15: Validating Health Check output"

# Test Case 16: Start utility daemon, health check, utility and aggregator daemon present
echo "Test Case 16: Start utility daemon, health check, utility and aggregator daemon present" >> ${LOG}
xdsh utility "systemctl start csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 0 "Test Case 16: Utility is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 16: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1"
check_return_flag $? "Test Case 16: Validating Health Check output"

# Test Case 17: Stop aggregator daemon, health check, unresponsive aggregator
echo "Test Case 17: Stop aggregator daemon, health check, unresponsive aggregator" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 1 "Test Case 17: Aggregator is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 17: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "Unresponsive Aggregators: 1"
check_return_flag $? "Test Case 17: Validating Health Check output"

# Test Case 18: Stop utility daemon, health check, unresponsive aggregator and utility
echo "Test Case 18: Stop utility daemon, health check, unresponsive aggregator and utility" >> ${LOG}
xdsh utility "systemctl stop csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 1 "Test Case 18: Utility is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 18: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "Unresponsive Aggregators: 1" "Unresponsive Utility Nodes: 1"
check_return_flag $? "Test Case 18: Validating Health Check output"

# Test Case 19: Start aggregator deamon, health check, responsive aggregator bounced=1 and unresponsive utility
echo "Test Case 19: Start aggregator deamon, health check, responsive aggregator bounced=1 and unresponsive utility" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 19: Aggregator is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 19: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "AGGREGATOR: .* (bounced=1;" "Unresponsive Utility Nodes: 1"
check_return_flag $? "Test Case 19: Validating Health Check output"

# Test Case 20: Start utility daemon, health check, responsive aggregator and utility and bounced=1 for both
echo "Test Case 20: Start utility daemon, health check, responsive aggregator and utility and bounced=1 for both" >> ${LOG}
xdsh utility "systemctl start csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 0 "Test Case 20: Utility is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 20: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "AGGREGATOR: .* (bounced=1;" "UTILITY: .* (bounced=1;"
check_return_flag $? "Test Case 20: Validating Health Check output"

# Test Case 21: Stop master daemon, health check, aggregator and utility transition to disconnected"
echo "Test Case 21: Stop master daemon, health check, aggregator and utility transition to disconnected" >> ${LOG}
systemctl stop csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 3 "Test Case 21: Master is not active"
xdsh ${AGGREGATOR_A} "cat /var/log/ibm/csm/csm_aggregator.log" > ${TEMP_LOG} 2>&1
check_all_output "Transition from RUNMODE_CONFIGURED to: RUNMODE_DISCONNECTED"
check_return_flag $? "Test Case 21: Aggregator transitions to RUNMODE_DISCONNECTED"
xdsh utility "cat /var/log/ibm/csm/csm_utility.log" > ${TEMP_LOG} 2>&1
check_all_output "Transition from RUNMODE_CONFIGURED to: RUNMODE_DISCONNECTED"
check_return_flag $? "Test Case 21: Utility transitions to RUNMODE_DISCONNECTED"

# Test Case 22: Start master daemon, health check, aggregator and utility daemon reconnect
echo "Test Case 22: Start master daemon, health check, aggregator and utility daemon reconnect" >> ${LOG}
systemctl start csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 0 "Test Case 22: Master is active"
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 0 "Test Case 22: Utility is active"
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 22: Aggregator is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 22: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "UTILITY: .* (bounced=0;" "AGGREGATOR: .* (bounced=0;"
check_return_flag $? "Test Case 22: Validating Health Check output"

# Test Case 23: Start compute daemon, health check, compute shows up
echo "Test Case 23: Start compute daemon, health check, compute shows up" >> ${LOG}
xdsh ${SINGLE_COMPUTE} "systemctl start csmd-compute"
sleep 1
xdsh ${SINGLE_COMPUTE} "systemctl is-active csmd-compute" > /dev/null
check_return_exit $? 0 "Test Case 23: Compute is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 23: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "COMPUTE: ${SINGLE_COMPUTE}"
check_return_flag $? "Test Case 23: Validating Health Check output"

# Test Case 24: Stop compute daemon, health check, compute unresponsive, RAS event for csm.status.down
echo "Test Case 24: Stop compute daemon, health check, compute unresponsive, RAS event for csm.status.down" >> ${LOG}
xdsh ${SINGLE_COMPUTE} "systemctl stop csmd-compute"
sleep 1
xdsh ${SINGLE_COMPUTE} "systemctl is-active csmd-compute" > /dev/null
check_return_exit $? 1 "Test Case 24: Compute is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 24: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "COMPUTE: ${SINGLE_COMPUTE} (bounced=1; version=n/a; link=ANY)"
check_return_flag $? "Test Case 24: Validating Health Check output"
${CSM_PATH}/csm_ras_event_query -l ${SINGLE_COMPUTE} -m "csm.status.down" > ${TEMP_LOG} 2>&1
check_all_output "Total_Records: 1"
check_return_flag $? "Test Case 24: Validating csm_ras_event_query output"

# Test Case 25: Start compute daemon, health check, compute responsive and bounced=1
echo "Test Case 25: Start compute daemon, health check, compute responsive and bounced=1" >> ${LOG}
xdsh ${SINGLE_COMPUTE} "systemctl start csmd-compute"
sleep 1
xdsh ${SINGLE_COMPUTE} "systemctl is-active csmd-compute" > /dev/null
check_return_exit $? 0 "Test Case 25: Compute is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 25: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "COMPUTE: ${SINGLE_COMPUTE} (bounced=1;" "Active: 1"
check_return_flag $? "Test Case 25: Validating Health Check output"

# Test Case 26: Stop aggregator daemon, health check, compute goes to disconnected, aggregator listed as unresponsive
echo "Test Case 26: Stop aggregator daemon, health check, compute goes to disconnected, aggregator listed as unresponsive" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 1 "Test Case 26: Aggregator is not active"
xdsh ${SINGLE_COMPUTE} "systemctl is-active csmd-compute" > /dev/null
check_return_exit $? 0 "Test Case 26: Compute is still active"
sleep 4
xdsh ${SINGLE_COMPUTE} "cat /var/log/ibm/csm/csm_compute.log" > ${TEMP_LOG} 2>&1
check_all_output "Transition from RUNMODE_CONFIGURED to: RUNMODE_DISCONNECTED"
check_return_flag $? "Test Case 26: Compute transitions to RUNMODE_DISCONNECTED"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 26: Health Check"
check_all_output "Unresponsive Aggregators: 1" "AGGREGATOR: ${AGGREGATOR_A} (bounced=1; version=n/a)"
check_return_flag $? "Test Case 26: Validating Health Check output"

# Test Case 27: Start aggregator daemon, health check, responsive compute, responsive aggregator bounced=1
echo "Test Case 27: Start aggregator daemon, health check, responsive compute, responsive aggregator bounced=1" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 27: Aggregator is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 27: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "COMPUTE: ${SINGLE_COMPUTE} (bounced=0;" "Active: 1" "AGGREGATOR: ${AGGREGATOR_A} (bounced=1;"
check_return_flag $? "Test Case 27: Validating Health Check output"

# Restart Daemons - SSL section
echo "------------------------------------------------------------" >> ${LOG}
echo "Restarting Daemons..." >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
shutdown_daemons

echo "SSL Section" >> ${LOG}
${FVT_PATH}/tools/enable_ssl.sh

# Test Case 28: Start master daemon, health check, nothing but master
echo "Test Case 28: Start master daemon, health check, nothing but master" >> ${LOG}
systemctl start csmd-master
sleep 1
systemctl is-active csmd-master > /dev/null
check_return_exit $? 0 "Test Case 28: Master is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 28: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:0" "Utility Nodes:0"
check_return_flag $? "Test Case 28: Validating Health Check output"

# Test Case 29: Start aggregator daemon, health check, aggregator daemon present
echo "Test Case 29: Start aggregator daemon, health check, aggregator daemon present" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 0 "Test Case 29: Aggregator is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 29: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:0"
check_return_flag $? "Test Case 29: Validating Health Check output"

# Test Case 30: Start utility daemon, health check, utility and aggregator daemon present
echo "Test Case 30: Start utility daemon, health check, utility and aggregator daemon present" >> ${LOG}
xdsh utility "systemctl start csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 0 "Test Case 30: Utility is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 30: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1"
check_return_flag $? "Test Case 30: Validating Health Check output"

# Test Case 31: Start compute daemon, health check, compute is present 
echo "Test Case 31: Start compute deamon, health check, compute is present" >> ${LOG}
xdsh ${SINGLE_COMPUTE} "systemctl start csmd-compute"
sleep 1
xdsh ${SINGLE_COMPUTE} "systemctl is-active csmd-compute" > /dev/null
check_return_exit $? 0 "Test Case 31: Compute is active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 31: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "COMPUTE: ${SINGLE_COMPUTE}"
check_return_flag $? "Test Case 31: Validating Health Check output"

# Test Case 32: Stop compute daemon, health check, compute unresponsive, RAS event for csm.status.down
echo "Test Case 32: Stop compute daemon, health check, compute unresponsive, RAS event for csm.status.down" >> ${LOG}
xdsh ${SINGLE_COMPUTE} "systemctl stop csmd-compute"
sleep 1
xdsh ${SINGLE_COMPUTE} "systemctl is-active csmd-compute" > /dev/null
check_return_exit $? 1 "Test Case 32: Compute is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 32: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Utility Nodes:1" "COMPUTE: ${SINGLE_COMPUTE} (bounced=1; version=n/a; link=ANY)"
check_return_flag $? "Test Case 32: Validating Health Check output"
${CSM_PATH}/csm_ras_event_query -l ${SINGLE_COMPUTE} -m "csm.status.down" > ${TEMP_LOG} 2>&1
check_all_output "Total_Records: 1"
check_return_flag $? "Test Case 32: Validating csm_ras_event_query output"

# Test Case 33: Stop utility daemon, health check, utility disconnected
echo "Test Case 33: Stop utility daemon, health check, utility disconnected" >> ${LOG}
xdsh utility "systemctl stop csmd-utility"
sleep 1
xdsh utility "systemctl is-active csmd-utility" > /dev/null
check_return_exit $? 1 "Test Case 33: Utility is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 33: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Unresponsive Utility Nodes: 1" "COMPUTE: ${SINGLE_COMPUTE} (bounced=1; version=n/a; link=ANY)"

# Test Case 34: Stop aggregator deamon, health check, aggregator disconnected
echo "Test Case 34: Stop aggregator daemon, health check, aggregator disconnected" >> ${LOG}
xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator"
sleep 1
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
check_return_exit $? 1 "Test Case 34: Aggregator is not active"
${CSM_PATH}/csm_infrastructure_health_check -v > ${TEMP_LOG} 2>&1
check_return_exit $? 0 "Test Case 34: Health Check"
check_all_output "MASTER: ${MASTER}" "Aggregators:1" "Unresponsive Utility Nodes: 1" "Unresponsive Aggregators: 1"  

# Clean up Temp Log
rm -f ${TEMP_LOG}

echo "------------------------------------------------------------" >> ${LOG}
echo "       Error Injected Infrastructure Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Failed test cases:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

exit 0
