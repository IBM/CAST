#!/bin/bash
#================================================================================
#   
#    hcdiag/src/tests/chk-ib-node/chk-ib-node.sh
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

EXPECTED_IB_BANDWITH="22000.00"
if [ $# -gt 0 ]; then EXPECTED_IB_BANDWITH=$1; fi

readonly me=${0##*/}
thishost=`hostname -s`
thisdir=`dirname $0`
source $thisdir/../common/functions
supported_machine
echo "Running $me on `hostname -s`, machine type $model."          
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

running=$(ps -ef | grep ib_  | grep -v ] | grep -v "grep" | grep -v $0 | wc -l)
portcheck=$(netstat -a | grep 20000 | wc -l)
if [ "$running" -ne "0" ]; then
    echo "WARNING: Other verbs tests running on system. Results may be inaccurate."
    ps -ef | grep ib_  | grep -v ] | grep -v "grep" | grep -v $0
fi

echo "Minimum bandwidth set to: $EXPECTED_IB_BANDWITH MB/s"
if [ "$portcheck" -ne "0" ]; then
    echo "ERROR: Network port in use.  Test cannot execute."
    ps -ef | grep ib_  | grep -v ] | grep -v "grep" | grep -v $0
    echo "$me test FAIL, rc=3"
    exit 2
fi

taskset -c 84 ib_write_bw --size=8M -d mlx5_0 -b --port=20000 -D 10 > /dev/null &
sleep .1

bw=$(taskset -c 172 ib_write_bw --size=8M -d mlx5_3 -b --port=20000 -D 10 --output=bandwidth localhost)
if [ -z "$bw" ]; then 
   # do some cleanup if necessary
   pid=`pgrep ib_write_bw`
   if [ -n "$pid" ]; then kill $pid; fi
   echo "$me test FAIL, rc=3"
   exit 3
fi

if (( $(echo "$bw < $EXPECTED_IB_BANDWITH" | bc -l) )); then
    echo "ERROR: Node bandwidth, got: $bw MB/s, expecting: $EXPECTED_IB_BANDWITH MB/s"
    echo "$me test FAIL, rc=1"
    exit 1
fi
echo "Bandwidth = $bw MB/s"
echo -e "$me test PASS, rc=0"
exit 0

