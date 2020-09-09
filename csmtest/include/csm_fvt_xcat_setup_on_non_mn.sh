#! /bin/bash
#================================================================================
#
#    csm_fvt_xcat_setup_for_csm_master_on_non_xcatmn.sh
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
echo "# Helper script to allow XCAT commands to be executable on non"
echo "# XCAT management nodes (CSM MASTER NODE)"
echo "# *** The XCAT daemon also has to be active.***"
echo "# XCATMN variable also needs to be defined in the"
echo "# /CAST/csmtest/csm_test.cfg file"
echo "# This script is to be executed by the root user"
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

#----------------------------------------------------------------
# Create the /etc/profile.d/xcat.sh script if it does not exist.
#----------------------------------------------------------------
ETC_PROFILE_D_DIR="/etc/profile.d/"
CHECK_XCATSH=`ls -l /etc/profile.d/xcat.sh >/dev/null 2>&1`
if [ $? -ne 0 ]; then 
    echo "[Info    ] Creating the /etc/profile.d/xcat.sh file."
    echo "CATROOT=/opt/xcat" > $ETC_PROFILE_D_DIR/xcat.sh
    echo "PATH=\$XCATROOT/bin:\$XCATROOT/sbin:\$XCATROOT/share/xcat/tools:\$PATH" >> $ETC_PROFILE_D_DIR/xcat.sh
    echo "MANPATH=\$XCATROOT/share/man:\$MANPATH" >> $ETC_PROFILE_D_DIR/xcat.sh
    echo "export XCATROOT PATH MANPATH" >> $ETC_PROFILE_D_DIR/xcat.sh
    echo "export PERL_BADLANG=0" >> $ETC_PROFILE_D_DIR/xcat.sh
    chmod 755 $ETC_PROFILE_D_DIR/xcat.sh
fi

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

#------------------------------------------------------------
# Check to see if the IP Adress is available in the
# csm_test.cfg file.
#------------------------------------------------------------
ping -c1 $XCATMN > /dev/null 2>&1

if [ $? -ne 0 ]; then 
    echo "[Error   ] The XCATMN address: $XCATMN is not reachable or is not defined properly."
    echo "[Info    ] Please check the $FVT_PATH/csm_test.cfg for details."
    echo "${line1_out}"
    exit 0
fi

#--------------------------------------------------------------------------------
# Variables
#--------------------------------------------------------------------------------
mydate=$(date +'%Y')
current_user=`id -u -n`
HOSTNAME=`hostname | cut -d . -f 1`
ORIG_DIR="/opt/xcat/bin"
ORIG_SBIN_DIR="/opt/xcat/sbin"
TEMP_DIR="/tmp/csm_fvt_xcat_temp"
DEST_DIR="$TEMP_DIR/tmp_xcat"
BACKUP_DIR="/tmp/csm_fvt_xcat_backup"

#--------------------------------------------------------
# Check if the script can be ran on this node
#--------------------------------------------------------
if [[ "$XCATMN" == "$MASTER" ]] || [[ "$XCATMN" == "$HOSTNAME"  ]]; then
    #echo "------------------------------------------------------------------------------------------------------------------------"
    echo "[Info    ] This script does not need to be executed as the CSM MASTER is running on the XCAT management node"
    echo "------------------------------------------------------------------------------------------------------------------------"
    echo "[Info    ] Current management node:  $HOSTNAME"
    echo "[Info    ] Expected management node: $XCATMN"
    echo "------------------------------------------------------------------------------------------------------------------------"
    rm -rf $TEMP_DIR/csm_fvt_xcat_temp*
    exit 0
fi

#----------------------------------------------------------------
# Below makes the TEMP and BACKUP directories
# if they do not exist.
#----------------------------------------------------------------
if [[ ! -e $TEMP_DIR ]] || [[ ! -e $BACKUP_DIR ]] || [[ ! -e $DEST_DIR ]]; then
mkdir -p $TEMP_DIR $BACKUP_DIR $DEST_DIR 2>>/dev/null

