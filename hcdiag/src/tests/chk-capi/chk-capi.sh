#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/chk-capi-enable/chk-capi-enable.sh
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

verbose=false
VERSION=0.1

usage()
{
cat << EOF

        Description: Checks if CAPI is enabled for a specified Mellanox ConnectX-5 adapter
        
        Usage: `basename $0` [options]
        
        Required arguments: 

              -d|--device <pci id from lspci>
              ex. 0003:01:00.0

        Optional arguments:

              [-l|--list]        List Mellanox adapters in system
              [-a|--all]         Run on all Mellanox devices in system
              [-h|--help]        This help screen
              [-v|--verbose]     Additional output for debug
              [-V|--version]     Display script version
                
EOF
}

list()
{
   echo
   lspci -nnd 15b3:
   echo
   
}

gather()
{
   devices=$(lspci | grep nox | cut -f1 -d" " | xargs)
}

me=$(basename $0) 
thisdir=`dirname $0`
thishost=`hostname -s`
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)

echo "Running $me on $thishost, machine type $model."          

# Collect input arguments
while [ ! -z "$1" ]
do
   case "$1" in
      -d | --device)
          devices="$2" 
          shift 2
         ;;
      -l | --list)
         list
         shift
         exit 0
         ;;
      -a | --all)
         gather
         shift
         ;;
      -h | --help)
         usage
         shift
         exit 0
         ;;
      -v | --verbose)
         verbose=true
         shift
         ;;
      -V | --version)
         echo $me version $VERSION
         shift
         exit 0
         ;;
      *)
      usage
         shift
         exit 1
         ;;
   esac
done


if [[ "x" == "x$devices" ]]; then
      usage
      echo "ERROR: No device specified/found. Aborting."
      echo "$me test FAIL, rc=1"
      exit 1
fi

sudo -v
rc=$?
if [ $rc -ne 0 ]; then echo -e "$me test FAIL, rc=$rc"; exit $rc; fi  

for device in $devices; do
   $verbose && echo "Verbose mode enabled, displaying adapter NVRAM config:"
   $verbose && mlxconfig -d $device -e q
   error=0

   if (( $(mlxconfig -d $device q ADVANCED_PCI_SETTINGS | grep "ADVANCED_PCI_SETTINGS" | grep False | wc -l) != "0" )); then
      echo "ERROR: ADVANCED_PCI_SETTINGS NOT ENABLED for $device on $thishost."
      error=1
   fi
   mlxconfig -d $device q IBM_CAPI_EN &> /dev/null
   if (( $? != "0" )); then
      echo "ERROR: IBM_CAPI_EN NOT ENABLED for $device on $thishost."
      error=1
   fi
   if (( $(mlxconfig -d $device q IBM_CAPI_EN | grep "IBM_CAPI_EN" | grep False | wc -l) != "0")); then
      echo "ERROR: IBM_CAPI_EN NOT ENABLED for $device on $thishost."
      error=1
   fi
   if (( $(mlxconfig -d $device q IBM_TUNNELED_ATOMIC_EN | grep "IBM_TUNNELED_ATOMIC_EN" | grep False | wc -l) != "0")); then
      echo "ERROR: IBM_TUNNELED_ATOMIC_EN NOT ENABLED for $device on $thishost."
      error=1
   fi
   if (( $(mlxconfig -d $device q IBM_AS_NOTIFY_EN | grep "IBM_AS_NOTIFY_EN" | grep False | wc -l) != "0")); then
      echo "ERROR: IBM_AS_NOTIFY_EN NOT ENABLED for $device on $thishost."
      error=1
   fi 
   if [[ $error == 0 ]]; then
      echo "SUCCESS: CAPI enabled for device $device on $thishost." 
   else 
      rc=1
   fi
done

if [[ $rc != 0 ]]; then echo -e "$me test FAIL, rc=$rc"; exit $rc; fi  

echo "$me test PASS, rc=0"  
exit 0
