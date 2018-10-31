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
SMPI_ROOT=/opt/ibm/spectrum_mpi/healthcheck
verbose=false

usage()
{
cat << EOF
        Usage: `basename $0` [options]
        Runs DGEMM
        Optional arguments:
              [-s]  Runs dgemm per socket
              [-v]  Set verbose mode
              [-m]  Use mpirun instead of jsrun
              [-h]  This help screen
EOF
}

tmpdir=/tmp/$$
args="-d $tmpdir"

thisdir=`dirname $0`
readonly me=${0##*/}
thishost=`hostname -s`
source $thisdir/../common/functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on $thishost, machine type $model."          
read_basics 
slots=$core_present

MPI_DIR=""
while [[ $# -gt 0 ]]; do
  opt="$1"
  case $opt in
      -s)
        args+=" $opt"
        slots=$((slots/2))
        ;;
      -v)
        verbose=true
        ;;
      -m)
        # use mpirun instead of jsrun
        MPI_DIR="mpirun_scripts"
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



hostfile=/tmp/host.list$$
#trap 'rm -rf $tmpdir; rm -f $hostfile' EXIT


# It is the dgemm version that comes with spectrum mpi
#------------------------------------------------------      
get_processor
if [ "$ret" -ne "0" ]; then echo -e "Processor not supported: $processor.\n$me test FAIL, rc=1"; exit 1; fi
DGEMM_DIR="${SMPI_ROOT}/POWER${processor:1:1}/${MPI_DIR}/dgemm"

if [ ! -x ${DGEMM_DIR}/run.dgemm ]; then 
   echo "Can not find the dgemm installation ${SMPI_ROOT}/POWER${processor:1:1}/dgemm."          
   echo "$me test FAIL, rc=$rc"
   exit 1
fi

mkdir $tmpdir
output_dir=$tmpdir

# check if we are in jsm land
need_jsmd=`grep -c "jsrun" $DGEMM_DIR/run.dgemm`
stopd=0
if [ "$need_jsmd" -ne "0" ]; then
   # check if we there is jsm daemon running
   is_running=`/usr/bin/pgrep jsmd`
   if [ -z "$is_running" ]; then 
      # not running, need to create a host.file, with just the hostname
      echo "$thishost" > $hostfile
      source $thisdir/../common/jsm_functions
      start_jsmd $hostfile
      echo "hostfile $hostfile content is:"
      cat $hostfile
      stopd=1
      args+=" -c"
   fi
else
   # need to create a host.file, with slots="
   echo "$thishost slots=$slots" > $hostfile
   echo "hostfile $hostfile content is:"
   cat $hostfile
   args+=" -f $hostfile"
fi

cmd="cd ${DGEMM_DIR}; ./run.dgemm ${args}"
echo -e "\nRunning: $cmd"
#echo -e "\n================================================================"
#echo -e "\nResults:\n"
eval $cmd
rc=$?

if [ "$stopd" -eq "1" ]; then stop_jsmd; fi

if $verbose; then
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
fi

if [ $rc -eq 0 ]; then echo "$me test PASS, rc=0"; exit 0; fi

echo "$me test FAIL, rc=$rc"
exit $rc

