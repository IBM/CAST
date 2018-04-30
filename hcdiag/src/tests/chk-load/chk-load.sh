#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-load/chk-load.sh
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


# This script checks the cpu or memory load,
# It returns 0 if the load <= threshold, 1 otherwise.
# 
LOAD_THRESHOLD=90
TOTAL_PROCESS=1
UNIT=cpu

if [ $# -gt 0 ]; then LOAD_THRESHOLD=$1; fi
if [ $# -gt 1 ]; then TOTAL_PROCESS=$2; fi
if [ $# -gt 2 ]; then UNIT=$3; fi

me=$(basename $0) 
model=$(cat /proc/device-tree/model | awk '{ print substr($1,1,8) }')
echo -e "Running $me on $(hostname -s), machine type $model.\n"          


trap 'rm -f /tmp/$$' EXIT
if [ "$UNIT" == "mem" ] || [ "$UNIT" == "cpu" ]; then  
   let TOTAL_PROCESS+=1
   if [ "$UNIT" == "cpu" ] ; then  
      ps -eo pid,ppid,user,%cpu,%mem,state,lstart,cmd --sort=-%cpu |head -$TOTAL_PROCESS | tee /tmp/$$
   else
      ps -eo pid,ppid,user,%mem,%cpu,state,lstart,cmd --sort=-%mem |head -$TOTAL_PROCESS | tee /tmp/$$
   fi

   total=`tail -${TOTAL_PROCESS} /tmp/$$| awk '{s+=$4}END {print s}'` 

   let TOTAL_PROCESS-=1
   echo -e "\n$TOTAL_PROCESS process(es) using $total% $UNIT."
   echo -e "Threshold set to $LOAD_THRESHOLD%."
   if (( $(echo "$total <= $LOAD_THRESHOLD" | bc -l) )); then
      echo -e "$me test PASS, rc=0"  
      exit 0
   fi
else
   echo "Invalid argument: $INIT. Valid values are: mem|cpu."
fi

echo -e "$me test FAIL, rc=1"  
exit 1
