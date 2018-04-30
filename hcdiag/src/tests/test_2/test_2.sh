#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/test_2/test_2.sh
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

if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi

me="test_2.sh"
[ $# -ne 1 ] && echo "Usage: $me N" && exit

echo "Running $me on `hostname -s`."
for i in $(seq 1 $1); do
sleep 1;
echo $i
done;
rc=$?

if [[ $rc -eq 0 ]]; then
   cat test_2.input
   rc=$?
fi

if [[ $rc -eq 0 ]]; then
   echo "$me test PASS, RC= $rc"
else
   echo "$me test FAIL, RC= $rc"
fi

exit $rc
