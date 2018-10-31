#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/jlink/jlink.sh
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
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 1>$THIS_LOG 2>&1
fi



# spectrum mpi install
SMPI_ROOT=/opt/ibm/spectrum_mpi/healthcheck
verbose=false

usage()
{
cat << EOF
        Usage: `basename $0` [options]
        Runs jlink
        Optional arguments:
              [-j]  Use jsrun instead of mpirun (not working)
              [-v]  Set verbose mode
              [-h]  This help screen
EOF
}

# jlink tests the bandwidth for all possible node pairings.
# Witherspoon, Garrison, Firestone machines are supported

export PATH=$PATH:/usr/sbin
readonly me=${0##*/}
thisdir=`dirname $0`


## fixme: make it work with jsm
MPI_DIR="mpirun_scripts"
while [[ $# -gt 0 ]]; do
  opt="$1"
  case $opt in
      -v)
        verbose=true
        ;;
      -j)
        # use jsrun instead of jsrun
        MPI_DIR=""
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


[ -z $MP_HOSTFILE ] && echo "MP_HOSTFILE not set. Exiting." && exit 1

source $thisdir/../common/functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running $me on `hostname -s`, machine type $model."          
trap 'rm -rf /tmp/$$' EXIT
tmpdir=/tmp/$$

get_processor
if [ "$ret" -ne "0" ]; then echo -e "Processor not supported: $processor.\n$me test FAIL, rc=1"; exit 1; fi
JLINK_DIR="${SMPI_ROOT}/POWER${processor:1:1}/${MPI_DIR}/jlink"
export PATH=$PATH:${JLINK_DIR}

# It is the jlink version that comes with spectrum mpi
#------------------------------------------------------
if [ ! -f ${JLINK_DIR}/run.jlink ]; then 
   echo "Can not find the jlink installation."          
   echo "$me test FAIL, rc=$rc"
   rc=1
fi

mkdir $tmpdir
need_jsmd=`grep -c "jsrun" ${JLINK_DIR}/run.jlink`
stopd=0
args="-d $tmpdir -a"
if [ "$need_jsmd" -ne "0" ]; then
   source $thisdir/../common/jsm_functions
   # check if we there is jsm daemon running
   is_running=`/usr/bin/pgrep jsmd`
   if [ -z "$is_running" ]; then 
      # not running, no need to modify the host.file
      start_jsmd $MP_HOSTFILE
      echo "hostfile $hostfile content is:"
      cat $MP_HOSTFILE
      stopd=1
      args+=" -c"
   fi
else
   # need to modify the host.file
   cat $MP_HOSTFILE | sed 's/$/ slots=1/' > $tmpdir/host.list
   mv $tmpdir/host.list $MP_HOSTFILE
   echo "hostfile $hostfile content is:"
   cat $MP_HOSTFILE
   args+=" -f $MP_HOSTFILE"
fi
echo -e "\nRunning: ${JLINK_DIR}; ./run.jlink ${args} tee $tmpdir/hcdiag_jlink_results"
cd ${JLINK_DIR}; ./run.jlink ${args} | tee $tmpdir/hcdiag_jlink_results
rc=$?

if [ "$stopd" -eq "1" ]; then stop_jsmd; fi

if $verbose; then
   echo -e "\n================================================================"
   echo -e "\nPrinting jlink raw output file(s)"
   for ofile in `ls $tmpdir/out*` ; do
      echo -e "\nFile: $ofile"
      cat $ofile
   done
   echo -e "\n================================================================"
fi


# Even if rc is zero, we need to parse the ouput, because of this:
# -- Tested 56 IB paths and found 14 below threshold (25.00 %).
# -- Tested 56 IB paths out of 56 (untested paths: 0) (tested: 100.00 %).
if [ "$rc" -eq "0" ]; then
   line1=`grep "paths and found" $tmpdir/hcdiag_jlink_results`
   line2=`grep "IB paths out of" $tmpdir/hcdiag_jlink_results`
   tested_paths=`echo "$line1" | awk '{ print $2 }'`
   bellow_thresh=`echo "$line1" | awk '{ print $7 }'`
   total_paths=`echo "$line2" | awk '{ print $7 }'`
   if [ "$tested_paths" != "$total_paths" ]; then
      rc=50
   else 
      if [ "$tested_paths" == "0" ]; then
         rc=51
      else 
         if [ "$bellow_thresh" != "0" ]; then
           rc=52
         fi
      fi
   fi
fi


if [ $rc -eq 0 ]; then echo "$me test PASS, rc=$rc"; exit 0; fi
echo "$me test FAIL, rc=$rc"

exit $rc
