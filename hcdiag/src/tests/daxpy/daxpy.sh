#!/bin/bash                                                                    

#================================================================================
#   
#    hcdiag/src/tests/daxpy/daxpy.sh
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

# DAXPY test measures aggregate memory bandwidth in MB/s.
# spectrum mpi install
S_BINDIR=/opt/ibm/spectrum_mpi/healthcheck/daxpy
# clean up os caches as root 1: yes, 0: no
CLEANUP_OS_CACHES=0

readonly me=${0##*/}
thishost=`hostname -s`
thisdir=`dirname $0`

source $thisdir/../common/functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running daxpy on `hostname -s`, machine type $model."          


eye_catcher="PERFORMANCE SUCCESS:"
trap 'rm -rf /tmp/$$*' EXIT

function read_basics_daxpy()
{

   echo -e "\nReading configuration that might affect the result of the test."

   ## check if the SMT is set to 8
   # 
   get_processor
   if [ "$ret" -ne "0" ]; then echo -e "Processor not supported: $processor.\n$me test FAIL, rc=1"; exit 1; fi

   # assumes it is a smt4
   smt=4
   if [ $processor == "p8" ]; then smt=8; fi

   ssmt=`/usr/sbin/ppc64_cpu --smt | cut -d \= -f 2`
   if [ "$ssmt" != "$smt" ]; then
      echo "Unsupported machine setting 'SMT= $ssmt'. We only support SMT=$smt."          
      echo "$me test FAIL, rc=$rc"
      exit 1
   fi
   echo -e "\nMachine SMT setting is $ssmt."

   # read memory info
   # 
   echo -e "\nMemory information"
   /bin/vmstat -s

}         

## main

echo -e "\n================================================================"
read_basics_daxpy
read_basics 


tmpdir=/tmp/$$
tmpout=/tmp/$$.out
tmperr=/tmp/$$.err

# cleanup OS caches
if [ "$CLEANUP_OS_CACHES" == "1" ]; then
   echo -e "\nCleaning up OS caches"
   sudo -s eval 'free && sync && echo 3 > /proc/sys/vm/drop_caches && echo  && free'
fi

# daxpy version comes with spectrum mpi
#------------------------------------------------------
if [ ! -x $S_BINDIR/run.daxpy ]; then echo -e "Can not find the daxpy installation.\n$me test FAIL, rc=$rc"; exit 1; fi

mkdir $tmpdir
level=1
if [ $# -gt 0 ]; then level=$1; fi 

#### level 1
rc=0
if [ $level -eq 1 ]; then
   echo -e "\nRunning: cd $S_BINDIR; ./run.daxpy -d $tmpdir >$tmpout 2>&1"
   cd $S_BINDIR; ./run.daxpy -d $tmpdir >$tmpout 2>&1

   echo -e "\n================================================================"
   echo -e "\nPrinting daxpy raw output file(s)"
   if [ -d $tmpdir ]; then
     for file in `ls $tmpdir`; do
        echo -e "\nFile: $tmpdir/$file"
        cat $tmpdir/$file
     done
   fi
   
   echo -e "\n================================================================"
   echo -e "\nResults:\n"
   
   if [ -f $tmpout ]; then
      cat $tmpout
      ok=`grep "$eye_catcher" $tmpout`
      if [ ! -n "$ok" ]; then rc=2; fi
   else
      rc=3
   fi
#### level 2
#### this should go to the run.daxpy (smpi)
elif [ $level -eq 2 ]; then 
   ncpu=`grep processor /proc/cpuinfo|wc -l`
   half=$((ncpu / 2))
   EXPECTED_PERF=134000
   re='^[0-9]+([.][0-9]+)?$'
   if [ $# -gt 1 ]; then EXPECTED_PERF=$2; fi 
   cd $S_BINDIR

   export GOMP_CPU_AFFINITY="`echo $(seq 0 $((half -1)))`"
   export OMP_NUM_THREADS=$half
   for i in 0 8; do
     if [ $i -eq 8 ]; then export GOMP_CPU_AFFINITY="`echo $(seq $half $((ncpu - 1)))`"; fi
     cmd="numactl --membind $i ./daxpy -N 100 -M 128 -b 8 > $tmpout 2> $tmperr"
     echo "Issuing: $cmd"
     #echo "GOMP_CPU_ACTIVIRY=$GOMP_CPU_AFFINITY"
     eval ${cmd}
     if [ -s $tmperr ]; then cat $tmperr; fi 
     if [ -s $tmpout ]; then cat $tmpout; fi 

     perf=`grep DAXPY $tmpout | awk '{printf $3}'`
     if  [[ $perf =~ $re ]]; then 
       perf=`echo ${perf%.*}` 
       if [[ $perf -lt "$EXPECTED_PERF" ]] ; then
           echo "$thishost: PERFORMANCE FAILURE: $perf MB/s; EXPECTED PERFORMANCE: $EXPECTED_PERF MB/s "
           rc=99
       else
           echo "$thishost: PERFORMANCE SUCCESS: $perf MB/s; EXPECTED PERFORMANCE: $EXPECTED_PERF MB/s "
       fi
     else
        echo "ERROR parsing the daxpy output file."
        rc=98
     fi
     echo -e "\n\n"
  done
else 
   echo -e "Invalid level $level. Level must be 1 or 2." 
   rc=1
fi


if [ $rc -eq 0 ]; then
  echo "$me test PASS, rc=$rc"
else
  echo "$me test FAIL, rc=$rc"
fi

exit $rc

