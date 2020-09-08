#! /bin/bash
#================================================================================
#
#    csm_fvt_test_dir_setup.sh
#
#  © Copyright IBM Corporation 2020. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

#---------------------------------------
# Output formatter
#---------------------------------------
line1_out=$(printf "%0.s-" {1..120})

#--------------------------------------------------------------------------------
# Usage function for -h --help options
#--------------------------------------------------------------------------------

function usage () {
echo "#--------------------------------------------------------------------------------"
echo "# Helper script to setup the CSM FVT /test directory."
echo "# This script is to be executed by the root user on the CSM MASTER node."
echo "#--------------------------------------------------------------------------------"
}

#----------------------------------------------------
# long options to short along with fixed length
#----------------------------------------------------

for arg in "$@"; do
	shift
	case "$arg" in
		--help)     set -- "$@" -h ;;
		-h)         set -- "$@" "$arg" ;;
		-*)         usage 2>>/dev/null &&
								exit 0 ;;
		*)          set -- "$@" "$arg"
	esac
done

# Default behavior
rest=false; ws=false

#--------------------------------------------
# now we can drop into the short getopts
#--------------------------------------------

OPTIND=1

while getopts "h" opt; do
	case $opt in
		h) usage && exit 0 ;;
		?) usage >&2 exit 1 ;;
	esac
done
shift $(expr $OPTIND - 1) # remove options from positional parameters

#--------------------------------------------------------------------------------
# Try to source the configuration file to get global configuration variables
#--------------------------------------------------------------------------------

if [ -f "${BASH_SOURCE%/*}/../csm_test.cfg" ]
then
	. "${BASH_SOURCE%/*}/../csm_test.cfg"
else
	echo "[Error   ] Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
	exit 1
fi

#---------------------------------------------------
# Check the user of the script and CSM MASTER node.
#---------------------------------------------------
USER=`whoami`
HOSTNAME=`hostname | cut -d . -f 1`

if [[ $USER != "root" ]] || [[ $HOSTNAME != $MASTER ]]; then
	echo "${line1_out}"
	echo "[Error   ] This script must be run as root and on the CSM MASTER: $MASTER node"
	echo "${line1_out}"
	exit 0
fi

#--------------------------
# Script output of steps
#--------------------------
echo "${line1_out}"
echo "[Info    ] Running the $0 script"
echo "${line1_out}"

#--------------------------------------------------------------------------------
# Variables
#--------------------------------------------------------------------------------
HOSTNAME=`hostname | cut -d . -f 1`

#--------------------------------------------------------
# Check if the script can be ran on this node
# If you are on the XCAT management node then you don't
# need to check the directory and files below.
#--------------------------------------------------------
if [[ "$XCATMN" != "$MASTER" ]] || [[ "$XCATMN" != "$HOSTNAME"  ]]; then
	#--------------------------------------------------------
	# First make sure that the script can run xcat commands.
	#--------------------------------------------------------
	DIR_NAME="/usr/local/bin"
	SCRIPTS="lsdef nodels nodestat ppping rinv rpower rvitals xcoll xdcp xdsh"

	for SCRIPT in $SCRIPTS; do
	SCRIPT_NAME="${DIR_NAME}/${SCRIPT}"

		# Check if the file does not exist
		if [[ ! -e "$SCRIPT_NAME" ]]; then
			echo "[Error   ] Please run the /CAST/csmtest/include/csm_fvt_xcat_setup_on_non_mn.sh script."
			echo "[Info    ] This will setup the /usr/local/bin directory with the appropriate scripts."
			echo "${line1_out}"
			exit 0
		fi
	done
else
xcat_ser_check=`service xcatd status >/dev/null 2>&1`
	if [ $? -ne 0 ]; then
		echo "[Error   ] The XCAT server is not running."
		echo "[Info    ] Please check the server (service xcatd status/restart) or configuration"
		echo "${line1_out}"
		exit 0
	fi
fi

#--------------------------------------------
# Make the CSM FVT test directories
# if they don't already exist/
#--------------------------------------------
mkdir -p /test/{archive,big_data,old_rpms,results/{buckets/{advanced,analytics,basic,BDS,error_injection,performance,timing},performance,setup,test},rpms/boost}

#------------------------------------------------------
# 1. Check to see if the /test/archive directory exists
# 2. Write the archive.sh and archive_input.sh scripts
#------------------------------------------------------
ARCH_DIR="/test/archive"
ARCH_INPUT="$ARCH_DIR/archive_input.sh"
ARCH="$ARCH_DIR/archive.sh"

if [[ ! -e $ARCH_DIR ]]; then
	echo "[Error   ] The $ARCH_DIR directory does not exist"
	echo "${line1_out}"
	exit 0
