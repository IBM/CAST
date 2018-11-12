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
SMPI_ROOT=/opt/ibm/spectrum_mpi/healthcheck
CLEANUP_OS_CACHES=false     # requires sudo
verbose=false

usage()
{
cat << EOF
        Usage: `basename $0` [options]
        Runs DGEMM
        Optional arguments:
              [-c]  Clean up OS caches 
              [-s]  Runs daxpy per socket
              [-v]  Set verbose mode
              [-h]  This help screen
EOF
}

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


tmpdir=/tmp/$$
trap 'rm -rf $tmpdir' EXIT
args="-d $tmpdir"

while [[ $# -gt 0 ]]; do
  opt="$1"
  case $opt in
      -s)
        args+=" $opt"
        ;;
      -v)
        verbose=true
        ;;
      -c)
        CLEANUP_OS_CACHES=true
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

readonly me=${0##*/}
thishost=`hostname -s`
thisdir=`dirname $0`

source $thisdir/../common/functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running daxpy on `hostname -s`, machine type $model."          



## main

echo -e "\n================================================================"
read_basics_daxpy
read_basics 


#tmpout=/tmp/$$.out

# cleanup OS caches
if $CLEANUP_OS_CACHES; then
   echo -e "\nCleaning up OS caches"
   sudo -s eval 'free && sync && echo 3 > /proc/sys/vm/drop_caches && echo  && free'
fi

# daxpy version comes with spectrum mpi
#------------------------------------------------------
DAXPY_DIR="${SMPI_ROOT}/POWER${processor:1:1}/daxpy"
if [ ! -x ${DAXPY_DIR}/run.daxpy ]; then echo -e "Can not find the daxpy installation ${DAXPY_DIR}/run.daxpy.\n$me test FAIL, rc=$rc"; exit 1; fi

mkdir $tmpdir

echo -e "\nRunning: cd ${DAXPY_DIR}; ./run.daxpy $args"
cd ${DAXPY_DIR}; ./run.daxpy ${args}
rc=$?
#cd ${DAXPY_DIR}; ./run.daxpy ${args} > ${tmpout}



if $verbose; then
  echo -e "\nPrinting daxpy raw output file(s)"
  echo -e "\n================================================================"
  if [ -d $tmpdir ]; then
    for file in `ls ${tmpdir}`; do
      echo -e "\nFile: ${tmpdir}/${file}"
      cat $tmpdir/$file
    done
  fi
  echo -e "\n================================================================"
fi 

if [ $rc -eq 0 ]; then echo "$me test PASS, rc=0"; exit 0; fi

echo "$me test FAIL, rc=$rc"
exit $rc


exit $rc

