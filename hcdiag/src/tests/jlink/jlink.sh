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


[ -z $MP_HOSTFILE ] && echo "MP_HOSTFILE not set. Exiting." && exit 1

# spectrum mpi install
#S_BINDIR=/opt/ibm/spectrum_mpi/healthcheck/jlink
S_BINDIR=/opt/ibm/spectrum_mpi/healthcheck/mpirun_scripts/jlink
export PATH=$PATH:$S_BINDIR

# jlink tests the bandwidth for all possible node pairings.
# Witherspoon, Garrison, Firestone machines are supported

export PATH=$PATH:/usr/sbin
readonly me=${0##*/}
thisdir=`dirname $0`

source $thisdir/../common/functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running $me on `hostname -s`, machine type $model."          


trap 'rm -rf /tmp/$$' EXIT
tmpdir=/tmp/$$

#set -x          

# It is the jlink version that comes with spectrum mpi
#------------------------------------------------------
if [ ! -f $S_BINDIR/run.jlink ]; then 
   echo "Can not find the jlink installation."          
   echo "$me test FAIL, rc=$rc"
   rc=1
fi

mkdir $tmpdir
need_jsmd=`grep -c "jsrun" $S_BINDIR/run.jlink`
stopd=0
if [ "$need_jsmd" -ne "0" ]; then
   source $thisdir/../common/jsm_functions
   # check if we there is jsm daemon running
   run_flag=""
   is_running=`/usr/bin/pgrep jsmd`
   if [ -z "$is_running" ]; then 
      # not running, no need to modify the host.file
      start_jsmd $MP_HOSTFILE
      echo "hostfile $hostfile content is:"
      cat $MP_HOSTFILE
      stopd=1
      run_flag="-c"
   fi
   cmd="cd $S_BINDIR; ./run.jlink $run_flag -d $tmpdir -a | tee $tmpdir/hcdiag_jlink_results"
else
   # need to modify the host.file
   cat $MP_HOSTFILE | sed 's/$/ slots=1/' > $tmpdir/host.list
   mv $tmpdir/host.list $MP_HOSTFILE
   echo "hostfile $hostfile content is:"
   cat $MP_HOSTFILE
   cmd="cd $S_BINDIR; ./run.jlink -f $MP_HOSTFILE -d $tmpdir -a | tee $tmpdir/hcdiag_jlink_results"
fi

echo -e "\nRunning: $cmd"
eval $cmd
rc=$?

if [ "$stopd" -eq "1" ]; then stop_jsmd; fi

echo -e "\n================================================================"
echo -e "\nPrinting jlink raw output file(s)"
for ofile in `ls $tmpdir/out*` ; do
   echo -e "\nFile: $ofile"
   cat $ofile
done

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


echo -e "\n"
if [ $rc -eq 0 ]; then
   echo "$me test PASS, rc=$rc"
else
   echo "$me test FAIL, rc=$rc"
fi

exit $rc
