#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-nfs-mount/chk-nfs-mount.sh
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
model=$(cat /proc/device-tree/model | awk '{ print substr($1,1,8) }')
echo -e "Running $(basename $0) on $(hostname -s), machine type $model.\n"          

count=0
for fs in "$@"; do
   timeout 30 ls $fs 1> /dev/null 2>&1
   if [ $? -eq 0 ]; then
       if [ `findmnt -n -o FSTYPE $fs` == "nfs" ]; then
          echo "$fs mount is correct"
       else
          echo "ERROR: $fs mount is incorrect"
          let count+=1
       fi
   else
      echo "ERROR: $fs is not mounted or hung"
      let count+=1
   fi
done


if [ "$count" -eq "0" ]; then
   echo "$me test PASS, rc=0"
else
   echo "$me test FAIL, rc=1"
fi

exit $count

