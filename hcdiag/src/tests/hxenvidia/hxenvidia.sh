#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/hxenvidia/hxenvidia.sh
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

readonly me=${0##*/}
thisdir=`dirname $0`
source $thisdir/../common/htx_functions
source $thisdir/../common/functions
source $thisdir/../common/gpu_functions

supported_machine
echo "Running $me on `hostname -s`, machine type $model."          

if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ -z $is_boston ]; then 
   echo -e "Could not determine if the machine has GPUs by model. Continuing.."
   is_boston=False
fi 
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 

[ $# -lt 2 ] && echo "Usage: $me <duration> <pass> [debug]" && exit 1
debug=0
duration=$1
pass=$2
if [ $# -gt 2 ]; then debug=$3; fi
if [ $pass -ne 1 -a $pass -ne 2 ]; then echo "Invalid pass $pass. Valid values: 1, 2."; exit 1; fi

# check if machine has GPUs
# ===========================
has_gpus
rc=$ret
if [ "$rc" -ne "0" ]; then echo "$me test FAIL, rc=$rc"; exit $rc; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 

read_gpu_basics 0

rule="nvidia:gpu.coral.pass$pass"
thismdt="$htxd_root/mdt/mdt.hxenvidia.coral_pass$pass"

# validates the enviroment to run htx and starts the htxd daemon
htxdiag_setup $debug
rc=$ret

if [ $rc -eq 0 ]; then 

   # creates the mdt file to run hxenvidia
   htxdiag_get_mdt $rule $thismdt
   rc=$ret
   if [ $rc -eq 0 ]; then 
      # run the test and validates
      htxdiag_run $thismdt $duration $debug
      rc=$ret
   fi

   # remove the files and stop the daemon
   htxdiag_cleanup
fi

if [ $rc -eq 0 ]; then
  echo "$me test PASS, rc=0"
else
  echo "$me test FAIL, rc=$rc"
fi

exit $rc
