#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-csm-health.sh
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

model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $(basename $0) on $(hostname -s), machine type $model.\n"          
CIHC=/opt/ibm/csm/bin/csm_infrastructure_health_check
if [ ! -x $CIHC ]; then echo "$CIHC executable not found or invalid permission"  ; exit -1; fi

readonly me=${0##*/}
trap 'rm -f /tmp/$$' EXIT

${CIHC} | tee /tmp/$$
rc=`grep "Test complete: rc" /tmp/$$`
if [ -z "$rc" ]; then
  rc=1
else
  rc=`echo $rc | tr -d '\n' | cut -d'=' -f2`
fi

if [ $rc == "0" ]; then
  echo "$me test PASS, rc=0"  
else
  echo "$me test FAIL, rc=$rc"  
fi

exit $rc 
  
