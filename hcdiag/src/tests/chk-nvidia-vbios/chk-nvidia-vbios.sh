#!/bin/bash
#================================================================================
#   
#    hcdiag/src/tests/chk-nvidia-vbios/chk-nvidia-vbios.sh
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

set -e
me=$(basename $0) 
thishost=`hostname -s`
thisdir=`dirname $0`
source $thisdir/../common/functions

supported_machine

if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo -e "Running $me on $thishost, machine type $model.\n"          
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 

$thisdir/chk-nvidia-vbios.pm $@

