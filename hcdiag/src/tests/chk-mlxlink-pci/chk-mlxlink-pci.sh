#!/bin/sh

#================================================================================
#   
#    hcdiag/src/tests/chk-mlxlink-pci/chk-mlxlink-pci.sh
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


export readonly me=${0##*/}
export thisdir=`dirname $0`
export LSPCI=/usr/sbin/lspci
export MLXLINK=/usr/bin/mlxlink

THRESHOLD=100
if [ $# -gt 0 ]; then THRESHOLD=$1; fi
export THRESHOLD

sudo -E bash <<"EOF"

trap 'rm -f /tmp/$$' EXIT
source $thisdir/../common/functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on `hostname -s`, machine type $model."          


# Link Speed Active (Enabled)     : 16G-Gen 4 (8G-Gen 3)
# Link Width Active (Enabled)     : 8X (8X)
# RX Errors                       : 0
# TX Errors                       : 20
# CRC Error dllp                  : 0
# CRC Error tlp                   : 

err=0
for i in $(${LSPCI} |  grep Mell | awk '{print $1}'); do 
  if [ "${i: -1}" == "0" ]; then
     echo -e "\n=== $i ==="
     ${MLXLINK} -d $i -c --port_type=PCIE --show_eye | grep -E "Link|Error" | tee /tmp/$$

     # Link Speed Active (Enabled)     : 16G-Gen 4 (8G-Gen 3)
     # Link Width Active (Enabled)     : 8X (8X)
     # RX Errors                       : 0
     # TX Errors                       : 20
     # CRC Error dllp                  : 0
     # CRC Error tlp                   : 
     while IFS= read -r line; do
        if [[ ${line} == Link* ]]; then
           if [[ ${line} == *(Enabled)* ]]; then
              continue;
           else
              echo "ERROR: Expected Enabled."
              let err+=1
           fi
        elif [[ ${line} = *"Error"* ]]; then
           v=`echo ${line} | awk '{print $NF}'` 
           if [ "$v" -gt "${THRESHOLD}" ]; then
              echo "ERROR: Value $v, exceed the threshold $THRESHOLD."
              let err+=1
           fi
        else
           echo "ERROR: $line, not recognized."
           let err+=1
        fi
     done < /tmp/$$
   fi
done
echo -e "\nFound $err error(s).\n"

if [ "$err" -eq "0" ]; then
   echo "$me test PASS, rc=0"
else
   echo "$me test FAIL, rc=$err"
fi

exit $err
EOF
     
