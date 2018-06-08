#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-idle-state/chk-idle-state.sh
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

me=$(basename $0) 
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $me on $(hostname -s), machine type $model.\n"          

idle_state=3
eyecatcher1=DISABLED
eyecatcher2="Number of idle states"

cmd=/usr/bin/cpupower
args=idle-info
tmpf=/tmp/$$

trap 'rm -f $tmpf' EXIT

[ ! -x $cmd ] && echo "$cmd command not not found" && echo "$me  test FAIL, rc=1" &&  exit 1;
rc=0
$cmd $args| tee $tmpf
n_idle_state=`grep "$eyecatcher2" $tmpf | awk '{print $5}'` 
if [ -n "$n_idle_state" ]; then
   if  [ "$n_idle_state" -eq "$idle_state" ]; then 
      # look for DISABLED
      while IFS= read -r line; do 
         if [ ! -z $line ]; then
            echo "ERROR: `echo $line | cut -d':' -f1`"
            rc=1
         fi
      done <<< $(grep $eyecatcher1 $tmpf)
   else
      echo "ERROR: $eyecatcher2, expecting: $idle_state, got: $n_idle_state" 
      rc=2
   fi
else 
   echo "ERROR: Expecting: $eyecatcher2: $idle_state, got: 0" 
   rc=1
fi
if [ $rc -eq 0 ]; then
    echo "$me test PASS, rc=$rc" 
else
  echo "$me test FAIL, rc=$rc"
fi

exit $rc

   
