#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/nvvs/nvvs.sh
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


#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64
#export SMI_TOOL=/usr/bin/nvidia-smi
export NVVS_TOOL=/usr/share/nvidia-validation-suite/nvvs 

RUN_LEVEL=3
if [ $# -gt 0 ]; then RUN_LEVEL=$1; fi
export RUN_LEVEL
export me=$(basename $0) 
export thisdir=`dirname $0`
export thishost=`hostname -s`



# nvidia diagnostic has to run as root
# ====================================

#sudo -E bash <<"EOF"

source $thisdir/../common/functions
source $thisdir/../common/gpu_functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on `hostname -s`, machine type $model."          
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 

# check if machine has GPUs
# ===========================
has_gpus
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 


status=( PASS SKIP FAIL FAIL_UNKNOWN )

case $RUN_LEVEL in
    1) run_level=short ;;
    2) run_level=medium ;;
    3) run_level=long ;;
    *) echo "Invalid RUN_LEVEL $RUN_LEVEL, valid values are: 1, 2, 3"  
       exit -1 ;;
esac

NVVS_ARGS="--specifiedtest $run_level --verbose --statsonfail"
#NVVS_ARGS="--specifiedtest $run_level --verbose --debugLevel 5"

trap 'rm -f /tmp/$$' EXIT
trap 'rm -f /tmp/$$r' EXIT

#if [ ! -x $SMI_TOOL ]; then echo "$SMI_TOOL not found or invalid permission."  ; exit 2; fi
if [ ! -x $NVVS_TOOL ]; then echo "$NVVS_TOOL not found or invalid permission."; exit 2; fi

# Enable Persistent Mode, we always save/restore persistent mode
# param has the indexes that were modified
# ==============================================================
#set_gpu_pm 1
#rc=$ret
#if [ "$rc" -ne "0" ]; then echo "$me test FAIL, rc=$rc"; exit $rc; fi  
#indexes="$param"

# start the daemon
# =================
start_nvidia_persistenced
pe_running=$is_nvidia_persistenced_running


$NVVS_TOOL --version

read_gpu_basics 

# start the diagnostic 
# ====================
         
echo -e  "\nRunning diagnostic: $NVVS_TOOL ${NVVS_ARGS}"
$NVVS_TOOL ${NVVS_ARGS} | tee /tmp/$$
rc=$?

echo -e "\n$NVVS_TOOL ${NVVS_ARGS}, rc= $rc"


#  restoring the Persistence Mode
#  ==============================
#restore_gpu_pm 0 "$indexes"
#rc=$ret

# stop the daemon 
# ===============
if [ $pe_running -eq 0 ]; then stop_nvidia_persistenced; fi

# analize the diag results
# =========================
drc=0
N=${#status[@]}
N=$((N-1))
NV=$((N-1))

declare -A count
for i in $(seq 0 $N); do
  count[${status[$i]}]=0
done
IFS=$'\n'

if [ "$rc" -eq "0" ]; then
   if [ -f /tmp/$$ ]; then 
      grep "\.\.\.\.\.\.\." /tmp/$$ > /tmp/$$r
      for next in `cat /tmp/$$r` ; do
         index=${status[$N]}
         found=False
         for i in $(seq 0 $NV); do
           echo "$next" | grep -s ${status[$i]} 
           if [ $? -eq 0 ];  then
              index=${status[$i]}
              found=True
              #echo "Index is $index"
           fi
           if [ $found = "True" ]; then break; fi
         done
         c=${count[$index]}
         count[$index]=$((c+1))
      done

      echo -e "\nnvvs GPU diagnostics result:"
      for i in $(seq 0 $N); do
         echo -e "tests ${status[$i]} : ${count[${status[$i]}]}"
      done
      if ( [ ${count[FAIL]} -gt 0 ] || [ ${count[FAIL_UNKNOWN]} -gt 0 ] || [ ${count[PASS]} -eq 0 ] ); then 
         drc=99; 
      fi
   else 
      echo "nvvs did not produce the output file"
      drc=99
   fi
fi


#  Exit code
#  =========
if [  "$drc" -eq "0" ]; then
   if [ "$rc" -eq "0" ]; then
      echo -e "\n$me test PASS, rc=0"  
   else
      echo -e "\n$me test FAIL, rc=$rc"  
   fi
else
   rc=$drc
   echo -e "\n$me test test FAIL, rc=$rc"  
fi
  
exit $rc


#EOF
