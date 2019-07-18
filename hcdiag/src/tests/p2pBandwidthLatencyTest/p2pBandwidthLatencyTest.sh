#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/p2pBandwidthLatencyTest/p2pBandwidthLatencyTest.sh
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

# NOTE: the binary for this test is distributed as sample by NVIDIA.
#       Binary is expected to be in this directory. 
#       If not, update the binary location below.

thisdir=`dirname $0`
LATENCY_TEST=$thisdir/p2pBandwidthLatencyTest
# if nvidia-persistenced is not running, start it 
START_PERSISTENCED=1

readonly me=${0##*/}
thishost=`hostname -s`
trap 'rm -rf /tmp/$$' EXIT                                

source $thisdir/../common/functions
source $thisdir/../common/gpu_functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 

echo "Running $me on $thishost, machine type $model."          
if [ -z $is_boston ]; then 
   echo -e "Could not determine if the machine has GPUs by model. Continuing.."
   is_boston=False
fi 
[ $is_boston == True ] && echo -e "Model does not have GPUs.\n$me test PASS, rc=0" && exit 0

# check if machine has GPUs
# ===========================
has_gpus
[ "$ret" -ne "0" ] &&  echo "$me test FAIL, rc=$ret" && exit $ret
[ "$ngpus" -eq "0" ] && echo "$me test FAIL, rc=1" &&  exit 1

# we need to determine if it is a P8 or P9 processor, to check connectivity properly
get_processor
[ "$ret" -ne "0" ] && echo "$me test FAIL, rc=$ret" && exit $ret 

verbose=$1
# P9 example 4 gpus
# Checking it is all is set to 1
# P2P Connectivity Matrix
# D\D     0     1     2     3
#   0     1     1     1     1
#   1     1     1     1     1
#   2     1     1     1     1
#   3     1     1     1     1
# Unidirectional P2P=Enabled Bandwidth Matrix (GB/s)
#   D\D     0      1      2      3 
#     0 752.60  68.46  29.10  28.97 
#     1  68.51 735.64  28.93  28.72 
#     2  28.76  28.63 744.05  69.24 
#     3  29.10  29.01  69.24 737.03 

#Bidirectional P2P=Enabled Bandwidth Matrix (GB/s)
#   D\D     0      1      2      3 
#     0 766.68 136.01  34.43  34.16 
#     1 137.50 763.69  33.90  33.43 
#     2  34.43  34.12 763.64 136.20 
#     3  34.25  33.72 135.02 765.18 

# P8 example
#P2P Connectivity Matrix
#     D\D     0     1     2     3
#     0       1     1     0     0
#     1       1     1     0     0
#     2       0     0     1     1
#     3       0     0     1     1
#Unidirectional P2P=Enabled Bandwidth (P2P Writes) Matrix (GB/s)
#   D\D     0      1      2      3                              
#     0 507.22  35.35  20.27  20.31                             
#     1  35.33 509.12  20.25  20.28                             
#     2  24.52  25.09 509.29  35.35
#     3  24.55  24.92  35.34 509.52
#Bidirectional P2P=Enabled Bandwidth Matrix (GB/s)
#   D\D     0      1      2      3
#     0 519.38  70.53  30.36  30.37
#     1  70.42 516.91  30.51  30.50
#     2  30.48  30.56 517.98  70.42
#     3  30.43  30.57  70.54 516.75


# we are checking only the P2P enabled results
# default values for 4 gpus. Values should be integer

UN_GPU_LOCAL=700             
UN_GPU_GPU=48                
UN_GPU_PPC=24                
BI_GPU_LOCAL=700             
BI_GPU_GPU=96              
BI_GPU_PPC=24                

if [ "${processor}" == "p8" ]; then
  UN_GPU_LOCAL=500            
  UN_GPU_GPU=35                
  UN_GPU_PPC=20                
  BI_GPU_LOCAL=500             
  BI_GPU_GPU=70              
  BI_GPU_PPC=20                
fi

if [ "$ngpus" -eq "6" ]; then 
   UN_GPU_LOCAL=730            
   UN_GPU_GPU=45                 
   UN_GPU_PPC=26                
   BI_GPU_LOCAL=750             
   BI_GPU_GPU=91              
   BI_GPU_PPC=22  
fi

GPU_LOCAL=( 1 $UN_GPU_LOCAL  $BI_GPU_LOCAL)
GPU_GPU=(   1 $UN_GPU_GPU    $BI_GPU_GPU)
GPU_PPC=(   1 $UN_GPU_PPC    $BI_GPU_PPC)

err=0
eyecatcher=( "P2P Connectivity Matrix" 
             "Unidirectional P2P=Enabled Bandwidth" 
             "Bidirectional P2P=Enabled Bandwidth" ) 

#=====================================================
# check_connectivity
# Only if ret is set to 9 we end the test, otherwise 
# keep counting
#=====================================================
function check_p8_connectivity() 
{
   i=$1; j=$2; v=$3; matrix=$4
   ret=0

   # This is P2P Connectivity Matrix (shortcut)
   #============================================
   if [ $matrix -eq 0 ]; then
     if [[ "$v" =~ ^-?[0-9]+$ ]]; then
        # good, it is is integer
        # calculate the diff
        d=$((i-j))
        d=${d#-}
        if [ $d -le 1 ]; then
           if [ $v -eq 1 ]; then 
              if [ $verbose ]; then echo "Connectivity gpus $i, $j, value: $v"; fi
           fi
        fi
        return
     fi
     echo "Error: Connectivity, gpus $i, $j, value: $v."
     ret=9 
     return
   fi

   # All other Matrixes
   #====================
   if [[ "$v" =~ ^-?[0-9]*[.,]?[0-9]*$ ]]; then
      ## echo "$string is a float"
      # GPU_LOCAL
      # ========
      if [ $i -eq $j ]; then
         # this is local
         if [ ${v%.*} -lt ${GPU_LOCAL[$matrix]} ]; then
           echo "Error: gpus $i,$j, expecting min value: ${GPU_LOCAL[$matrix]}, got: $v"
           let err+=1
         else
           if [ $verbose ]; then echo "gpus $i,$j, expecting min value: ${GPU_LOCAL[$matrix]}, got: $v"; fi
         fi
         return
      fi

      # GPU_PPC
      # ========
      if ( [ $i -lt $limit ] && [ $j -ge $limit ] ) || ( [ $i -ge $limit ] && [ $j -lt $limit ] )  ; then
         # this is ppc
         if [ ${v%.*} -lt ${GPU_PPC[$matrix]} ]; then
           echo "Error: gpus $i,$j, expecting min value: ${GPU_PPC[$matrix]}, got: $v"
           let err+=1
         else
           if [ $verbose ]; then echo "gpus $i,$j, expecting min value: ${GPU_PPC[$matrix]}, got: $v"; fi
         fi
         return
      fi

      # GPU_GPU
      # ========
      if [ ${v%.*} -lt ${GPU_GPU[$matrix]} ]; then
         echo "Error: gpus $i,$j, expecting min value: ${GPU_GPU[$matrix]}, got: $v"
         let err+=1
         return
      else
         if [ $verbose ]; then echo "gpus $i,$j, expecting min value: ${GPU_GPU[$matrix]}, got: $v"; fi
      fi

   # not a floating point
   else   
      let err+=1
      echo "Error: gpus $i,$j, invalid value, got: $v" 
   fi
   return
}

function check_p9_connectivity() 
{
   i=$1; j=$2; v=$3; matrix=$4
   ret=0

   # This is P2P Connectivity Matrix (shortcut)
   #============================================
   if [ $matrix -eq 0 ]; then
     if [[ "$v" =~ ^-?[0-9]+$ ]]; then
        # good, it is is integer
        if [ $v -eq 1 ]; then
           if [ $verbose ]; then echo "Connectivity gpus $i, $j, value: $v"; fi
           return
        fi
     fi
     echo "Error: Connectivity, gpus $i, $j, value: $v."
     ret=9; 
     return
   fi
   # All other Matrixes
   #====================
   if [[ "$v" =~ ^-?[0-9]*[.,]?[0-9]*$ ]]; then
      ## echo "$string is a float"
      # GPU_LOCAL
      # ========
      if [ $i -eq $j ]; then
         # this is local
         if [ ${v%.*} -lt ${GPU_LOCAL[$matrix]} ]; then
           echo "Error: gpus $i,$j, expecting min value: ${GPU_LOCAL[$matrix]}, got: $v"
           let err+=1
         else
           if [ $verbose ]; then echo "gpus $i,$j, expecting min value: ${GPU_LOCAL[$matrix]}, got: $v"; fi
         fi
         return
      fi

      # GPU_PPC
      # ========
      if ( [ $i -lt $limit ] && [ $j -ge $limit ] ) || ( [ $i -ge $limit ] && [ $j -lt $limit ] )  ; then
         # this is ppc
         if [ ${v%.*} -lt ${GPU_PPC[$matrix]} ]; then
           echo "Error: gpus $i,$j, expecting min value: ${GPU_PPC[$matrix]}, got: $v"
           let err+=1
         else
           if [ $verbose ]; then echo "gpus $i,$j, expecting min value: ${GPU_PPC[$matrix]}, got: $v"; fi
         fi
         return
      fi

      # GPU_GPU
      # ========
      if [ ${v%.*} -lt ${GPU_GPU[$matrix]} ]; then
         echo "Error: gpus $i,$j, expecting min value: ${GPU_GPU[$matrix]}, got: $v"
         let err+=1
         return
      else
         if [ $verbose ]; then echo "gpus $i,$j, expecting min value: ${GPU_GPU[$matrix]}, got: $v"; fi
      fi

   # not a floating point
   else   
      let err+=1
      echo "Error: gpus $i,$j, invalid value, got: $v" 
   fi
   return
}

#=====================================================
# fail
#=====================================================

function fail()
{ 
  exit_code=$1
  echo "$me test FAIL, rc=$1"
  exit $exit_code
}

#  Main
# =======

if [ ! -x $LATENCY_TEST ]; then 
   echo "$LATENCY_TEST not found or it is not executable."
   fail 1
fi

pe_running=1
if [ $START_PERSISTENCED -eq 1 ]; then
  start_nvidia_persistenced
  pe_running=$is_nvidia_persistenced_running
fi

file=/tmp/$$
$LATENCY_TEST 1>$file 2>&1
rc=$?
cat $file
lines=`wc -l $file|awk '{print $1}'`
if [ $pe_running -eq 0 ]; then stop_nvidia_persistenced; fi

echo -e "\nAnalyzing the matrix using values:"
echo "GPU_LOCAL Connectivity, Unidirectional, Bidirectional: ${GPU_LOCAL[@]}"
echo "GPU_GPU:  Connectivity, Unidirectional, Bidirectional: ${GPU_GPU[@]}"
echo -e "GPU_PPC:  Connectivity, Unidirectional, Bidirectional: ${GPU_PPC[@]}\n"


if [ $rc -ne 0 ]; then echo "$LATENCY_TEST fail, rc=$rc."; fail $rc; fi
if [ $lines -lt 50 ]; then 
   echo "$LATENCY_TEST fail, not enough lines in the output. Is nvidia-persistenced running?"
   fail 1
fi

## let's parse the output
nmatrix=${#eyecatcher[@]}   # number of matrixes to look
tel=$(($ngpus**2))          # total elements in the matrix
limit=$(($ngpus/2))         # value to determine the quatrant

for (( m=0; m<$nmatrix; m++ )); do
   n=`grep -n "${eyecatcher[$m]}" $file | cut -d':' -f1`
   if [ -z "$n" ]; then
     echo "String '${eyecatcher[$m]}' not found in output file: $file."
     fail 1
   fi
   echo -e "\nChecking ${eyecatcher[$m]}"
   let n+=2    
   readarray -t a < <(tail -n +$n $file | head -$ngpus | sed 's/^ *[^ ][^ ]*  *//')
   if [ $ngpus -ne ${#a[@]} ]; then 
      echo "Invalid output. ${a[@]} does not have $ngpus elements."
      err++
   else
     for (( i=0; i<$ngpus; i++ )); do
        aa=( ${a[$i]} )
        for (( j=0; j<$ngpus; j++ )); do
          [ "${processor}" == "p9" ] && check_p9_connectivity $i $j ${aa[$j]} $m;
          [ "${processor}" == "p8" ] && check_p8_connectivity $i $j ${aa[$j]} $m;
          if [ $ret -eq 9 ]; then fail 9; fi
        done
     done
   fi
done

if [ $err -eq 0 ]; then echo "$me test PASS, rc=0"; exit 0; fi
echo "$me test FAIL, rc=1"
exit 1             