else
	if [ ! -f $ARCH_INPUT ] || [ ! -f $ARCH ]; then
		echo "[Info    ] Creating $ARCH & $ARCH_INPUT files."
		# copy over the archive_input.sh script
		echo "RESULTS_DIR=/test/results" > ${ARCH_INPUT}
		echo "ARCHIVE_DIR=/test/archive" >> ${ARCH_INPUT}
		echo "TEMP_LOG=\${ARCHIVE_DIR}/archive_tmp.log" >> ${ARCH_INPUT}
		echo "" >> ${ARCH_INPUT}
		echo "# Make current archive directory" >> ${ARCH_INPUT}
		echo "DIR=\$1" >> ${ARCH_INPUT}
		echo "CURR_ARCHIVE_DIR=\${ARCHIVE_DIR}/\${DIR}" >> ${ARCH_INPUT}
		echo "" >> ${ARCH_INPUT}
		echo "# Check current archiving directory exists.  If not, create it" >> ${ARCH_INPUT}
		echo "ls \${CURR_ARCHIVE_DIR} > \${TEMP_LOG} 2>&1" >> ${ARCH_INPUT}
		echo "if [ \$? -eq 0 ]; then" >> ${ARCH_INPUT}
		echo "        echo "Current Archive Directory \${CURR_ARCHIVE_DIR} exists..."" >> ${ARCH_INPUT}
		echo "else" >> ${ARCH_INPUT} 
		echo "        echo "Creating \${CURR_ARCHIVE_DIR}..."" >> ${ARCH_INPUT}
		echo "        mkdir \${CURR_ARCHIVE_DIR}" >> ${ARCH_INPUT}
		echo "fi" >> ${ARCH_INPUT}
		echo "" >> ${ARCH_INPUT}
		echo "# Copy bucket log files to current archive directory" >> ${ARCH_INPUT}
		echo "# TODO add options for copying files, default to all bucket logs" >> ${ARCH_INPUT}
		echo "echo "copying files from ${RESULTS_DIR}/buckets/ to ${CURR_ARCHIVE_DIR}"" >> ${ARCH_INPUT}
		echo "cp -R \${RESULTS_DIR}/buckets/ \${CURR_ARCHIVE_DIR}" >> ${ARCH_INPUT}
		echo "echo "copying files from \${RESULTS_DIR}/test to \${CURR_ARCHIVE_DIR}"" >> ${ARCH_INPUT}
		echo "cp -R \${RESULTS_DIR}/test/ \${CURR_ARCHIVE_DIR}" >> ${ARCH_INPUT}
		echo "echo "copying files from \${RESULTS_DIR}/setup to \${CURR_ARCHIVE_DIR}"" >> ${ARCH_INPUT}
		echo "cp -R \${RESULTS_DIR}/setup \${CURR_ARCHIVE_DIR}" >> ${ARCH_INPUT}
		echo "echo "copying files from \${RESULTS_DIR}/performance to \${CURR_ARCHIVE_DIR}"" >> ${ARCH_INPUT}
		echo "cp -R \${RESULTS_DIR}/performance/ \${CURR_ARCHIVE_DIR}" >> ${ARCH_INPUT}
		echo "" >> ${ARCH_INPUT}
		echo "# Clean current results directory" >> ${ARCH_INPUT}
		echo "# TODO: Add option to enable clean results logs" >> ${ARCH_INPUT}
		echo "echo "Cleaning logs in \${RESULTS_DIR}"" >> ${ARCH_INPUT}
		echo "\${RESULTS_DIR}/clean_logs.sh" >> ${ARCH_INPUT}
		echo "" >> ${ARCH_INPUT}
		echo "# Clean up temp log" >> ${ARCH_INPUT}
		echo "" rm -f ${TEMP_LOG} >> ${ARCH_INPUT}
		echo "" >> ${ARCH_INPUT}
		echo "fi" >> ${ARCH_INPUT}
		chmod 755 ${ARCH_INPUT}
 
		# copy over the archive.sh script
		echo "RESULTS_DIR=/test/results" > ${ARCH}
		echo "ARCHIVE_DIR=/test/archive" >> ${ARCH}
		echo "TEMP_LOG=\${ARCHIVE_DIR}/archive_tmp.log" >> ${ARCH}
		echo "" >> ${ARCH}
		echo "# Make current archive directory" >> ${ARCH}
		echo "# TODO: Add input, default to date" >> ${ARCH}
		echo "DATE=20$(date +%y-%m-%d)" >> ${ARCH}
		echo "CURR_ARCHIVE_DIR=\${ARCHIVE_DIR}/${DATE}" >> ${ARCH}
		echo "" >> ${ARCH}
		echo "# Check current archiving directory exists.  If not, create it" >> ${ARCH}
		echo "ls \${CURR_ARCHIVE_DIR} > \${TEMP_LOG} 2>&1" >> ${ARCH}
		echo "if [ \$? -eq 0 ]" >> ${ARCH}
		echo "then" >> ${ARCH}
		echo "        echo "Current Archive Directory \${CURR_ARCHIVE_DIR} exists..."" >> ${ARCH}
		echo "else" >> ${ARCH}
		echo "        echo "Creating \${CURR_ARCHIVE_DIR}..."" >> ${ARCH}
		echo "        mkdir \${CURR_ARCHIVE_DIR}" >> ${ARCH}
		echo "fi" >> ${ARCH}
		echo "" >> ${ARCH}
		echo "# Copy bucket log files to current archive directory" >> ${ARCH}
		echo "# TODO add options for copying files, default to all bucket logs" >> ${ARCH}
		echo "echo "copying files from \${RESULTS_DIR}/buckets/ to \${CURR_ARCHIVE_DIR}"" >> ${ARCH}
		echo "cp -R \${RESULTS_DIR}/buckets/ \${CURR_ARCHIVE_DIR}" >> ${ARCH}
		echo "echo "copying files from \${RESULTS_DIR}/test to \${CURR_ARCHIVE_DIR}"" >> ${ARCH}
		echo "cp -R \${RESULTS_DIR}/test/ \${CURR_ARCHIVE_DIR}" >> ${ARCH}
		echo "echo "copying files from \${RESULTS_DIR}/setup to \${CURR_ARCHIVE_DIR}"" >> ${ARCH}
		echo "cp -R \${RESULTS_DIR}/setup/ \${CURR_ARCHIVE_DIR}" >> ${ARCH}
		echo "echo "copying files from \${RESULTS_DIR}/performance to \${CURR_ARCHIVE_DIR}"" >> ${ARCH}
		echo "cp -R \${RESULTS_DIR}/performance/ \${CURR_ARCHIVE_DIR}" >> ${ARCH}
		echo "" >> ${ARCH}
		echo "# Clean current results directory" >> ${ARCH}
		echo "# TODO: Add option to enable clean results logs" >> ${ARCH}
		echo "echo "Cleaning logs in \${RESULTS_DIR}"" >> ${ARCH}
		echo "\${RESULTS_DIR}/clean_logs.sh" >> ${ARCH}
		echo "" >> ${ARCH}
		echo "# Clean up temp log" >> ${ARCH}
		echo "rm -f \${TEMP_LOG}" >> ${ARCH}
		chmod 755 ${ARCH}
	else
		echo "[Info    ] The $ARCH & $ARCH_INPUT files already exist."
	fi
