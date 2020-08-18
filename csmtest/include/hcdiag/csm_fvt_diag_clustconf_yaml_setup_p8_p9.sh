#================================================================================
#
#    csm_fvt_diag_clustconf_yaml_setup_p8_p9.sh
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
echo "# Helper script for setting up some of the specific"
echo "# CSM FVT Regression related to the HCDiag test cases for the clustconf.yaml file."
echo "# This script is designed to be executed on an XCAT managment node within the"
echo "# /CAST/csmtest/include/hcdiag directory."
echo "# *** The XCAT daemon also has to be active.***"
echo "# XCATMN variable also needs to be defined in the /CAST/csmtest/csm_test.cfg file"
echo "#--------------------------------------------------------------------------------"
echo "# 1.  Node range of compute nodes that as designated for CSM FVT regression."
echo "# 2.  Number of CPUS"
echo "# 3.  Total Memory"
echo "# 4.  Total banks associated with each of the computes."
echo "# 5.  Total banks sizes associated with the computes."
echo "# 6.  Clock frequency measured in GHz (Max)."
echo "# 7.  Clock frequency measured in GHz (Min)."
echo "# 8.  Firmware Version versions associated with the computes."
echo "# 9.  IB Related: slot_rx attribute with the mellanox switch(es)."
echo "# 10. Board ID associated with the switch(es)."
echo "# 11. Firware Version version of the mellanox adapter."
echo "# 12. OS Related: Pretty name associated with the computes."
echo "# 13. Kernel Release version associated with the computes."
echo "# 14. CSM core software installed on the management node (MASTER)"
echo "# 15. Threshold for sensor temperature testing (High) (Hard coded values)."
echo "# 16. Threshold for sensor temperature testing (Low) (Hard coded values)."
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

#--------------------------------------------
# Check the user of the script
#--------------------------------------------
USER=`whoami`

if [[ $USER != "root" ]]; then
   echo "${line1_out}"
   echo "[Error   ] This script must be run as root"
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
# Try to source the configuration file to get global configuration variables
#--------------------------------------------------------------------------------

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        #echo "${line1_out}"
        echo "[Error   ] Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        echo "${line1_out}"
        exit 1
fi

#--------------------------------------------------------------------------------
# Variables
#--------------------------------------------------------------------------------
mydate=$(date +'%Y')
current_user=`id -u -n`
ORIG_FILE="$FVT_PATH/include/hcdiag/clustconf.yaml"
TEMP_DIR="/tmp/csm_fvt_hcdiag_temp/tmp_clustconf_yaml"
BACKUP_DIR="/tmp/csm_fvt_hcdiag_backup/clustconf_yaml_backup"

#----------------------------------------------------------------
# Below makes the directory if it does not exist
#----------------------------------------------------------------

    if [[ ! -e $TEMP_DIR ]] || [[ ! -e $BACKUP_DIR ]]; then
        mkdir -p $TEMP_DIR $BACKUP_DIR 2>>/dev/null

        if [ $? -ne 0 ]; then
            echo "[Error   ] make directory failed for: $TEMP_DIR" & $BACKUP_DIR 2>>/dev/null
            echo "[Info    ] User: $current_user does not have permission to write to this directory"
            echo "[Info    ] Please specify a valid directory"
            echo "[Info    ] Or log in as the appropriate user"
            echo "${line1_out}"
            exit 1
        else
            chown root:root {$TEMP_DIR,$BACKUP_DIR}
            chmod 755 {$TEMP_DIR,$BACKUP_DIR}
        fi
    elif [[ ! -d $TEMP_DIR ]] || [[ ! -d $BACKUP_DIR ]]; then
        echo "[Info   ] $TEMP_DIR and $BACKUP_DIR already exists but is not a directory" 1>&2
        echo "${line1_out}"
        exit 1
    fi

#----------------------------------------------------------------
# Clean up directory if anything is left over from previous runs
#----------------------------------------------------------------
if ls $TEMP_DIR/clustconf* >/dev/null 2>&1;then
    rm -rf $TEMP_DIR/clustconf*;
fi

BACKUP_FILE="$TEMP_DIR/clustconf.yaml"
MOVE_FILE=`cp $ORIG_FILE $BACKUP_FILE.$(date +%s)`

FILE=$TEMP_DIR/clustconf.yaml
xdsh_output="===================================="
HOSTNAME=`hostname | cut -d . -f 1`

