#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/ppping.sh
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

PPPING=/opt/xcat/bin/ppping

readonly me=${0##*/}
if [ ! -x $PPPING ]; then echo -e "$PPPING not found or invalid permission.\n$me test FAIL, rc=1"  ; exit 1; fi
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo "Running $me on `hostname -s`, machine type $model."          

trap 'rm -f /tmp/$$' EXIT

export PATH=$PATH:/opt/xcat/sbin
args="$*"
${PPPING} ${args} 2>/tmp/$$ 1>&2
rc=$?
cat /tmp/$$

if [ "$rc" -eq "0" ]; then
  n=`grep noping /tmp/$$ | wc -l`

  if [ $n == '0' ]; then
    echo -e "\n$me test PASS, rc=0"  
    exit 0
  fi
fi

echo -e "\n$me test FAIL, rc=1"  
exit 1
  

