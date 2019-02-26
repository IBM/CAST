#!/bin/bash
####################################################
#    nvmfConnect.sh
#
#    Copyright IBM Corporation 2017,2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
###################################################

network=$1
nameSpace=$2
ipAddr=$3
port=$4

# exit on any script failure
set -e

cmd="modprobe nvme-rdma"
echo "Executing: $cmd"
$cmd

cmd="/usr/sbin/nvme connect -t $network -n $nameSpace -a $ipAddr -s $port --hostnqn"
echo "Executing: $cmd <redacted>"
$cmd $NVMEKEY

exit
