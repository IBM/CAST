#!/bin/sh

#================================================================================
#   
#    hcdiag/src/tests/chk-mlnx-pci/chk-mlnx-pci.sh
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


export LSPCI=/usr/sbin/lspci
export SETPCI=/usr/sbin/setpci
export readonly me=${0##*/}
export thisdir=`dirname $0`

sudo -E bash <<"EOF"

source $thisdir/../common/functions

supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on `hostname -s`, machine type $model."          

if [ ! -x "$LSPCI" ] || [ ! -x "$SETPCI" ]; then echo -e "Could not find the $LSPCI and/or $SETPCI executables.\n$me test FAIL, rc=1"; exit 1; fi
    
FIRMWARE_LOG=/sys/firmware/opal/msglog
if [ ! -r "$FIRMWARE_LOG" ]; then echo -e "$FIRMWARE_LOG not found or invalid permission.\n$me test FAIL, rc=1"; exit 1; fi

# bit 0-3 is Gen check.  100 is Gen4  0011 is GEN3 0010 is GEN2
# bit 4-9 is width chech.00100 is X4, 01000 is x8. 10000 is x16

g4x16="100000100"
 g4x8="010000100"
 g4x4="001000100"
g3x16="100000011"
 g3x8="010000011"
 g3x4="001000011"
#gen2=0010

gen=`egrep 'GEN2|GEN3|GEN4' $FIRMWARE_LOG | head -1 | awk '{print substr($NF,1,4)}'`

if [ -z "$gen" ]; then echo -e "Coud not determine PCIe version.\n$me test FAIL, rc=1."; exit 1; fi
echo "System in $gen mode"

if [ "$gen" != "GEN4" ]; then echo -e "$gen not supported by this script.\n$me test FAIL, rc=1."; exit 1; fi

### it is GEN4

server="$(cat $FIRMWARE_LOG | grep -c Witherspoon)"

rc=0
count=0
for i in `$LSPCI -d 15b3: -s.0 | cut -d " " -f1`;
 do
        count=$((count+1))
        # Check link status register at offset 12h
        # Change hex values are upper case
        lnkstat="$($SETPCI -s $i CAP_EXP+0x12.w |tr "a-z" "A-Z")"
        capstat="$($SETPCI -s $i CAP_EXP+0x0c.w |tr "a-z" "A-Z")"
        device="$($LSPCI -vvvs $i |grep "Product Name")"
        #echo " $i - $lnkstat - $capstat

        #convert hex to bin and extract last nine bits
        #lnkstatbin="$(echo "obase=2; ibase=16; $lnkstat" | bc |tail -10c)"
        #capstatbin="$(echo "obase=2; ibase=16; $capstat" | bc |tail -10c)"
        lnkstatbin="$(echo "obase=2; ibase=16; $lnkstat" | bc |awk '{ len = (16 - length % 16) % 16; printf "%.*s%s\n", len, "00000000", $0}' |tail -10c)"
        capstatbin="$(echo "obase=2; ibase=16; $capstat" | bc |awk '{ len = (16 - length % 16) % 16; printf "%.*s%s\n", len, "00000000", $0}' |tail -10c)"

        #echo $i - $lnkstat - $lnkstatbin - $capstat - $capstatbin

        if [ "$server" -ne "0" ] &&  [[ $i == "0034:01:00.0"  ]] && [[ $lnkstatbin == $g4x4 ]] && [[ $capstatbin == $g4x8 ]]; then
                echo "Passes pci link check"
                echo "WSP x8 slot operating in x4 mode"
                echo "$i - $device"
                echo "Link status- $lnkstatbin"
                echo "Cap status - $capstatbin"
                echo "-----------------------------"

        elif [[  $lnkstatbin == $capstatbin ]]; then
                echo "Passes pci link check"
                echo "$i - $device"
                echo "Link status- $lnkstatbin"
                echo "Cap status - $capstatbin"
                echo "-----------------------------"

        else [[ $lnkstatbin != $capstatbin ]]
                echo "Device at pci id $i is misconfigured"
                echo "Fail code  : Lnkstat - $lnkstatbin; Capstat -  $capstatbin"
                echo "Failed device: $device"
                echo "-----------------------------"
                rc=2
        fi
done

if [ "$count" -eq "0" ]; then
   echo "No Mellanox pci adapter found."
   echo "$me test FAIL, rc=3"
else
   echo "Found $count Mellanox pci port(s)."
   if [ "$rc" -eq "0" ]; then
      echo "$me test PASS, rc=0"
   else
      echo "$me test FAIL, rc=$rc"
   fi
fi

exit $rc

EOF
