#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/hxestorage/hxestorage.sh
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
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi

readonly me=${0##*/}
thisdir=`dirname $0`

source $thisdir/../common/htx_functions
source $thisdir/../common/functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on `hostname -s`, machine type $model."          

[ $# -lt 3 ] && echo "Usage: $me <sd|nvme> <duration> <pass> [args]" && exit 1
module=$1
duration=$2
pass=$3
if [ $pass -ne 1 -a $pass -ne 2 ]; then echo "Invalid pass $pass. Valid values: 1, 2."; exit 1; fi
if [ "$module" != "sd" ] &&  [ "$module" != "nvme" ]; then echo "Invalid argument: $module"; exit 1; fi


# validates the enviroment to run htx and starts the htxd daemon
htxdiag_setup $debug
rc=$ret
if [ $rc -ne 0 ]; then echo "$me test FAIL, rc=$rc"; exit $rc; fi 

# sd and nvme are devices that change overtime, make sure we can still run diag on them
htxdiag_check_device $module 1
rc=$ret
if [ "$rc" -ne "0" ]; then htxdiag_cleanup; echo "$me test FAIL, rc=$rc"; exit $rc; fi
if [ "$n_dev" -eq "0" ]; then htxdiag_cleanup; echo -e "No devices to be tested.\nme test PASS, rc=0."; exit 0; fi


ext=":storage.coral.pass$pass"
sep=".*"
thismdt="$htxd_root/mdt/mdt.hxestorage_"
if [ $# -eq 3 ]; then
   rule="$module$sep$ext"
   thismdt+="$module."
else
   rule=""; i=3
   while [ $i -lt $# ]; do
      i=$[$i+1]
      echo ${dev_list[*]} | grep -q ${!i}
      if [ $? -ne 0 ]; then htxdiag_cleanup; echo -e "Invalid argument ${!i} for module $module.\n $me FAIL, rc=1"; exit 1; fi 
      rule+="${!i}$ext "
      thismdt+="${!i}."
   done
fi
thismdt+="coral_pass$pass"
#echo "==> thismdt: $thismdt"
#echo "==> rule: $rule"

# creates the mdt file to run hxestorage
htxdiag_get_mdt "$rule" $thismdt
rc=$ret
if [ $rc -eq 0 ]; then 
   # run the test and validates
   htxdiag_run $thismdt $duration $debug
   rc=$ret
fi


# remove the files and stop the daemon
htxdiag_cleanup

if [ $rc -eq 0 ]; then
  echo "$me test PASS, rc=0"
else
  echo "$me test FAIL, rc=$rc"
fi

exit $rc
