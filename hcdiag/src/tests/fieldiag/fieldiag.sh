#!/bin/bash

#============================================================================
#   
#    hcdiag/src/tests/fieldiag/fieldiag.sh
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

# fieldiag run as root
# ======================
# NOTE 1: This is a long run test, takes 5-6 hours to complete! 
#         We recommend that you invoke this shell directly on the node
#
# NOTE 2: Reboot is required after running this test
#
# NOTE 3: Before reboot, save the output file, they are saved in /tmp/fieldiag/$$ if 
#         HCDIAG_LOGdir environemnt variable is not set
#
# NOTE 4: We assume the following default, unless you pass your own
#         For Minsky, x8 argument is required
#         For Withersppon, pex_lanes=2 argument is required
#
# NOTE 5: Please update NVIDIA_FILEDIAG with the location of nvidia fieldiag
#         Also visit the function release_gpu to make the necessaries updates
# 
#
# When completed, the diagnostic will return:
#    0: PASS - hardware passes the diagnostics
#    1: FAIL - hardware has failed the diagnostics
#    2: RETEST - hardware setup has failed the pre-check portion of the 
#       diagnostics and a warning message appears describing the problem. 
#       Correct the problem per the pre-check message and then test again
#
# It will also print PASS, FAIL, or RETEST to the screen.
#
## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   [ ! -d "$HCDIAG_LOGDIR" ] && echo "Invalid directory. Exiting" && exit 1
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   export OUTPUT_DIR=$HCDIAG_LOGDIR
   exec 2>$THIS_LOG 1>&2
fi

export thisdir=`pwd`
export NVIDIA_FIELDIAG=$thisdir
export NVIDIA_MODULES="nvidia_drm nvidia_modeset nvidia_uvm nvidia nouveau"
export FIELDIAG_MODULE="mods"
#export LSF_DIR="/shared/lsf/10.1/linux3.10-glibc2.17-ppc64le/etc"

export args=$@
source $thisdir/../common/functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "$me started on `hostname -s`, machine type $model, `date`."          

get_processor
if [ "$ret" -ne "0" ]; then echo -e "Not supported.\n$me test FAIL, rc=9"; exit 9; fi  
if [ -z "$args" ]; then
  args="pex_lanes=2"
  if [ "$processor" == "p8" ]; then args="x8"; fi
fi

sudo -E bash <<"EOF"

# stop all the processes that are using the nvidia modules
# =========================================================
function release_gpu()
{
  # stop csm service
  for s in csmd-compute csmd-utility csmd-aggregator; do
     ok=`systemctl status $s | grep running`
     if [ "$ok" != "" ]; then
        echo "Stopping $s"
        systemctl stop $s
        break
     fi
  done  

  # let's stop the daemons
  stop_dcgm
  stop_nvidia_persistenced
  sleep 1
}

source $thisdir/../common/gpu_functions

release_gpu
echo "Unloading all nvidia modules"
echo "Issuing '/usr/sbin/rmmod ${NVIDIA_MODULES} ${FIELDIAG_MODULE}'"
/usr/sbin/rmmod ${NVIDIA_MODULES} ${FIELDIAG_MODULE} || true

if [ -z "$OUTPUT_DIR" ]; then
   OUTPUT_DIR=/tmp/fieldiag/$$
   mkdir -p $OUTPUT_DIR
fi

cd $OUTPUT_DIR
echo "Installing nvidia module 'mods'"
echo "Issuing '$NVIDIA_FIELDIAG/install_module.sh -i'" 
$NVIDIA_FIELDIAG/install_module.sh -i 
rc=$?
  
if [ $rc -eq 0 ]; then
   # check if modules were installed
   ok=`/usr/sbin/lsmod |grep mods` 
   if [ -n "$ok" ]; then
     echo "Running $NVIDIA_FIELDIAG/fieldiag $args"
     echo "Be patient, might take up to 6 hours to complete"
     $NVIDIA_FIELDIAG/fieldiag $args
     rc=$?
     # remove the installed module
     echo "Uninstalling nvidia 'mods' module"
     echo "Issuing '/usr/sbin/rmmod ${FIELDIAG_MODULE}'"
     /usr/sbin/rmmod ${FIELDIAG_MODULE} || true 
     echo "Issuing '$NVIDIA_FIELDIAG/install_module.sh -u'"
     $NVIDIA_FIELDIAG/install_module.sh -u
   else
     echo "Problems installing fieldiag module, install_module.sh returned $rc"
     rc=10
   fi
fi

case $rc in
   # PASS, but check for false positive
   0) 
      #logfile=`ls $OUTPUT_DIR/fieldiag_PASS_*.log 2>/dev/null`
      #if [ -n "$logfile" ]; then
      #   # file exist, so we are good
      #   echo "fieldiag binary PASS file saved in $OUTPUT_DIR/$logfile"
      #   echo "$me test PASS, rc=0"
      #   exit 0
      #fi
      #echo "fieldiag return $rc: PASS, but could not find PASS binary file in $OUTPUT_DIR"
      #echo "Maybe retest?"
      #rc=11
      echo "fieldiag return $rc: PASS"
      echo "Machine need to be rebooted"
      echo "$me test PASS, rc=0"
      exit 0
      ;;

   # FAIL
   1) 
      #logfile=`ls $OUTPUT_DIR/fieldiag_FAIL_*.log 2>/dev/null`
      #if [ -n "$logfile" ]; then
      #   # file exist, so we are good
      #   echo "fieldiag binary FAIL file saved in $OUTPUT_DIR/$logfile"
      #else
      #   echo "fieldiag return $rc: FAIL, but could not find FAIL binary file in $OUTPUT_DIR"
      #   echo "Maybe retest?"
      #fi       
      echo "fieldiag return $rc: FAIL"
      ;;

   2) echo "fieldiag return $rc: RETEST, check message and rerun"
      ;;

   # all other rc were set by this script, so ignore
   *)  ;;
esac


echo "Machine need to be rebooted"
echo "$me test FAIL, rc=$rc"
exit $rc
EOF

