#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/dgemm-gpu/dgemm-gpu.sh
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

# spectrum mpi install
S_BINDIR=/opt/ibm/spectrum_mpi/healthcheck/dgemm_gpu
#S_BINDIR=/opt/ibm/spectrum_mpi/healthcheck/mpirun_scripts/dgemm_gpu

readonly me=${0##*/}
thishost=`hostname -s`
thisdir=`dirname $0`

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


# check if machine has GPUs
# ===========================
has_gpus
rc=$ret
nsuccess=$((ngpus+1))
if [ "$rc" -ne "0" ]; then echo "$me test FAIL, rc=$rc"; exit $rc; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 


eye_catcher="PERFORMANCE SUCCESS:"
tmpdir=/tmp/$$
tmpout=/tmp/$$.out
hostfile=/tmp/host.list$$

trap 'rm -rf $tmpdir; rm -f $hostfile $tmpout' EXIT


## first, we need to lock the  clock  
## please read /opt/ibm/spectrum_mpi/healthcheck/dgemm_gpu/README.dgemm_gpu

LOCK_CLOCK=1
if [ $# -gt 0 ]; then 
  if ( [ "$1" -ne "0" ] && [ "$1" -ne "1" ]); then echo -e "Invalid argument: $1\n$me test FAIL, rc=$1"; exit 1; fi 
  LOCK_CLOCK=$1
fi

if [ "$LOCK_CLOCK" -eq "1" ]; then
   echo "Issuing 'sudo /usr/bin/nvidia-smi -ac 877,1342'"
   sudo /usr/bin/nvidia-smi -ac 877,1342 
fi

# It is the dgemm_gpu version that comes with spectrum mpi
#------------------------------------------------------      
if [ ! -x $S_BINDIR/run.dgemm_gpu ]; then 
   echo "Can not find the dgemm-gpu installation."          
   echo "$me test FAIL, rc=$rc"
   exit 1
fi

mkdir $tmpdir
output_dir=$tmpdir


# check if we need jsmd
need_jsmd=`grep -c "jsrun " $S_BINDIR/run.dgemm_gpu`
stopd=0
if [ "$need_jsmd" -ne "0" ]; then
   # check if we there is jsm daemon running
   run_flag="-g $ngpus"
   is_running=`/usr/bin/pgrep jsmd`
   if [ -z "$is_running" ]; then 
      # not running, need to create a host.file, just with the hostname
      echo "$thishost" > $hostfile
      source $thisdir/../common/jsm_functions
      start_jsmd $hostfile
      echo "hostfile $hostfile content is:"
      cat $hostfile
      stopd=1
      run_flag="${run_flag} -c"
   fi
   cmd="cd $S_BINDIR; ./run.dgemm_gpu ${run_flag} -d $tmpdir >$tmpout 2>&1"
else
   # need to create a host.file, hostname slots="
   echo "$thishost slots=1" > $hostfile
   echo "hostfile $hostfile content is:"
   cat $hostfile
   cmd="cd $S_BINDIR; ./run.dgemm_gpu -f $hostfile -d $tmpdir >$tmpout 2>&1"
fi


echo -e "\nRunning: $cmd.\n"          
eval $cmd
rc=$?

if [ "$stopd" -eq "1" ]; then stop_jsmd; fi

if [ "$LOCK_CLOCK" -eq "1" ]; then
   echo "Issuing 'sudo /usr/bin/nvidia-smi -rac'"
   sudo /usr/bin/nvidia-smi -rac
fi


echo -e "\n================================================================"
echo -e "\nPrinting dgemm_gpu raw output file(s) in $output_dir"
if [ -d $output_dir ]; then
   for file in `ls $output_dir`; do
     echo -e "\nFile: $output_dir/$file"
     cat $output_dir/$file
     #rm $output_dir/$file
   done
fi


echo -e "\n================================================================"
echo -e "\nResults:\n"
trc=3
if [ -f $tmpout ]; then
   cat $tmpout
   echo -e "\n"
   trc=0
fi

if [ $rc -eq 0 ]; then
   if [ $trc -eq 0 ]; then 
      npass=`grep -c "$eye_catcher" $tmpout`
      if [ "$npass" -ne "$nsuccess" ]; then rc=2; fi
   else
      rc=$trc
   fi
fi

if [ "$rc" -eq "0" ]; then
  echo "$me test PASS, rc=0"
else
  echo "$me test FAIL, rc=$rc"
fi

exit $rc

