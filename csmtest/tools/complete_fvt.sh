#================================================================================
#   
#    tools/complete_fvt.sh
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

if [ -f "${BASH_SOURCE%/*}/../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

if [ -f "${BASH_SOURCE%/*}/../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
	exit 1
fi

source /etc/profile.d/xcat.sh

check_return () {
	if [ $1 -ne 0 ]
	then
		exit 1
	fi
}


${FVT_PATH}/buckets/basic/node.sh
check_return $?
${FVT_PATH}/buckets/basic/allocation.sh
check_return $?
${FVT_PATH}/buckets/basic/jsrun_cmd.sh
check_return $?
${FVT_PATH}/buckets/basic/ras.sh
check_return $?
${FVT_PATH}/buckets/basic/step.sh
check_return $?
${FVT_PATH}/buckets/basic/bb.sh
check_return $?
${FVT_PATH}/buckets/basic/db_script.sh
check_return $?
${FVT_PATH}/buckets/basic/ib_inventory.sh
check_return $?
${FVT_PATH}/buckets/basic/switch_inventory.sh
check_return $?
${FVT_PATH}/buckets/basic/inventory_collection.sh
check_return $?
${FVT_PATH}/buckets/basic/compute_node.sh
check_return $?
${FVT_PATH}/buckets/advanced/allocation.sh
check_return $?
${FVT_PATH}/buckets/error_injection/allocation.sh
check_return $?
${FVT_PATH}/buckets/error_injection/bb.sh
check_return $?
${FVT_PATH}/buckets/error_injection/node.sh
check_return $?
${FVT_PATH}/buckets/error_injection/ib_inventory.sh
check_return $?
${FVT_PATH}/buckets/error_injection/switch_inventory.sh
check_return $?
${FVT_PATH}/buckets/error_injection/messaging.sh
check_return $?
#${FVT_PATH}/buckets/timing/allocation.sh
#check_return $?
${FVT_PATH}/buckets/basic/hcdiag.sh
check_return $?
${FVT_PATH}/buckets/basic/csm_ctrl_cmd.sh
check_return $?
${FVT_PATH}/tools/dual_aggregator/shutdown_daemons.sh
sleep 5
${FVT_PATH}/setup/csm_setup.sh
${FVT_PATH}/buckets/error_injection/infrastructure.sh
check_return $?
${FVT_PATH}/buckets/error_injection/versioning.sh
check_return $?
${FVT_PATH}/setup/csm_setup.sh