if [ $? -ne 0 ]; then
    echo "${line1_out}"
    echo "[Error   ] make directory failed for: $TEMP_DIR & $BACKUP_DIR & $DEST_DIR"
    echo "[Info    ] User: $current_user does not have permission to write to this directory"
    echo "[Info    ] Please specify a valid directory"
    echo "[Info    ] Or log in as the appropriate user"
    echo "${line1_out}"
    exit 1
else
    chown root:root {$TEMP_DIR,$BACKUP_DIR,$DEST_DIR}
    chmod 755 {$TEMP_DIR,$BACKUP_DIR,$DEST_DIR}
fi
elif [[ ! -d $TEMP_DIR ]] || [[ ! -d $BACKUP_DIR ]] || [[ ! -d $DEST_DIR ]]; then
echo "[Info   ] $TEMP_DIR , $BACKUP_DIR , $DEST_DIR already exists but is not a directory" 1>&2
exit 1
fi

#----------------------------------------------------------------
# Clean up directory if anything is left over from previous runs
#----------------------------------------------------------------
TEMP_SUB=`find /tmp/csm_fvt_xcat_temp/* -type d >/dev/null 2>&1`
if [ $? == 0 ]; then
    rm -rf $TEMP_DIR/csm_fvt_xcat_temp*;
fi

#----------------------------------------------------------------
# Below makes the /opt/xcat/bin and /opt/xcat/sbin
# directories if they do not exist.
#----------------------------------------------------------------
if [[ ! -e $ORIG_DIR ]] || [[ ! -e $ORIG_SBIN_DIR ]]; then
mkdir -p $ORIG_DIR $ORIG_SBIN_DIR 2>>/dev/null

if [ $? -ne 0 ]; then
    echo "${line1_out}"
    echo "[Error   ] make directory failed for: $ORIG_DIR & $ORIG_SBIN_DIR"
    echo "[Info    ] User: $current_user does not have permission to write to this directory"
    echo "[Info    ] Please specify a valid directory"
    echo "[Info    ] Or log in as the appropriate user"
    echo "${line1_out}"
    exit 1
else
    chown root:root {$ORIG_DIR,$ORIG_SBIN_DIR}
    chmod 755 {$ORIG_DIR,$ORIG_SBIN_DIR}
fi
elif [[ ! -d $ORIG_DIR ]] || [[ ! -d $ORIG_SBIN_DIR ]]; then
echo "[Info   ] $ORIG_DIR and $ORIG_SBIN_DIR already exists but is not a directory" 1>&2
exit 1
fi

#----------------------------------------------------------------
# Copy over all previous XCAT script files
#----------------------------------------------------------------
echo "[Info    ] Copying all the previous XCAT script files."
cp -pr "${ORIG_DIR}" "$TEMP_DIR/${TEMP_DIR##*/}.$(date +%s)"

#-----------------------------------------------------------------------------
# This creates the some of scripts
# Including: lsdef,nodels,nodestat,ppping,rinv,rpower,rvitals,xcoll,xdcp.orig
# Defines the destination directory /opt/xcat/bin
# Declare an array and pass all the script names to it.
# Loops through each of the files to be created.
# Last step is to make all the scripts an executable.
#-----------------------------------------------------------------------------
declare -a xcatArray
xcatArray=(lsdef nodels nodestat ppping rinv rpower rvitals xcoll)

for i in ${xcatArray[@]}
do
    echo "[Info    ] Creating the ${i}.sh script"
    echo "#! /bin/bash" > $DEST_DIR/$i
    echo "ssh root@$XCATMN \"$i \$@\"" >> $DEST_DIR/$i
    chmod 755 $DEST_DIR/$i
done

