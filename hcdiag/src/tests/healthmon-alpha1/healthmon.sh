#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/healthmon/healthmon.sh
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


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64
HTOOL=gdk/usr/bin/nvidia-healthmon
HCONF=nvidia-healthmon-fst.conf
ARGS="-e"

if [[ $# -gt 0 ]]; then
   HCONF=$1
fi

if [ ! -r $HCONF ]; then echo "$HCONF configuration file not found or invalid permission"; exit -2; fi
#####
echo "Invoking $HTOOL with config file $HCONF"
$HTOOL $ARGS -c $HCONF -v
rc=$?
if [ $rc -ne 0 ]; then 
   echo "nvidia-healthmon FAIL RC= $rc"
else
   echo "nvidia-healthmon PASS, RC= $rc"
fi
#####


exit $rc
