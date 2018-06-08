#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-free-memory/chk-free-memory.sh
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


# This script checks if the free memory is lower the min accepted
# Minimal threshold accepted, in percentage
MIN_THRESHOLD=10

if [ $# -gt 0 ]; then MIN_THRESHOLD=$2; fi

me=$(basename $0) 
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $me on $(hostname -s), machine type $model.\n"          

trap 'rm -f /tmp/$$' EXIT

mtotal=`grep MemTotal /proc/meminfo|awk '{print $2}'`
mfree=`grep MemFree /proc/meminfo |awk '{print $2}'`
min=$(( $mtotal * $MIN_THRESHOLD/100 )) 

echo "Totalmemory     : $mtotal"
echo "Free memory     : $mfree"
echo "Min free memory : $min"

if (( $(echo "$mfree < $min" | bc -l) )); then
  echo -e "$me test FAIL, rc=1"  
  exit 1
fi

echo -e "$me test PASS, rc=0"
exit 0

   
