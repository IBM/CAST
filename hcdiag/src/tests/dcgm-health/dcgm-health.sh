#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/dcgm-health/dcgm-health.sh
# 
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#=============================================================================

# Checks the overall health for the GPU subsystems (PCIe, SM, MCU, PMU, 
# Inforom, Power and thermal system)
# It consists of 3 steps:
# 1. Set the watches to be monitored.
# 2. Sleep 1 s
# 2. Check to see if any errors or warnings have occurred in the currently 
#      monitored watches.
# This script looks for lines with 'Error' below:
#    | Overall Health:   Healthy           |
#    | Overall Health:   Warning           |
#    | Overall Health:   Error             |
# Group monitoring is not supported

## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi


me=$(basename $0) 
thishost=`hostname -s`

# Possible values for watch. Default is a - all
# a - all watches
# p - PCIe watches (*)
# m - memory watches (*)
# i - infoROM watches
# t - thermal and power watches (*)
# n - NVLink watches (*)
# (*) watch requires 60 sec before first query
WATCHES=a
if [[ $# -gt 0 ]]; then WATCHES="$@"; fi

thisdir=`dirname $0`
source $thisdir/../common/functions
source $thisdir/../common/gpu_functions

supported_machine
echo "Running $me on $thishost, machine type $model."          

if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ -z $is_boston ]; then 
   echo -e "Could not determine if the machine has GPUs by model. Continuing.."
   is_boston=False
fi 
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 

# check if machine has GPUs
# ===========================
has_gpus
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 


trap 'rm -f /tmp/$$' EXIT

# check if argument is valid
# ==========================
export DCGM_DAEMON=/usr/bin/nv-hostengine
DCGM_CMD=/usr/bin/dcgmi

# start the daemons
# =================
start_nvidia_persistenced
pe_running=$is_nvidia_persistenced_running
start_dcgm
dcgm_running=$is_dcgm_running


$DCGM_CMD --version

# start the health check
# ======================
echo -e "\nIssuing command $DCGM_CMD discovery -l" 
$DCGM_CMD discovery -l 
         
# we need to activate the watches
echo -e "\nIssuing command $DCGM_CMD health --set ${WATCHES}"
$DCGM_CMD health --set "${WATCHES}"
# show what was activated
# we need to wait 60s, for some of the watches.. we wait for all
sleep 1 
echo -e "\nIssuing command $DCGM_CMD health --fetch"
$DCGM_CMD health --fetch 
echo -e "\nIssuing command $DCGM_CMD health --check"
$DCGM_CMD health --check  | tee /tmp/$$
rc=$?


# stop the daemons 
# =================
if [ $pe_running -eq 0 ]; then stop_nvidia_persistenced; fi
if [ $dcgm_running -eq 0 ]; then stop_dcgm; fi

# analize the diag results
# =========================
drc=0
if [ "$rc" -ne "0" ]; then
   echo "Error running $DGM_CMD health command,  rc= $rc."
else
   # Looking lines like "Overal Health"
   # | Overall Health:   Healthy           |
   # | Overall Health:   Warning           |
   # | Overall Health:   Error             |
   tmp=(`awk '$0~/Health:/{ print $(NF-1) }' /tmp/$$`)
   echo ${tmp[*]} | grep -q Error
   if [ $? -eq "0" ]; then
      drc=99 
   fi
fi


#  Exit code
#  =========
if [ "$drc" -eq "0" ]; then
   if [ "$rc" -eq "0" ]; then
      echo -e "\n$me test PASS, rc=0"  
   else
      echo -e "\n$me test FAIL, rc=$rc"  
   fi
else
   rc=$drc
   echo -e "\n$me test FAIL, rc=$rc"  
fi
  
exit $rc

