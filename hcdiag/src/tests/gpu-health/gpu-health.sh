#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/gpu-health/gpu-health.sh
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

# NOTE: the binary for this test is distributed as samples.
#       Binary is expected to be in this directory. 
#       Otherwise, update the binary location below.
thisdir=`dirname $0`
GPU_HEALTH=$thisdir/gpu-health

EXPECTED_NVLINK_XFER_SPEED=40    # GB/s
EXPECTED_GPU_MEM_BANDWITH=800    # GB/s
EXPECTED_DGEMM_FLOPS=7           # TFlofs
if [ $# -gt 0 ]; then EXPECTED_NVLINK_XFER_SPEED=$1; fi
if [ $# -gt 1 ]; then EXPECTED_GPU_MEM_BANDWITH=$2; fi
if [ $# -gt 2 ]; then EXPECTED_DGEMM_FLOPS=$3; fi

# around 40 GB/sec for nvlink transfer to devices in the same socket ... 
# around 800 GB/sec for bandwidth, 
# around 7 TFlops dgemm 
# checks gpu memory bw, gpu dgemm flops, nvlink transfer speeds 


export CUDA_VISIBLE_DEVICES=0,1,2,3,4,5
export OMP_NUM_THREADS=10
export GOMP_CPU_AFFINITY="0-80:8"

EXPECTED_VALUE=( $EXPECTED_NVLINK_XFER_SPEED $EXPECTED_NVLINK_XFER_SPEED $EXPECTED_GPU_MEM_BANDWITH $EXPECTED_DGEMM_FLOPS ) 
 

eyecatcher=( "host to device transfer rate from pinned" 
             "device to host transfer rate from pinned" 
             "GPU daxpy bandwidth" 
             "GPU dgemm TFlops" )
pos=( 9 9 4 4 )
unit=( GB/sec GB/sec GB/sec TFlops )


readonly me=${0##*/}
thishost=`hostname -s`
trap 'rm -rf /tmp/$$' EXIT                                

source $thisdir/../common/functions
source $thisdir/../common/gpu_functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running $me on $thishost, machine type $model."          
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 


# check if machine has GPUs
# ===========================
has_gpus
rc=$ret
if [ "$rc" -ne "0" ]; then echo "$me test FAIL, rc=$rc"; exit $rc; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 

n=${#eyecatcher[@]}
err=0
i=0

$GPU_HEALTH 1>/tmp/$$ 2>&1
rc=$?
cat /tmp/$$
if [ $rc -eq 0 ]; then
   # let's parse the output
   while [ $i -lt $n ]; do 
      counter=0 
      echo ""
      while read -r line; do
         echo -e "Checking GPU $counter: ${eyecatcher[$i]}"
         aline=($line)
         value=${aline[${pos[$i]}]}
         ivalue=${value%.*}
         if [ $ivalue -lt ${EXPECTED_VALUE[$i]} ]; then
            echo "Error, expecting: ${EXPECTED_VALUE[$i]} ${unit[$i]}, got: $value ${unit[$i]}."
            let err+=1
         fi   
         let counter+=1
      done < <(grep "${eyecatcher[$i]}" /tmp/$$)
      if [  $counter -ne $ngpus ]; then
         echo "Some error occurred, not enough lines with: ${eyecatcher[$i]}."
         err+=1
      fi
      let i+=1
   done   
fi


if [ $err -eq 0 ] && [ $rc -eq 0 ] ; then
  echo "$me test PASS, rc=0"
else
  if [ $rc -eq 0 ]; then rc=1; fi
  echo "$me test FAIL, rc=$rc"
fi

exit $rc              