#--------------------------------
# This creates the xdcp script
#--------------------------------
echo "[Info    ] Creating the xdcp.sh script"
sudo tee $DEST_DIR/xdcp > /dev/null <<'TXT'
#! /bin/bash

# This script can only handle these variations:
#xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_aggregator.cfg /etc/ibm/csm/csm_aggregator.cfg
#xdcp csm_comp,utility /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
#xdcp csm_comp -R ${INSTALL_DIR} /root
#xdcp csm_comp -p /opt/ibm/csm/share/prologs/* /opt/ibm/csm/prologs
#xdcp ${MASTER} -P /var/log/ibm/csm/csm_master.log ${LOG_PATH}/performance/


if [ $# -lt 3 ]; then
   echo "xdcp: unexpected number of arguments"
   exit 1
fi

NODES=`ssh root@$XCATMN "nodels $1"`
shift

# Check to see if there are any options passed
# Only handle -R, -p, -P based on options currently in use in CSM FVT scripts
if [ "${1:0:1}" == "-" ] ; then
   if [ "$1" == "-R" ] ; then
      OPT="-r"
   elif [ "$1" == "-p" ] ; then
      OPT="-p"
   elif [ "$1" == "-P" ] ; then
      if [ $# -ne 3 ] ; then
         echo "xdcp: -P only supports exactly 3 params"
         exit 1
      fi
      OPT="-P"
   else
      echo "xdcp: unsupported option: $1"
      exit 1
   fi
   shift
else
   # No options, just handle the basic case
   OPT=""
fi

for node in $NODES; do
   if [ "$OPT" == "-P" ] ; then
      # Pull version, does not work with wildcards
      scp -q $node:$1 $2/$(basename "$1")._$node
   else
      # Push version (default)
      # To handle wildcards, split the last param from the list
      scp -q $OPT ${@:1:$(($#-1))} $node:${@:$#}
   fi
done
TXT
sed -i "s/root@10.7.0.221/root@$XCATMN/" $DEST_DIR/xdcp
chmod 755 $DEST_DIR/xdcp

#--------------------------------
# This creates the xdsh
#--------------------------------
echo "[Info    ] Creating the xdsh.sh script"
echo "#! /bin/bash" > $DEST_DIR/xdsh
echo "printf -v args '%q ' \"\${@}\"" >> $DEST_DIR/xdsh
echo "" >> $DEST_DIR/xdsh
echo "ssh root@$XCATMN xdsh \"\$args\"" >> $DEST_DIR/xdsh
echo "" >> $DEST_DIR/xdsh
echo "" >> $DEST_DIR/xdsh
echo "#ssh root@$XCATMN \"xdsh \$@\"" >> $DEST_DIR/xdsh
chmod 755 $DEST_DIR/xdsh

#-----------------------------------------------------------------------------
# Check and compare the backup file with the newly generated file.
#-----------------------------------------------------------------------------
DIFF_DIR=`diff -rq $DEST_DIR $ORIG_DIR | wc -l`

if [ $DIFF_DIR -ne 0 ]; then
    echo "${line1_out}"
    echo "[Info    ] The XCAT files are different from the newly generated script."
    echo "${line1_out}"
    echo "[Info    ] 1. The newly generated XCAT files will replace the existing one."
    echo "[Info    ] 2. The previous XCAT files will be saved to the following dir: $BACKUP_DIR"
    mv $TEMP_DIR/csm_fvt_xcat_temp.* $BACKUP_DIR
    mv $DEST_DIR/* $ORIG_DIR
    cp $ORIG_DIR/* /usr/local/bin
    rm -rf $TEMP_DIR/tmp_xcat*
else
    echo "${line1_out}"
    echo "[Info    ] The xcat files are the same as the newly generated script."
    echo "[Info    ] No additional steps needed."
    rm -rf $TEMP_DIR/csm_fvt_xcat_temp*;
    rm -rf $TEMP_DIR/tmp_xcat*;
fi
echo "${line1_out}"
exit 0
