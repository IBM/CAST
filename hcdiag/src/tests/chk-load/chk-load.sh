#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-load/chk-load.sh
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


# This script checks the cpu, memory load or average load
# It returns 0 if the load <= threshold, 1 otherwise.
# parameters:
#    UNIT : cpu|mem|average, default is cpu
#    LOAD_THRESHOLD: threshold to be compared to. Default is 90. 
#           If unit is average, please read NOTE, below.
#    TOTAL_PROCESS: the number of process you want to be include in the check. Default is 1. 
#           If unit is average, this parameter is ignored       
#    list of process to exclude (cpu and mem only)

# NOTE:
# System load averages is the average number of processes that are either
# in a runnable or uninterruptable state.  A process  in a  runnable  state  
# is either using the CPU or waiting to use the CPU.  A process in uninterruptableS
# state is waiting for some I/O access, eg waiting for disk.  The averages are 
# taken over the three time intervals.  Load averages are not normalized  for
# the  number of CPUs in a system, so a load average of 1 means a single CPU system 
# is loaded all the time while on a 4 CPU system it means it was idle 75% of the time.


UNIT=cpu
LOAD_THRESHOLD=90
TOTAL_PROCESS=1

if [ $# -gt 0 ]; then UNIT=$1; fi
if [ $# -gt 1 ]; then LOAD_THRESHOLD=$2; fi
if [ $# -gt 2 ]; then TOTAL_PROCESS=$3; fi


me=$(basename $0) 
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $me on $(hostname -s), machine type $model.\n"          

trap 'rm -f /tmp/$$' EXIT

exc=""
if [ "$UNIT" != "mem" ] && [ "$UNIT" != "cpu" ] && [ "$UNIT" != "average" ] ; then  
   echo "Invalid argument: $INIT. Valid values are: mem|cpu."
else
   if [ "$UNIT" == "average" ] ; then  
      n=1   # 1 for 1min, 2 for 5 min, 3 for 15 min
      avg=`uptime`
      echo "${avg}"
      # expected output:                             past  1min  5min  15min
      # 16:44:20 up 3 days,  6:19, 49 users,  load average: 0.20, 0.25, 0.47
      avg=${avg##*average:}
      avg=`echo "${avg}" | cut -d "," -f $n `
      echo -e "\nload average: $avg."
      echo "Threshold set to $LOAD_THRESHOLD."
      if (( $(echo "$avg <= $LOAD_THRESHOLD" | bc -l) )); then
         echo -e "$me test PASS, rc=0"  
         exit 0
      fi
   else 
      #check if there is a list of exclusions
      if [ $# -gt 3 ]; then 
        i=4
        tmp=()
        while [ $i -le $# ]; do 
          tmp+=( ${!i} )
          let i+=1
        done
        if [ ${#tmp[@]} -ne 0 ]; then
          exc=$(echo ${tmp[@]} | tr ' ' '|')
          exc="| egrep -v '${exc}'"
        fi
      fi
      let TOTAL_PROCESS+=1
      if [ "$UNIT" == "cpu" ] ; then  
         cmd="ps -eo pid,ppid,user,%cpu,%mem,state,lstart,cmd --sort=-%cpu ${exc} |head -$TOTAL_PROCESS | tee /tmp/$$"
      else 
         cmd="ps -eo pid,ppid,user,%mem,%cpu,state,lstart,cmd --sort=-%mem ${exc} |head -$TOTAL_PROCESS | tee /tmp/$$"
      fi
      eval $cmd
      total=`tail -${TOTAL_PROCESS} /tmp/$$| awk '{s+=$4}END {print s}'` 
   
      let TOTAL_PROCESS-=1
      echo -e "\n$TOTAL_PROCESS process(es) using $total% $UNIT."
      echo -e "Threshold set to $LOAD_THRESHOLD%."
      if [ ! -z "$exc" ]; then echo "Excluding: ${tmp[@]}."; fi

      if (( $(echo "$total <= $LOAD_THRESHOLD" | bc -l) )); then
         echo -e "$me test PASS, rc=0"  
         exit 0
      fi
   fi
fi   



echo -e "$me test FAIL, rc=1"  
exit 1
   
