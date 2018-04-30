#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/hxediag/hxediag.sh
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

readonly me=${0##*/}
thisdir=`dirname $0`

[ $# -lt 2 ] && echo "Usage: $me <eth|ib> <duration> [args]" && exit 1
if ! [[ $2 =~ ^[0-9]+$ ]] ; then echo "error: Duration has to be a number"; exit 1; fi

source $thisdir/../common/htx_functions
source $thisdir/../common/network_functions
source $thisdir/../common/functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on `hostname -s`, machine type $model."          


module=$1
duration=$2

# check which ib adapter is active
if [ "$module" == "ib" ]; then
   active_ib_ca
   if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi
   ca=( $ibargs ); nca=${#ca[@]}
   if [ "$nca" -eq "0" ]; then echo -e "No active IB adapters found.\n$me test FAIL, rc=3"; exit 3; fi
   ext=":default.lassen"

# check which ethernet adapter is active
elif [ "$module" == "eth" ]; then
   active_ethernet
   if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi
   if [ "$nenp" -eq "0" ]  && [ "$neth" -eq "0" ]; then echo -e "No active Ethernet adapters found.\n$me test PASS, rc=0"; exit 0; fi
   ext=":diag.coral.pass1"
else
   echo "Invalid argument: $module"; exit 1
fi

thismdt="$htxd_root/mdt/mdt.hxediag_"
rule=""

# this is the generic case, do it for all adapters
if [ $# -eq 2 ]; then
   if [ "$module" == "ib" ]; then
      i=-1
      while (( i++ < $nca-1 )); do
         rule+="${ca[$i]}$ext "
         thismdt+="${ca[$i]}."
      done
   else
      if [ "$nenp" -ne "0" ]; then rule="en.*$ext";  thismdt+="en.";  fi
      if [ "$neth" -ne "0" ]; then rule+="eth.*$ext"; thismdt+="eth.";  fi
   fi

# this is the specific case
# we think you know what you are doing... just basic checking!
else
   i=2
   while [ $i -lt $# ]; do
      i=$[$i+1]
      # for ethenet it should star with: eth, enp or enP 
      if [ "$module" == "eth" ]; then
         if [ "$nenp" -ne "0" ]; then
            if ! [[ "${!i}" =~ ^enp.*|^enP.* ]]; then echo -e "Invalid argument ${!i} for ethernet adapter.\n$me test FAIL, rc=1"; exit 1; fi
         fi
         if [ "$neth" -ne "0" ]; then
            if ! [[ "${!i}" =~ ^eth.* ]]; then echo -e "Invalid argument ${!i} for ethernet adapter.\n$me test FAIL, rc=1"; exit 1; fi
         fi
      else
         if ! [[ "${!i}" =~ ^mlx5_.*|^mlx4_.* ]]; then echo -e "Invalid argument ${!i} for IB adapter.\n $me test FAIL, rc=1"; exit 1; fi
      fi
      rule+="${!i}$ext "
      thismdt+="${!i}."
   done
fi
thismdt+="coral"

rc=0
# validates the enviroment to run htx and starts the htxd daemon
htxdiag_setup $debug
rc=$ret

if [ $rc -eq 0 ]; then 

   # creates the mdt file to run hxediag
   htxdiag_get_mdt "$rule" $thismdt
   rc=$ret
   if [ $rc -eq 0 ]; then 
      # run the test and validates
      htxdiag_run $thismdt $duration $debug
     rc=$ret
   fi

   # remove the files and stop the daemon
   htxdiag_cleanup
fi

if [ "$rc" -eq "0" ]; then
  echo "$me test PASS, rc=0"
else
  echo "$me test FAIL, rc=$rc"
fi

exit $rc
