#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-smt/chk-smt.sh
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

SMT=4
if [ $# -gt 0 ]; then SMT=$1; fi

smt=`/usr/sbin/ppc64_cpu --smt -n`;
smt=`echo $smt | cut -d '=' -f2`

if [ "$smt" -eq "$SMT" ]; then
   echo "Node SMT=$smt"
   echo "$me test PASS, rc=0"
   exit 0
fi

echo "Node SMT expected $SMT, got: $smt"
echo "$me test FAIL, rc=1"
exit 1

   
