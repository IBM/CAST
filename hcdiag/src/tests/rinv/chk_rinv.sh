#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk_rinv/chk_rinv.sh
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

trap 'rm -rf /tmp/$$' EXIT

readonly TESTNAME=$(basename $0)

[ $# -ne 2 ] && echo "Usage: $TESTNAME <node_range> <memory|gpu|cpu|bmc|system|fan|psu|all>" && exit 1

rcmd=/opt/xcat/bin/rinv
xcmd=/opt/xcat/bin/xcoll
if [ !  -x $cmd ]; then
   printf "\n! Warning, executable '$cmd' not found\n"
   exit 1
fi

case "$2" in
   memory)
       $rcmd "$1" | egrep 'CPU|DIMM|MEMBUF' | $xcmd
       ;;
   cpu)
       $rcmd "$1" | egrep 'CPU' | $xcmd 
       ;;
   gpu)
       $rcmd "$1" | egrep 'GPU'| $xcmd 
       ;;             
   system)
       $rcmd "$1" | egrep 'SYSTEM' | $xcmd 
       ;;
   bmc)
       $rcmd "$1" | egrep 'BMC|UUID|Product ID|Manufacturer ID|Device ID' | $xcmd 
       ;;
   psu)       ## power supply unit
       $rcmd "$1" | egrep 'PSU' | $xcmd 
       ;;
   fan)
       $rcmd "$1" | egrep 'FAN' | $xcmd 
       ;;
   all)
       echo "invalid argument"
       $rcmd "$1"  | $xcmd 
       ;;
   *)
       exit 1
esac

exit 0
