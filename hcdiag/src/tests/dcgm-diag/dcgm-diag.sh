#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/dcgm-diag/dcgm-diag.sh
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

## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi

export DCGM_DAEMON=/usr/bin/nv-hostengine
export DCGM_CMD=/usr/bin/dcgmi

me=$(basename $0) 
thishost=`hostname -s`

RUN_LEVEL=3
if [[ $# -gt 0 ]]; then RUN_LEVEL=$1; fi

# nvidia diagnostic has to run as root
# ====================================
#sudo -E bash <<"EOF"

thisdir=`dirname $0`
source $thisdir/../common/functions
source $thisdir/../common/gpu_functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on $thishost, machine type $model."          
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 

# check if machine has GPUs
# ===========================
has_gpus
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 


trap 'rm -f /tmp/$$' EXIT

# check if argument is valid
# ==========================
run_levels=(1 2 3)
valid=$(echo ${run_levels[@]} | grep -o $RUN_LEVEL | wc -w)
if [ $valid -eq 0 ]; then echo "Invalid RUN_LEVEL $RUN_LEVEL, valid values are: ${run_levels[@]} "  ; exit -1; fi 

FAIL="Fail"


if [ ! -x $DCGM_DAEMON ]; then echo "$DCGM_DAEMON executable not found or invalid permission"  ; exit -1; fi
if [ ! -x $DCGM_CMD ]; then echo "$DCGM_CMD executable not found or invalid permission"; exit -1; fi


# start the daemons
# =================
start_nvidia_persistenced
pe_running=$is_nvidia_persistenced_running
start_dcgm
dcgm_running=$is_dcgm_running


# Enable Persistent Mode, we always save/restore persistent mode
# param has the indexes that were modified
# ==============================================================
#set_gpu_pm 1
#indexes="$param"

$DCGM_CMD --version

read_gpu_basics 0

# start the diagnostic 
# ====================
echo -e "\nIssuing command $DCGM_CMD discovery -l" 
$DCGM_CMD discovery -l 
         
echo -e "\nIssuing command $DCGM_CMD diag -r $RUN_LEVEL"
$DCGM_CMD diag -r $RUN_LEVEL | tee /tmp/$$
rc=$?


# stop the daemons 
# =================
if [ $pe_running -eq 0 ]; then stop_nvidia_persistenced; fi
if [ $dcgm_running -eq 0 ]; then stop_dcgm; fi

# analize the diag results
# =========================
drc=0
if [ "$rc" -ne "0" ]; then
   echo "Error running $DGM_CMD diag command,  rc= $rc."
else
   if egrep -i -q 'Killed|Error' /tmp/$$; then
      drc=255
   fi
   if grep -q $FAIL /tmp/$$; then
      if [ "$drc" -eq "0" ]; then 
         drc=99 
      fi                           
   fi
fi


#  restoring the Persistence Mode
#  ==============================
#restore_gpu_pm 0 "$indexes"

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

