#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/dgemm-per-socket/dgemm-per-socket.sh
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

# dgemm-per-socket tests cpu performance per socket
# only for  witherspoon

## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi

# spectrum mpi install
export MPI_ROOT=/opt/ibm/spectrum_mpi
S_BINDIR=$MPI_ROOT/healthcheck/dgemm
ENV="-E LD_LIBRARY_PATH=$MPI_ROOT/lib:/opt/ibm/spectrum_mpi/lib:/opt/ibmmath/essl/6.1/lib64:$LD_LIBRARY_PATH"

# Expected performance in Gflops.
EXPECTED_PERF=450

# Siz of matrix and number of iterations
MATRIX_MEMORY=20
NUM_ITERATIONS=15
print_raw_file=1

if [ $# -gt 0 ]; then EXPECTED_PERF=$1; fi 


readonly me=${0##*/}
thishost=`hostname -s`
thisdir=`dirname $0`

source $thisdir/../common/functions
source $thisdir/../common/jsm_functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ ! is_witherspoon ]; then echo "Machine not supported.\n$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running $me on $thishost, machine type $model."          


tmpout=/tmp/$$.out
tmperr=/tmp/$$.err
hostfile=/tmp/host.list$$

trap 'rm -f $hostfile $tmpout $tmperr' EXIT

read_basics 


# It is the dgemm version that comes with spectrum mpi
#------------------------------------------------------      
if [ ! -x $S_BINDIR/dgemm ]; then 
   echo "Can not find the dgemm installation."          
   echo "$me test FAIL, rc=$rc"
   exit 1
fi


# check if we are in jsm land
stopd=0
ret=0
#set -x
# check if we there is jsm daemon running
is_running=`/usr/bin/pgrep jsmd`
if [ -z "$is_running" ]; then 
    # not running, need to create a host.file, with just the hostname
    echo "$thishost" > $hostfile
    start_jsmd $hostfile
    echo "hostfile $hostfile content is:"
    cat $hostfile
    stopd=1
fi

for socket in 0 1; do
   cmd="${MPI_ROOT}/jsm_pmix/bin/jsrun -R ${env} --rs_per_host 22 --env PINNER_SOCKET=$socket ${thisdir}/pinner.py ${S_BINDIR}/dgemm $MATRIX_MEMORY $NUM_ITERATIONS >>${tmpout} 2>>${tmperr}"
   echo -e "\nRunning on socket $socket: $cmd"
   eval $cmd 
   rc=$?
   if [ $rc -ne 0 ]; then ret=$rc, echo "(ERROR) invoking jsrun, rc= $rc"; fi
done

if [ "$stopd" -eq "1" ]; then stop_jsmd; fi

if [ -s ${tmperr} ]; then
    cat ${tmperr}
fi     

cout=0
if [ -s $tmpout ]; then
   if [ $print_raw_file -eq 1 ]; then cat $tmpout; fi
   echo -e "\n"
   # let's parse the output
   echo -e "\n================================================================"
   echo -e "\nResults:\n"
   while read line; do
     aline=( $line )
     HOSTNAME=${aline[0]}
     PERF=${aline[10]}
     if [[ ! -z "$PERF" ]] ; then 
        PERF=$( printf "%.0f" $PERF )
     fi

     if [[ ! -z "$PERF" ]] ; then 
        OUTPUT_LINE="$HOSTNAME PERFORMANCE SUCCESS: $PERF GF/s; EXPECTED PERFORMANCE: $EXPECTED_PERF GF/s "
        if [[ $PERF -lt "$EXPECTED_PERF" ]] ; then
           OUTPUT_LINE="$HOSTNAME PERFORMANCE FAILURE: $PERF GF/s; EXPECTED PERFORMANCE: $EXPECTED_PERF GF/s "
           ret=99
        fi
        echo " "
        echo $OUTPUT_LINE
        count=$((count+1))
     fi
   done < ${tmpout}
fi



if [ $ret -eq 0 ]; then 
   if [ $count -eq 2 ]; then 
      echo "$me test PASS, rc=$ret" 
      exit 0
   else 
      echo "(ERROR): Count not find two results"
      rc=1 
   fi
fi
echo "$me test FAIL, rc=$ret"
exit $ret

