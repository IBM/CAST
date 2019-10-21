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
MEMORY_LOG=${LOG_PATH}/performance/memory_usage.log
SEGFAULT_LOG=${LOG_PATH}/performance/segfault.log

segfault_scan () {
	bucket_type=$1
	bucket_name=$2

	# Collect CSM logs
	xdcp ${MASTER} -P /var/log/ibm/csm/csm_master.log ${LOG_PATH}/performance/
	xdcp ${AGGREGATOR_A} -P /var/log/ibm/csm/csm_aggregator.log ${LOG_PATH}/performance/
	xdcp ${AGGREGATOR_B} -P /var/log/ibm/csm/csm_aggregator.log ${LOG_PATH}/performance/
	xdcp utility -P /var/log/ibm/csm/csm_utility.log ${LOG_PATH}/performance/
	xdcp csm_comp -P /var/log/ibm/csm/csm_compute.log ${LOG_PATH}/performance/

	# Grep for segfault
	# Exit if found
	grep segfault ${LOG_PATH}/performance/*
	rc=$?
	if [ "$rc" -eq 0 ]
	then
		echo "SEGFAULT DETECTED" >> ${LOG_PATH}/buckets/${bucket_type}/${bucket_name}.log
		exit 1
	fi
}

run_bucket () {
	bucket_type=$1
	bucket_name=$2
	
	# Collect memory usage before bucket
	memory_usage=`ps -eo rss -o args | grep "csm[d]\b" | grep master | cut -d ' ' -f 1`
	printf "%-40s %10s\n" "${bucket_type} ${bucket_name}" "BEFORE: ${memory_usage}" >> ${MEMORY_LOG}

	# Determine bash or python bucket
	bash_bucket="${FVT_PATH}/buckets/${bucket_type}/${bucket_name}.sh"
        python_bucket="${FVT_PATH}/buckets/${bucket_type}/${bucket_name}.py"

        if [ -e ${bash_bucket} ]
        then
                full_bucket="${bash_bucket}"
        elif [ -e ${python_bucket} ]
        then
                full_bucket="${python_bucket}"
        else
                echo "error - could not find ${bucket_type} ${bucket_name} bucket"
                exit 1
        fi

	# Run bucket script
	${full_bucket}
	rc=$?
	if [ "$rc" -ne 0 ]
	then
		exit 1
	fi

	# Collect memory usage after bucket
	memory_usage=`ps -eo rss -o args | grep "csm[d]\b" | grep master | cut -d ' ' -f 1`
	printf "%-40s %10s\n" "${bucket_type} ${bucket_name}" "AFTER:  ${memory_usage}" >> ${MEMORY_LOG}

	# Segfault scan
	segfault_scan ${bucket_type} ${bucket_name}
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
run_bucket "basic" "soft_failure_recovery"
${FVT_PATH}/tools/dual_aggregator/shutdown_daemons.sh
sleep 5
${FVT_PATH}/setup/csm_setup.sh
run_bucket "error_injection" "infrastructure"
run_bucket "error_injection" "versioning"
${FVT_PATH}/setup/csm_uninstall.sh
${FVT_PATH}/setup/csm_install.sh
run_bucket "basic" "python_libraries"
run_bucket "advanced" "allocation_timing"
run_bucket "BDS" "python_scripts"

## TODO DON'T USE THIS IF USER AND SECOND_USER ARE NOT SET AND CONFIGURED!
#run_bucket "basic" "pamd"
