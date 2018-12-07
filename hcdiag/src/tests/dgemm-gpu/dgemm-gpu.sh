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
SMPI_ROOT=/opt/ibm/spectrum_mpi/healthcheck
## please read /opt/ibm/spectrum_mpi/healthcheck/dgemm_gpu/README.dgemm_gpu
## do we need to lock the clock? 
## application clock, <memory,graphics> clocks as a pair (e.g. 2000,800) 
## that defines GPU's speed in MHz while running applications on a GPU.
APPL_CLOCK="877,1342"
LOCK_CLOCK=0
verbose=false

usage()
{
cat << EOF
        Usage: `basename $0` [options]
        Runs GPU DGEMM
        Optional arguments:
              [-l]  Lock GPU application clock
              [-c]  Set GPU application clock: <memory,graphics>
              [-v]  Set verbose mode
              [-m]  Use mpirun instead of jsrun
              [-h]  This help screen
EOF
}

readonly me=${0##*/}
thishost=`hostname -s`
thisdir=`dirname $0`

MPI_DIR=""
while [[ $# -gt 0 ]]; do
  opt="$1"
  case $opt in
      -l)
        LOCK_CLOCK=1
        ;;
      -c)
        APPL_CLOCK=$2
        shift 
        ;;
      -m)
        # use mpirun instead of jsrun
        MPI_DIR="mpirun_scripts"
        ;;
      -v)
        verbose=true
        ;;
      -h)
        usage
        exit 0
        ;;
      *)
        echo "Invalid argument: $opt"
        exit 1
        ;;
  esac
  shift
done

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
if [ "$rc" -ne "0" ]; then echo "$me test FAIL, rc=$rc"; exit $rc; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 

if [ "$LOCK_CLOCK" -eq "1" ]; then
   echo "Issuing 'sudo /usr/bin/nvidia-smi -ac ${APPL_CLOCK}'"
   sudo /usr/bin/nvidia-smi -ac ${APPL_CLOCK} 
fi

tmpdir=/tmp/$$
hostfile=/tmp/host.list$$
trap 'rm -rf $tmpdir; rm -f $hostfile' EXIT

# It is the dgemm_gpu version that comes with spectrum mpi
#------------------------------------------------------      
get_processor
if [ "$ret" -ne "0" ]; then echo -e "Processor not supported: $processor.\n$me test FAIL, rc=1"; exit 1; fi
DGEMM_DIR="${SMPI_ROOT}/POWER${processor:1:1}/${MPI_DIR}/dgemm_gpu"

if [ ! -x ${DGEMM_DIR}/run.dgemm_gpu ]; then 
   echo "Can not find the dgemm-gpu installation: ${DGEMM_DIR}."          
   echo "$me test FAIL, rc=$rc"
   exit 1
fi

mkdir $tmpdir
output_dir=$tmpdir

# check if we need jsmd
need_jsmd=`grep -c "jsrun " ${DGEMM_DIR}/run.dgemm_gpu`
flags="-d $tmpdir" 
stopd=0
if [ "$need_jsmd" -ne "0" ]; then
   # check if we there is jsm daemon running
   flags+=" -g $ngpus"
   is_running=`/usr/bin/pgrep jsmd`
   if [ -z "$is_running" ]; then 
      # not running, need to create a host.file, just with the hostname
      echo "$thishost" > $hostfile
      source $thisdir/../common/jsm_functions
      start_jsmd $hostfile
      echo "hostfile $hostfile content is:"
      cat $hostfile
      stopd=1
      flags+=" -c"
   fi
else
   # need to create a host.file, hostname slots="
   echo "$thishost slots=1" > $hostfile
   echo "hostfile $hostfile content is:"
   cat $hostfile
   flags+=" -f $hostfile"
fi

echo -e "\nRunning: cd ${DGEMM_DIR}; ./run.dgemm_gpu ${flags}"
cd ${DGEMM_DIR}; ./run.dgemm_gpu ${flags}
rc=$?

if [ "$stopd" -eq "1" ]; then stop_jsmd; fi

if [ "$LOCK_CLOCK" -eq "1" ]; then
   echo "Issuing 'sudo /usr/bin/nvidia-smi -rac'"
   sudo /usr/bin/nvidia-smi -rac
fi

if $verbose; then
   echo -e "\n================================================================"
   echo -e "\nPrinting dgemm_gpu raw output file(s) in $output_dir"
   if [ -d $output_dir ]; then
      for file in `ls $output_dir`; do
        echo -e "\nFile: $output_dir/$file"
        cat $output_dir/$file
      done
   fi
fi


if [ "$rc" -eq "0" ]; then echo "$me test PASS, rc=0"; exit 0; fi

echo "$me test FAIL, rc=$rc"
exit $rc

