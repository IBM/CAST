#!/bin/bash  

#================================================================================
#   
#    hcdiag/src/tests/gpfsperf/gpfsperf.sh
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

# gpsferp is a simple benchmark program (gpfsperf) that can be used to
# measure the performance of GPFS for several common file access patterns.
# it is shipped under /usr/lpp/mmfs/samples/perf

## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi

GPFS_DIR=/usr/lpp/mmfs
TEST_BIN=${GPFS_DIR}/samples/perf/gpfsperf

# Adjust the name of the GPFS filesystem name, block size and test dir
# if they are not suplied as paramenter
# make sure the user running the test have permission in $TEST_DIR
BLOCK_SIZE=1M

readonly me=${0##*/}
[ $# -lt 1 ] && echo "Usage: $me gpfs_fs [block_size] [test_dir]" && exit 1
GPFS_FS=$1
if [ $# -gt 1 ]; then BLOCK_SIZE=$2; fi 
TEST_DIR=${GPFS_FS}/diagadmin
if [ $# -gt 2 ]; then TEST_DIR=$3; fi 

TEST_FILE=${TEST_DIR}/hdiag-gpfs.test$$

# Define here the tests that you want to perform.
# Use "-" if you don't care about the values data_rate, op_rate, avg_latency and thread_untilization
# Example: "read randhint"   "read randhint ${TEST_FILE} -r 10000 -n 100m"              569130.66  542.77   1.842     -
TESTS=(
# id              args                                                       data_rate op_rate  avg_latency thread_utilization
"create"          "create seq ${TEST_FILE} -n 10g -r ${BLOCK_SIZE} -fsync"   720000.00  690.20   2.000       -
"read sequencial" "read seq ${TEST_FILE} -r 1M"                              549130.66  540.77   2.000       -  
"read rand"       "read rand ${TEST_FILE} -r 10000 -n 100m"                    4500.00  440.77   2.000       -
"read randhint"   "read randhint ${TEST_FILE} -r 10000 -n 100m"               19000.00 2500.00   1.000       -
)

THRESHOLD_ID=("Data rate" "Op rate" "Avg Latency" "thread utilization")  
THRESHOLD_OP=(0 0 1 1)   # set 0: the values in TESTS table is the min; 1: value is the max
UNITS=("Kbytes/sec" "Ops/sec" "milliseconds" "") 


thishost=`hostname -s`
thisdir=`dirname $0`
source $thisdir/../common/functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running $me on $thishost, machine type $model."          

trap 'rm -f ${TEST_FILE}' EXIT
trap 'rm -f /tmp/$$' EXIT

# if executable exist
#--------------------
if [ ! -x ${TEST_BIN} ]; then 
   echo "Error: can not find the gpfsperf binary."          
   echo "$me test FAIL, rc=1"
   exit 1
fi

i=0
ret=0
ntests=${#TESTS[@]}
ntests=$((ntests / 6))
isfirst=1
while [[ $i -lt ${#TESTS[@]} ]] ; do
  echo -e "\n\n"
  name="${TESTS[$i]}"; let i+=1
  ${TEST_BIN} ${TESTS[$i]} 1>/tmp/$$ 2>&1
  rc=$?
  cat /tmp/$$
  let i+=1
  if [ "$rc" -eq "0" ]; then
     values=(`grep "Data rate" /tmp/$$ | awk '{ gsub(","," "); printf "%s %s %s %s", $4, $9, $14, $18 }'` )

     echo -e "\nSummary for test $name."
     for ((j=0; j<${#THRESHOLD_ID[@]};j++)); do
        let k=$i+$j
        if [ "${TESTS[$k]}" == "-" ]; then
           # we don not care about this test
           echo "${THRESHOLD_ID[$j]} ignored." 
        else
           msg=""
           if [ "${THRESHOLD_OP[$j]}" == "0" ]; then
              if (( $(echo "${values[$j]} < ${TESTS[$k]}" | bc -l) )); then msg="ERROR: "; let ret+=1;  fi
           else
              if (( $(echo "${values[$j]} > ${TESTS[$k]}" | bc -l) )); then msg="ERROR: "; let ret+=1;  fi
           fi
           echo "${msg}${THRESHOLD_ID[$j]} was ${values[$j]} "${UNITS[$j]}", threshold ${TESTS[$k]} "${UNITS[$j]}"." 
        fi
     done
  else
    echo "Error executing test id $name, rc = $rc."
    # if creates fail, no point of continuing
    if ((isfirst)); then 
       echo "$me test FAIL, rc=$rc"
       exit $rc
    fi
  fi
  let i+=${#THRESHOLD_ID[@]}
  if ((isfirst)); then isfirst=0; fi
done

echo -e "\n"
if [ "$ret" -eq "0" ]; then
   echo "$me test PASS, rc=0"
   exit 0
fi

echo "$me test FAIL, rc=$ret"
exit $ret
  
