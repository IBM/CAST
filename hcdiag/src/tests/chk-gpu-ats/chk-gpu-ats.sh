#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-gpu-ats/chk-gpu-ats.sh
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

# desired ATS is disabled
desired_ats=0
if [ $# -gt 0 ]; then 
   case $1 in  
      0)  
      ;;
      1) desired_ats=$1
      ;;
      *) echo "Invalid value for ATS. Posible values are: 0, 1"
         echo "$me test FAIL, rc=1"
         exit 1
      ;;
   esac
fi


if [ -r /sys/module/nvidia_uvm/parameters/uvm8_ats_mode ]; then
   ats=`cat /sys/module/nvidia_uvm/parameters/uvm8_ats_mode`
   if [ "$ats" -eq "$desired_ats" ]; then
     echo "ATS is set to $desired_ats."
     echo "$me test PASS, rc=0"
     exit 0
   else
     echo "ATS value expected: $desired_ats, got: $ats."
     echo "$me test FAIL, rc=1"
     exit 1
   fi
fi

echo "Can not read /sys/module/nvidia_uvm/parameters/uvm8_ats_mode"
echo "$me test FAIL, rc=2"
exit 2

   