#-------------------------------------------------
# Read in the compute nodes and store in an array
#-------------------------------------------------
IFS=', ' read -r -a comp_node_array <<< "$COMPUTE_NODES"

#----------------------------------------------------------------
# Check if the XCAT daemon is active on the XCAT management node
#----------------------------------------------------------------
xdsh $XCATMN "systemctl is-active xcatd" > /dev/null 2>&1

    if [ $? -ne 0 ]; then
            echo "[Error   ] XCAT daemon is not running."
            echo "${line1_out}"
	    echo "[Info    ] 1. Make sure XCAT is installed. (Check the xCAT version: lsxcatd -a)"
            echo "[Info    ] 2. Check the service of the XCAT daemon on the XCAT management node."
	    echo "[Info    ]    run: systemctl status xcatd"
	    echo "[Info    ] 3. If the XCAT daemon is down, then the user can try to start the service."
            echo "[Info    ]    run: systemctl start xcatd"
            echo "[Info    ] 4. If none of the solutions above work then:"
            echo "[Info    ]    a. try installing XCAT"
            echo "[Info    ]    b. contact a system admin"
            echo "[Info    ]    c. contact an XCAT team member."
            echo "${line1_out}"
            rm -rf $TEMP_DIR/*
            exit 0
    fi

#------------------------------------------
# Check to see if "xcatdebugmode" is on
# If so then change the definition to "0"
#------------------------------------------
check_xcat_debug=`xdsh $XCATMN "tabdump site | grep xcatdebug"`

IFS=', ' read -r -a x_array <<< "$check_xcat_debug"

if  [ "${x_array[1]}" != "\"0"\" ]; then
    chdef -t site clustersite xcatdebugmode=0 > /dev/null 2>&1
fi

#--------------------------------------------------------------------------------
# This create the clustconfg.yaml file
#--------------------------------------------------------------------------------

echo "#================================================================================" > ${FILE}
echo "#" >> ${FILE}
echo "#    csm_fvt_diag_clustconf_yaml_setup_p8_p9.sh" >> ${FILE}
echo "#" >> ${FILE}
echo "#  © Copyright IBM Corporation $mydate. All Rights Reserved" >> ${FILE}
echo "#" >> ${FILE}
echo "#    This program is licensed under the terms of the Eclipse Public License" >> ${FILE}
echo "#    v1.0 as published by the Eclipse Foundation and available at" >> ${FILE}
echo "#    http://www.eclipse.org/legal/epl-v10.html" >> ${FILE}
echo "#" >> ${FILE}
echo "#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure" >> ${FILE}
echo "#    restricted by GSA ADP Schedule Contract with IBM Corp." >> ${FILE}
echo "#" >> ${FILE}
echo "#================================================================================" >> ${FILE}
echo "" >> ${FILE}
echo "# node specific information, make it work like a " >> ${FILE}
echo "# bash case statement." >> ${FILE}
echo "# note very little of this information is actually used yet." >> ${FILE}
echo "# so this is a placeholder for when we get all the tests to pick it up" >> ${FILE}
echo "node_info:" >> ${FILE}

#-------------------------------------------------------------------------------------------------
# This is where the loop begins for each "COMPUTE_NODES" that's defined in the csm-test.cfg file
#-------------------------------------------------------------------------------------------------
for i in "${comp_node_array[@]}"
do

#-------------------------------------------------------------------------------------------------
# These are the environmental varibles related to the hardware details
#-------------------------------------------------------------------------------------------------
ncpus=`xdsh ${i} "grep processor /proc/cpuinfo | wc -l" | xcoll | grep -v ${i} | grep -v $xdsh_output`
total_memory=`xdsh ${i} "lshw -quiet -class memory | grep -A 3 '*-memory' | grep -Po 'size: \K[^ ]+' | sed 's/[A-Za-z]*//g'" | xcoll | grep -v ${i} | grep -v $xdsh_output`
total_banks=`xdsh ${i} "sudo lshw -quiet -class memory | egrep '*-bank:' | wc -l" | xcoll | grep -v ${i} | grep -v $xdsh_output`
bank_size=`xdsh ${i} "lshw -quiet -class memory | grep -Po '          size: \K[^ ]+' | sed 's/[A-Za-z]*//g'" | sort -u | xcoll | grep -v ${i} | grep -v $xdsh_output`
clock_min=`xdsh ${i} "cpupower frequency-info | grep 'hardware limits:'" | sort | xcoll | grep -v ${i} | grep -v $xdsh_output | awk '{print $3}'`
clock_max=`xdsh ${i} "cpupower frequency-info | grep 'hardware limits:'" | sort | xcoll | grep -v ${i} | grep -v $xdsh_output | awk '{print $6}'`
firm_array=`/opt/xcat/bin/rinv \"${i}\" firm | xcoll | grep -v \"${i}\" | grep -v $xdsh_output`
IFS=$'\n' read -a firm_array -d '' <<< "${firm_array[0]}"
unset firm_array[0]
slot_rx=`xdsh ${i} "lspci |grep Mellanox" | xcoll | grep -v ${i} | grep -v $xdsh_output | awk -vORS=, '{ print $1 }' | sed 's/,$/\n/'`
board_id=`xdsh ${i} "ibv_devinfo|egrep 'fw_ver|board' | sort -u" | xcoll | grep -v ${i} | grep -v $xdsh_output | awk 'NR==1{print}' | awk '{print $2}'`
ib_fw=`xdsh ${i} "ibv_devinfo|egrep 'fw_ver|board' | sort -u" | xcoll | grep -v ${i} | grep -v $xdsh_output | awk 'NR==2{print}' | awk '{print $2}'`
pretty_name=`xdsh ${i} "cat /etc/os-release | grep PRETTY_NAME" | sort | xcoll | grep -v ${i} | grep -v $xdsh_output | cut -d'"' -f2`
kernel_release=`xdsh ${i} "uname -r" | sort | xcoll | grep -v ${i} | grep -v $xdsh_output`
csm_rpm_version=`xdsh $MASTER "rpm -qa | grep ibm-csm-core" | awk '{ print substr( $2, 14, 10 ) }'`

