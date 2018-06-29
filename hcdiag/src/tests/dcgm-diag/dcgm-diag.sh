#!/bin/bash      

#================================================================================
#   
#    hcdiag/src/tests/dcgm-diag/dcgm-diag.sh
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


# todo: if the console log for all tests changes to json, we need to change the parsing
#
export DCGM_DAEMON=/usr/bin/nv-hostengine
export DCGM_CMD=/usr/bin/dcgmi

RUN_LEVELS=(1 2 3 4 5)
run_level=3
if [[ $# -gt 0 ]]; then run_level=$1; fi
valid=$(echo ${RUN_LEVELS[@]} | grep -o $run_level | wc -w)

me=$(basename $0) 
thishost=`hostname -s`

if [ $valid -eq 0 ]; then echo "Invalid run_level $run_level, valid values are: ${RUN_LEVELS[@]} "  ; exit -1; fi 
if [ "$run_level" -eq  "4" ] || [ "$run_level" -eq  "5" ]; then
  sdir=/tmp                                             
  ddir=/tmp
  if [[ $# -gt 1 ]]; then sdir=$2; fi
  if [[ $# -gt 2 ]]; then ddir=$3; fi

  if [ "$run_level" -eq  "4" ]; then
     # Single Precision
     sdir="${sdir}/${thishost}_s"
     ddir="${ddir}/${thishost}_s"
     args="-r 3 -j --statspath ${sdir} --debugLogFile ${ddir}/dcgm_log -v -d 5" 
  else
     #Double Precision
     sdir="${sdir}/${thishost}_d"
     ddir="${ddir}/${thishost}_d"
     args="-r diagnostic --parameters Diagnostic.use_doubles=True --statspath ${sdir} --debugLogFile ${ddir}/dcgm.log -v -d 5"
  fi
  mkdir -p $sdir
  if [ $? -ne 0 ]; then "ERROR: can't not create $sdir directory."; echo "$me test FAIL, rc=1"; exit 1; fi  
  mkdir -p $ddir
  if [ $? -ne 0 ]; then "ERROR: can't not create $ddir directory."; echo "$me test FAIL, rc=1"; exit 1; fi  

else
  args="-r $run_level"
fi


thisdir=`dirname $0`
source $thisdir/../common/functions
source $thisdir/../common/gpu_functions

supported_machine
echo "Running $me on $thishost, machine type $model."          

if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ -z $is_boston ]; then 
   echo -e "Could not determine if the machine has GPUs by model. Continuing.."
   is_boston=False
fi 
if [ $is_boston == True ]; then echo -e "Model does not have GPUs.\n$me test PASS, rc=0"; exit 0; fi 

# check if machine has GPUs
# ===========================
has_gpus
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
if [ "$ngpus" -eq "0" ]; then echo "$me test FAIL, rc=1"; exit 1; fi 


trap 'rm -f /tmp/$$' EXIT

FAIL="Fail"


if [ ! -x $DCGM_DAEMON ]; then echo "$DCGM_DAEMON executable not found or invalid permission"  ; exit -1; fi
if [ ! -x $DCGM_CMD ]; then echo "$DCGM_CMD executable not found or invalid permission"; exit -1; fi


# start the daemons
# =================
start_nvidia_persistenced
pe_running=$is_nvidia_persistenced_running
start_dcgm
dcgm_running=$is_dcgm_running


$DCGM_CMD --version

read_gpu_basics 0

# start the diagnostic 
# ====================
echo -e "\nIssuing command $DCGM_CMD discovery -l" 
$DCGM_CMD discovery -l 
         
echo -e "\nIssuing command $DCGM_CMD diag ${args}"
$DCGM_CMD diag ${args} | tee /tmp/$$
rc=$?


# stop the daemons 
# =================
if [ $pe_running -eq 0 ]; then stop_nvidia_persistenced; fi
if [ $dcgm_running -eq 0 ]; then stop_dcgm; fi

# analize the diag results
# =========================
drc=0
if [ "$rc" -ne "0" ]; then
   echo "Error running $DGM_CMD diag command,  rc= $rc."
else
   #if egrep -i -q 'Killed|Error' /tmp/$$; then
   if egrep -i -q 'Killed|^Error' /tmp/$$; then
      drc=255
   fi
   if grep -q $FAIL /tmp/$$; then
      if [ "$drc" -eq "0" ]; then 
         drc=99 
      fi                           
   fi
fi


#  Exit code
#  =========
if [ "$drc" -eq "0" ]; then
   if [ "$rc" -eq "0" ]; then
      echo -e "\n$me test PASS, rc=0"  
   else
      echo -e "\n$me test FAIL, rc=$rc"  
   fi
else
   rc=$drc
   echo -e "\n$me test FAIL, rc=$rc"  
fi
  
exit $rc

