#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/mmhealth/mmhealth.sh
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

# We expect an output:
#Node name:      c699c048-ib0.c699.net
#Node status:    HEALTHY              
#Status Change:  2 days ago           
#
#Component      Status        Status Change     Reasons
#------------------------------------------------------
#GPFS           HEALTHY       2 days ago        -
#NETWORK        HEALTHY       2 days ago        -
#FILESYSTEM     HEALTHY       2 days ago        -
#PERFMON        HEALTHY       2 days ago        -
#THRESHOLD      HEALTHY       2 days ago        -

export me=$(basename $0) 

sudo -E bash <<"EOF"

model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $me on $(hostname -s), machine type $model.\n"          

rc=0
if [ -x /usr/lpp/mmfs/bin/mmhealth ]; then 
  /usr/lpp/mmfs/bin/mmhealth node show | tee 1
  echo ""
  rc=$?
  if [ $rc -eq 0 ]; then 
     status=`grep "Node status:" 1 | awk '{print $3}'`
     if [ ${status} ]; then
        if [ "${status}" != "HEALTHY" ]; then
           echo "ERROR: Node health is: ${status}"  
           rc=1
        fi
     else 
        echo "ERROR: Node health is: ${status}"  
        rc=2
     fi
  else
     echo "ERROR: '/usr/lpp/mmfs/bin/mmhealth node show' command failed"
  fi
else
    echo "ERROR: '/usr/lpp/mmfs/bin/mmhealth' command not found or not executable"
fi

if [ $rc -eq 0 ]; then 
   echo "$me test PASS, rc=0"
else
   echo "$me test FAIL, rc=$rc"
fi


EOF
