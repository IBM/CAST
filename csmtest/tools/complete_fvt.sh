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

run_bucket () {
	bucket_type=$1
	bucket_name=$2
	${FVT_PATH}/buckets/${bucket_type}/${bucket_name}.sh
	rc=$?
	if [ "$rc" -ne 0 ]
	then
		exit 1
	fi
}

run_bucket "basic" "node"
run_bucket "basic" "allocation"
run_bucket "basic" "jsrun_cmd"
run_bucket "basic" "ras"
run_bucket "basic" "step"
run_bucket "basic" "bb"
run_bucket "basic" "db_script"
run_bucket "basic" "ib_inventory"
run_bucket "basic" "switch_inventory"
run_bucket "basic" "inventory_collection"
run_bucket "basic" "compute_node"
run_bucket "advanced" "allocation"
run_bucket "error_injection" "allocation"
run_bucket "error_injection" "bb"
run_bucket "error_injection" "node"
run_bucket "error_injection" "ib_inventory"
run_bucket "error_injection" "switch_inventory"
run_bucket "error_injection" "messaging"
run_bucket "basic" "hcdiag"
run_bucket "basic" "csm_ctrl_cmd"
${FVT_PATH}/tools/dual_aggregator/shutdown_daemons.sh
sleep 5
${FVT_PATH}/setup/csm_setup.sh
run_bucket "error_injection" "infrastructure"
run_bucket "error_injection" "versioning"
${FVT_PATH}/setup/csm_uninstall.sh
${FVT_PATH}/setup/csm_install.sh
run_bucket "basic" "python_libraries"
