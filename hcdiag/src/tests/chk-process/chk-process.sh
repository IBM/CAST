#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-process/chk-process.sh
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

PROCS="csmd nvidia-persistenced nv-hostengine opal-prd automount"
me=$(basename $0) 
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $me on $(hostname -s), machine type $model.\n"          

procs=($PROCS)
if [ $# -ne 0 ]; then procs=("$@"); fi

total=0
failed=0
for p in "${procs[@]}"; do
  pid=`ps -ef | grep $p | egrep -v 'chk-process|grep' | awk '{ print $2}'`
  if [ -z "$pid" ]; then
    echo "Error: $p is not running"
    let failed+=1
  fi
  let total+=1
done
if [ $failed -eq 0 ]; then
   echo "$total process(es) running."
   echo "$me test PASS, rc=0"
else
  echo "$failed process(es) out of $total is(are) not running."
  echo "$me test FAIL, rc=$failed"  
fi

exit $failed

   
