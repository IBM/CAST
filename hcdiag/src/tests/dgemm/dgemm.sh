#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/dgemm/dgemm.sh
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

# dgemm tests cpu performance
# garrison, firestone, witherspoon, boston  machines are supported

## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi

# spectrum mpi install
S_BINDIR=/opt/ibm/spectrum_mpi/healthcheck/dgemm
#DISABLE_GPU_PM=1


readonly me=${0##*/}
thishost=`hostname -s`
thisdir=`dirname $0`

source $thisdir/../common/functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running $me on $thishost, machine type $model."          

trap 'rm -rf /tmp/$$*' EXIT
trap 'rm -f /tmp/host.list$$' EXIT


tmpdir=/tmp/$$
tmpout=/tmp/$$.out

read_basics 
slots=$core_present


# It is the dgemm version that comes with spectrum mpi
#------------------------------------------------------      
if [ ! -x $S_BINDIR/run.dgemm ]; then 
   echo "Can not find the dgemm installation."          
   echo "$me test FAIL, rc=$rc"
   exit 1
fi

mkdir $tmpdir
output_dir=$tmpdir
eye_catcher="PERFORMANCE SUCCESS:"
hostfile=/tmp/host.list$$


# check if we are in jsm land
need_jsmd=`grep -c "jsrun" $S_BINDIR/run.dgemm`
stopd=0
if [ "$need_jsmd" -ne "0" ]; then
   # check if we there is jsm daemon running
   run_flag=""
   is_running=`/usr/bin/pgrep jsmd`
   if [ -z "$is_running" ]; then 
      # not running, need to create a host.file, with just the hostname
      echo "$thishost" > $hostfile
      source $thisdir/../common/jsm_functions
      start_jsmd $hostfile
      echo "hostfile $hostfile content is:"
      cat $hostfile
      stopd=1
      run_flag="-c"
   fi
   cmd="cd $S_BINDIR; ./run.dgemm $run_flag -d $tmpdir >$tmpout 2>&1"
else
   # need to create a host.file, with slots="
   echo "$thishost slots=$slots" > $hostfile
   echo "hostfile $hostfile content is:"
   cat $hostfile
   cmd="cd $S_BINDIR; ./run.dgemm -f $hostfile -d $tmpdir >$tmpout 2>&1"
fi


# disable GPU persistence mode, it creates noise
# it set param with the indexes
# ----------------------------------------------
#if [ $DISABLE_GPU_PM -eq 1 ]; then 
#  source $thisdir/../common/gpu_functions
#  has_gpus
#  setpm=$ngpus
#  if [ "$setpm" -ne "0" ]; then set_gpu_pm 0; fi
#fi

echo -e "\nRunning: $cmd"
eval $cmd
rc=$?

if [ "$stopd" -eq "1" ]; then stop_jsmd; fi

# set gpu persistence mode back to what it was
# ---------------------------------------------
#if [ $DISABLE_GPU_PM -eq 1 ]; then 
#   if [ "$setpm" -ne "0" ]; then restore_gpu_pm 1 "$param"; fi
#fi

echo -e "\n================================================================"
echo -e "\nPrinting dgemm raw output file(s) in $output_dir"
if [ -d $output_dir ]; then
   for file in `ls $output_dir`; do
     echo -e "\nFile: $output_dir/$file"
     cat $output_dir/$file
     rm $output_dir/$file
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
      ok=`grep "$eye_catcher" $tmpout`
      if [ -n "$ok" ]; then
         echo "$me test PASS, rc=0"
         exit 0
      else 
         rc=2
      fi
   else
     rc=$trc
   fi
fi

echo "$me test FAIL, rc=$rc"


exit $rc

