#================================================================================
#
#    csm_fvt_diag_test_properties_setup_p8_p9.sh
#
#  © Copyright IBM Corporation 2020-2021. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

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
echo "# CSM FVT Regression related to the HCDiag test cases for the test.properties file"
echo "# related to POWER8 or POWER9 node types."
echo "# *** The XCAT daemon also has to be active.***"
echo "# XCATMN variable also needs to be defined in the /CAST/csmtest/csm_test.cfg file"
echo "#--------------------------------------------------------------------------------"
echo "# 1.  This script will update the smt (Simultaneous Multithreading)."
echo "#     based on the node type (POWER8 or POWER9)"
echo "#     (POWER8 = args 8)"
echo "#     (POWER9 = args 4)"
echo "#     SMT in numeric form. Default is 4 in the hcdiag framework."
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
        echo "[Error   ] Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

#--------------------------------------------------------------------------------
# Variables
#--------------------------------------------------------------------------------
mydate=$(date +'%Y')
current_user=`id -u -n`
ORIG_FILE="$FVT_PATH/include/hcdiag/test.properties"
TEMP_DIR="/tmp/csm_fvt_hcdiag_temp/tmp_test"
BACKUP_DIR="/tmp/csm_fvt_hcdiag_backup/test_properties_backup"

#----------------------------------------------------------------
# Clean up directory if anything is left over from previous runs
#----------------------------------------------------------------
if ls $TEMP_DIR/* >/dev/null 2>&1;then
    rm -rf $TEMP_DIR/*;
fi

#----------------------------------------------------------------
# Below makes the directory if it does not exist
#----------------------------------------------------------------

    if [[ ! -e $TEMP_DIR ]] || [[ ! -e $BACKUP_DIR ]]; then
        mkdir -p $TEMP_DIR $BACKUP_DIR 2>>/dev/null

        if [ $? -ne 0 ]; then
            #echo "${line1_out}"
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

#----------------------------------------------
# Backup file directory and
# Copy the original file over as a backup.
# Define the temp file name.
#----------------------------------------------
BACKUP_FILE="$TEMP_DIR/test.properties"
MOVE_FILE=`cp $ORIG_FILE $BACKUP_FILE.$(date +%s)`
FILE=$TEMP_DIR/test.properties

#----------------------------------------------------------------
# Check if the XCAT daemon is active on the XCAT management node
#----------------------------------------------------------------
xdsh $XCATMN "systemctl is-active xcatd" > /dev/null 2>&1

    if [ $? -ne 0 ]; then
        #echo "${line1_out}"
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

#------------------------------------------------
# Checks to see is the node(s) are active/online
# Compare the COMPUTE_NODE list against the
# defined nodes on the XCATMN.
#------------------------------------------------
node_type=`cat /proc/cpuinfo | grep 'cpu' | sort -u | awk -F'[, ]' '{print $2}' | cut --complement -c7-15`
active_nodes=$(nodestat all | grep "sshd" | awk '{ print $1}' | cut -f1 -d":" | awk 'BEGIN{ORS=","}1')

if [ -z "${active_nodes##*$COMPUTE_NODES*}" ]; then
    #echo "${line1_out}"
    echo "[Error   ] Please check the status of the compute node(s)."
    echo "[Info    ] One or more might be off line."
    echo "$active_nodes"
    echo "$COMPUTE_NODES"
    echo "${line1_out}"
    rm -rf $TEMP_DIR/*
    exit 0
fi

#-------------------------------------------------------------
# Variables to assist in the replacment process.
#-------------------------------------------------------------
# Check the COMPUTE_NODES smt values
# Check to ensure the return count is not -gt 1.
# Find the pattern in the test.properties file to replace.
#-------------------------------------------------------------
smt_set=`xdsh $COMPUTE_NODES "/usr/sbin/ppc64_cpu --smt | cut -d '=' -f2" | xcoll | grep -v $COMPUTE_NODES | grep -v ==================================== | grep -v "No route to host" | grep -v compute | awk 'NR>1 {print last} {last=$0}'`
wc_smt_set=$(echo -n $smt_set | wc -w)
smt_tp_file=`grep -A 3 "\[tests.chk-smt\\b" $ORIG_FILE | awk 'FNR == 4 {print}' | awk '{ print $3}'`

if [ "$wc_smt_set" -gt "1" ]; then
    #echo "${line1_out}"
    echo "[Error   ] The Nodes seem to have differnt smt settings."
    echo "[Info    ] Please check to make sure the nodes are consistently configured."
    echo "${line1_out}"
    rm -rf $TEMP_DIR/*
    exit 0
fi

#------------------------------------------------------
# Determine if it is POWER8 or POWER9
# 1. Check the COMPUTE_NODES smt values
# 2. Check to ensure the return count is not -gt 1. 
#------------------------------------------------------
if [ $node_type == "POWER8" ]; then
        if [[ $smt_set != $smt_tp_file ]]; then
            #echo "${line1_out}"
            echo "[Info    ] This is a POWER 8 node."
            echo "[Info    ] The smt-mode value $smt_tp_file needs to be set to: 8"
            cp $ORIG_FILE $BACKUP_FILE 
            line_number=$(grep -n -A 3 "\[tests.chk-smt\\b" $BACKUP_FILE | awk 'FNR == 4 {print}' | awk '{sub(/-.*/,""); print}')
            sed -i "${line_number}s/args        = $smt_tp_file/args        = $smt_set/" $BACKUP_FILE
        else
            #echo "${line1_out}"
            echo "[Info    ] This is a POWER 8 node."
            echo "[Info    ] No action is needed the file is the same"
            echo "${line1_out}"
            rm -rf $TEMP_DIR/*
            exit 0
        fi
elif [ $node_type == "POWER9" ]; then
        if [[ $smt_set != $smt_tp_file ]]; then
            #echo "${line1_out}"
            echo "[Info    ] This is a POWER 9 node."
            echo "[Info    ] The smt-mode value $smt_tp_file needs to be set to: 4"
            cp $ORIG_FILE $BACKUP_FILE 
            line_number=$(grep -n -A 3 "\[tests.chk-smt\\b" $BACKUP_FILE | awk 'FNR == 4 {print}' | awk '{sub(/-.*/,""); print}')
            sed -i "${line_number}s/args        = $smt_tp_file/args        = $smt_set/" $BACKUP_FILE
        else
            #echo "${line1_out}"
            echo "[Info    ] This is a POWER 9 node."
            echo "[Info    ] No action is needed the file is the same"
            echo "${line1_out}"
            rm -rf $TEMP_DIR/*
            exit 0
        fi
fi

#--------------------------------------------------------------------------------
# Check and compare the backup file with the newly generated file.
#--------------------------------------------------------------------------------
if cmp -s $TEMP_DIR/test.properties $TEMP_DIR/test.properties.* > /dev/null 2>&1; then
    #echo "${line1_out}"
    echo "[Info    ] The test.properties file is the same as the newly generated script"
    echo "[Info    ] No additional steps needed."
    rm -rf $TEMP_DIR/*
else
    #echo "${line1_out}"
    echo "[Info    ] The test.properties file is different from the newly generated script"
    echo "${line1_out}"
    echo "[Info    ] 1. The newly generated test.properties file will replace the existing one."
    echo "[Info    ] 2. The previous test.properties file will saved to the following dir: $BACKUP_DIR"
    mv $TEMP_DIR/test.properties.* $BACKUP_DIR
    mv $TEMP_DIR/test.properties $FVT_PATH/include/hcdiag/test.properties
fi

echo "${line1_out}"
exit 0
