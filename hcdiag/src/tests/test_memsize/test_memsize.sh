#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/test_memsize/test_memsize.sh
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
# check MemTotal in GB
readonly TESTNAME=$0

[ $# -ne 1 ] && echo "Usage: $TESTNAME <total memsize in GB>" && exit
readonly EXPECTED_MEMSIZE=$1

memsize_gb=$(awk '/^MemTotal:/{printf "%d",$2/1048576}' /proc/meminfo)

if [ $memsize_gb -ge $EXPECTED_MEMSIZE ]; then
	rc=0
	echo "MemTotal of ${memsize_gb}GB is greater or equal to ${EXPECTED_MEMSIZE}GB"
	echo "$TESTNAME test PASS, rc=$rc"
else
	rc=1
	echo "MemTotal of ${memsize_gb}GB is less than expected ${EXPECTED_MEMSIZE}GB"
	echo "$TESTNAME test FAIL, rc=$rc"
fi

exit $rc