fi

#--------------------------------------------------------
# 1. Check to see if the /test/old_rpms dictory exists
# 2. Import the old RPMS to the /test/old_rpms directory
#--------------------------------------------------------
OLD_RPMS_DIR="/test/old_rpms"
OLD_RPM_1="$OLD_RPMS/ibm-csm-api-1.0.0-9460.ppc64le.rpm"
OLD_RPM_2="$OLD_RPMS/ibm-csm-core-1.0.0-9460.ppc64le.rpm"
OLD_RPM_3="$OLD_RPMS/ibm-csm-hcdiag-1.0.0-9460.noarch.rpm"
OLD_RPM_4="$OLD_RPMS/ibm-flightlog-1.0.0-9460.ppc64le.rpm"

if [[ ! -e $OLD_RPMS_DIR ]]; then
	echo "[Error   ] The $OLD_RPMS_DIR directory does not exist"
	echo "${line1_out}"
	exit 0
else
	if [ -z "$(ls -A $OLD_RPMS_DIR)" ]; then
		echo "[Info    ] Creating OLD RPMS in $OLD_RPMS_DIR"
		xdcp ${MASTER} $OLD_RPM_1 $OLD_RPMS_DIR 2> /dev/null
		xdcp ${MASTER} $OLD_RPM_2 $OLD_RPMS_DIR 2> /dev/null
		xdcp ${MASTER} $OLD_RPM_3 $OLD_RPMS_DIR 2> /dev/null
		xdcp ${MASTER} $OLD_RPM_4 $OLD_RPMS_DIR 2> /dev/null
	else
		echo "[Info    ] The OLD RPMS already exist."
		#echo "${line1_out}"
	fi
fi

#------------------------------------------------------
# 1. Check to see if the /test/archive directory exists
# 2. Write the archive.sh and archive_input.sh scripts
#------------------------------------------------------
RESULTS_DIR="/test/results"
CLEAN_LOGS="$RESULTS_DIR/clean_logs.sh"

if [[ ! -e $RESULTS_DIR ]]; then
	echo "[Error   ] The $RESULTS_DIR directory does not exist"
	echo "${line1_out}"
	exit 0
else
	if [ ! -f $CLEAN_LOGS ]; then
		echo "[Info    ] Creating the $CLEAN_LOGS script in $RESULTS_DIR"
		# copy over the clean_logs.sh script
		echo "rm -f /test/results/test/*.log" > ${CLEAN_LOGS}
		echo "rm -f /test/results/buckets/basic/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/buckets/advanced/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/buckets/error_injection/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/buckets/timing/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/buckets/performance/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/setup/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/performance/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/buckets/BDS/*.log" >> ${CLEAN_LOGS}
		echo "rm -f /test/results/buckets/analytics/*.log" >> ${CLEAN_LOGS}
		chmod 755 ${CLEAN_LOGS}
		echo "${line1_out}"
	else
		echo "[Info    ] The $CLEAN_LOGS file already exist."
		echo "${line1_out}"
	fi
fi