echo "[Info    ] Generating the HCDiag environmental data for COMPUTE_NODE:    '$i'"
echo "  - case: (${i})" >> ${FILE}
echo "    ncpus: ${ncpus}" >> ${FILE}
echo "    memory: " >> ${FILE}
echo "      total: ${total_memory}" >> ${FILE}
echo "      banks: ${total_banks}" >> ${FILE}
echo "      bank_size:  ${bank_size}" >> ${FILE}
echo "    clock:   " >> ${FILE}
echo "      max: ${clock_max}" >> ${FILE}
echo "      min: ${clock_min}" >> ${FILE}
echo "    firmware: " >> ${FILE}
echo "      name: \"unknown version\"" >> ${FILE}
echo "      versions:" >> ${FILE}
printf "        - '%s'\n" "${firm_array[@]}" >> ${FILE}
echo "" >> ${FILE}
echo "    ib:" >> ${FILE}
echo "      slot_rx: \"${slot_rx} \"" >> ${FILE}
echo "      board_id: \"${board_id}\"" >> ${FILE}
echo "      firmware: \"${ib_fw}\"" >> ${FILE}
echo "    os:" >> ${FILE}
echo "      pretty_name: \"${pretty_name}\"" >> ${FILE}
echo "    kernel:" >> ${FILE}
echo "      release: \"${kernel_release}\"" >> ${FILE}
echo "" >> ${FILE}
echo "    software:" >> ${FILE}
echo "        - ibm-csm: ${csm_rpm_version}" >> ${FILE}
echo "    temp:" >> ${FILE}
echo "      celsius_high: \"100.0\"" >> ${FILE}
echo "      celsius_low: \"14.0\"" >> ${FILE}
echo "" >> ${FILE}
done

#--------------------------------------------------------------------------------
# Check and compare the backup file with the newly generated file.
#--------------------------------------------------------------------------------
if cmp -s $TEMP_DIR/clustconf.yaml $TEMP_DIR/clustconf.yaml.* > /dev/null 2>&1; then
    echo "${line1_out}"
    echo "[Info    ] The cluster.yaml file is the same as the newly generated script"
    echo "[Info    ] No additional steps needed."
    rm -rf $TEMP_DIR/*
else
    echo "${line1_out}"
    echo "[Info    ] The cluster.yaml file is different from the newly generated script"
    echo "${line1_out}"
    echo "[Info    ] 1. The newly generated cluster.yaml file will replace the existing one."
    echo "[Info    ] 2. The previous cluster.yaml file will saved to the following dir: $BACKUP_DIR"
    mv $TEMP_DIR/clustconf.yaml.* $BACKUP_DIR
    mv $TEMP_DIR/clustconf.yaml $FVT_PATH/include/hcdiag/clustconf.yaml
fi

echo "${line1_out}"
exit 0
