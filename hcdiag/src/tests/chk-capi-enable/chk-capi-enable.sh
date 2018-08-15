#!/bin/bash
#================================================================================
#   
#    hcdiag/src/tests/chk-capi-enable/chk-capi-enable.sh
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

## mst commands requires sudo
export me=$(basename $0) 
export thisdir=`dirname $0`
export thishost=`hostname -s`
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)

echo "Running $me on $thishost, machine type $model."          
sudo -v
rc=$?
if [ $rc -ne 0 ]; then echo -e "$me test FAIL, rc=$rc"; exit $rc; fi  

sudo -E bash <<"EOF"
mst start

rc=0
for x in `ls /dev/mst/*`; do
   echo -e "\nChecking device: $x"
   if (( $(mlxconfig -d $x q ADVANCED_PCI_SETTINGS | grep "ADVANCED_PCI_SETTINGS" | grep False | wc -l) != "0" )); then
      echo "(ERROR) Checking ADVANCED_PCI_SETTINGS parameter: CAPI NOT ENABLED"
      rc=1
   fi
   if (( $(mlxconfig -d $x q IBM_CAPI_EN | grep "IBM_CAPI_EN" | egrep 'False|Device doesn' | wc -l) != "0")); then
      echo "(ERROR) Checking IBM_CAPI_EN parameter: CAPI NOT ENABLED."
      rc=1
   fi
   if (( $(mlxconfig -d $x q IBM_TUNNELED_ATOMIC_EN | grep "IBM_TUNNELED_ATOMIC_EN" | egrep 'False|Device doesn' | wc -l) != "0")); then
      echo "(ERROR) Checking IBM_TUNNELED_ATOMIC_EN parameter: CAPI NOT ENABLED."
      rc=1
   fi
   if (( $(mlxconfig -d $x q IBM_AS_NOTIFY_EN | grep "IBM_AS_NOTIFY_EN" | egrep 'False|Device doesn' | wc -l) != "0")); then
      echo "(ERROR) Checking IBM_AS_NOTIFY_EN parameter:  CAPI NOT ENABLED."
      rc=1
   fi
done



if [ $rc -ne 0 ]; then echo -e "\n$me test FAIL, rc=$rc"; exit $rc; fi  

echo -e "\nSUCCESS: CAPI enabled for $thishost."
echo -e "\n$me test PASS, rc=0"  
exit 0


EOF
