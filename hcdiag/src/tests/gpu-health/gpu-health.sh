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

# NOTE: we distribute only the source code of the gpu-health test,
#       under /opt/ibm/csm/hcdiag/samples/gpu-health.
#       Binary is expected to be in this directory. 
#       Otherwise, update the binary location below.
thisdir=`dirname $0`
GPU_HEALTH=$thisdir/gpu-health

EXPECTED_NVLINK_XFER_SPEED=65      # GB/s
EXPECTED_GPU_MEM_BANDWITH=800      # GB/s
EXPECTED_DGEMM_FLOPS=6.7           # TFlops
if [ $# -gt 0 ]; then EXPECTED_NVLINK_XFER_SPEED=$1; fi
if [ $# -gt 1 ]; then EXPECTED_GPU_MEM_BANDWITH=$2; fi
if [ $# -gt 2 ]; then EXPECTED_DGEMM_FLOPS=$3; fi

# around 65 GB/sec for nvlink transfer to devices in the same socket ... 
# around 800 GB/sec for bandwidth, 
# around 10.5 TFlops dgemm 
# checks gpu memory bw, gpu dgemm flops, nvlink transfer speeds 



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
echo "Running $me on `hostname -s`, machine type $model."          

if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ -z $is_boston ]; then 
   echo -e "Could not determine if the machine has GPUs by model. Continuing.."
   is_boston=False
fi 
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 



# check if machine has GPUs
# ===========================
has_gpus
rc=$ret
if [ "$rc" -ne "0" ]; then echo "$me test FAIL, rc=$rc"; exit $rc; fi 
if ([ "$ngpus" -ne "4" ] && [ "$ngpus" -ne "6" ]) ; then echo -e "Unsupported number of gpus: $ngpus\n. $me test FAIL, rc=1"; exit 1; fi 

export OMP_NUM_THREADS=1
n=${#eyecatcher[@]}
err=0
sockets=( `numactl --hardware |grep cpus|awk 'NF>3 {print $2}'`)
cpus=( `numactl --hardware |grep cpus|awk 'NF>3 {print $4}'`)

output_lines=$(($ngpus/2))
# check devices on socket 0 and 1
let j=0; let k=0
for socket in ${sockets[@]}; do
   i=0
   gpus="${k}"
   k=$((k+1)); gpus+=",${k}"
   if [ "$ngpus" -eq "6" ]; then
      k=$((k+1)); gpus+=",${k}"
   fi
   export CUDA_VISIBLE_DEVICES="${gpus}"

   cmd="numactl -N $socket --membind $socket $GPU_HEALTH 1>/tmp/$$ 2>&1"

   export GOMP_CPU_AFFINITY="${cpus[$j]}"
   eval ${cmd}
   rc=$?
   echo -e "\nUsing GOMP_CPU_AFFINITY=$GOMP_CPU_AFFINITY"
   cat /tmp/$$
   if [ $rc -eq 0 ]; then
      # let's parse the output
      while [ $i -lt $n ]; do 
         counter=0 
         echo -e "\n"
         while read -r line; do
            aline=($line)
            value=${aline[${pos[$i]}]}
            if (( $(echo "$value < ${EXPECTED_VALUE[$i]}" | bc -l) )); then
               echo "Checking GPU $counter: ${eyecatcher[$i]}"
               echo -e "ERROR, expecting: ${EXPECTED_VALUE[$i]} ${unit[$i]}, got: $value ${unit[$i]}."
               let err+=1
            else 
               echo "Checking GPU $counter: ${eyecatcher[$i]}. ok"
            fi   
            let counter+=1
         done < <(grep "${eyecatcher[$i]}" /tmp/$$)
         if [  $counter -ne $output_lines ]; then
            echo "ERROR: not enough lines with: ${eyecatcher[$i]}."
            err+=1
         fi
         let i+=1
      done   
   else 
      echo "$GPU_HEALTH error, rc=$rc"
   fi
   j=$((j+1)); k=$((k+1))
done   


if [ $err -eq 0 ] && [ $rc -eq 0 ] ; then
  echo -e "\n$me test PASS, rc=0"
else
  if [ $rc -eq 0 ]; then rc=1; fi
  echo -e "\n$me test FAIL, rc=$rc"
fi

exit $rc              
