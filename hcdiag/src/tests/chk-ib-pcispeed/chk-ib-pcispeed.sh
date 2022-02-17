#!/bin/bash
#================================================================================
#   
#    hcdiag/src/tests/chk-ib-pcispeed/chk-ib-pcispeed.sh
# 
#  © Copyright IBM Corporation 2015-2022. All Rights Reserved
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

SPEED=16GT/s
WIDTH=x8
VENDOR=Mellanox

if [ $# -gt 1 ]; then SPEED=$1; fi
if [ $# -gt 2 ]; then WIDTH=$2; fi
if [ $# -gt 3 ]; then VENDOR=$3; fi

me=$(basename $0) 
thishost=`hostname -s`
thisdir=`dirname $0`
source $thisdir/../common/functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on $thishost, machine type $model."          


count=0
err=0
for a in `lspci |grep ${VENDOR} | awk '{print $1}'`; do
   line=`sudo lspci -s $a -vv | grep "LnkSta:"`
   speed="$(echo "${line}" | awk 'match($0, /Speed\s*([0-9]+GT\/s)/, a) {print a[1]}')"
   width="$(echo "${line}" | awk 'match($0, /Width\s*(x[0-9]+)/, a) {print a[1]}')"
   echo "Adapter: $a, $speed, $width."
   if [ "$SPEED" != "$speed" ]; then
      echo "Error, expecting: $SPEED, got: $speed"
      let err+=1
   fi
   if [ "$WIDTH" != "$width" ]; then
      echo "Error, expecting:  $WIDTH, got: $width"
      let err+=1
   fi
   let count+=1
done

echo -e "Found $count $VENDOR adapters.\n"

if [ $err == 0 ]; then
   echo "$me test PASS, rc=0"  
else
   echo "$me test PASS, rc=$err"  
fi

exit $err

