#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/gpudirect/gpudirect.sh
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

export CUDA_H_PATH=/usr/local/cuda/include/cuda.h
export CUDA_PATH=/usr/local/cuda/
export PATH=$PATH:/usr/local/cuda/bin

set -e
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $(basename $0) on $(hostname -s), machine type $model.\n"          

trap 'rm -f /tmp/$$' EXIT

# SETUP -----------------------------------------------------------------------
list="$*"
if [[ $list = *"["* ]]; then
    # Expand range syntax for bracket expanded node list
    list=${list/[/{};
    list=${list/]/\}};
    nodelist=$(eval echo $list)
else
    # Expand range syntax for comma separated node list
    nodelist=${list/,/ };
fi
nodearr=(${nodelist})

# Check for sufficient number of nodes to test
length=${#nodearr[@]}
if [ "$length" -lt "2" ]; then
    echo -e "Must provide at least 2 nodes to verify communication.\n$(basename $0) test FAIL, rc=1"
    exit 1
fi

# Run ib_write_bw --------------------------------------------------------------
server=${nodearr[0]}
for client in "${nodearr[@]:1}"
do
    echo -e "Results for $server and $client:\n" >>/tmp/$$
    # Run verbs test for mlx5_[0,1,2,3]
    for idx in 0 1 2 3
    do
        # Find available port
        listen=" "
        while [ "$listen" != "$server: 0" ]; do
            port=$(shuf -i 1025-32700 -n 1)
            listen=$(xdsh $server "ss -lH src :$port | wc -l")
        done

        xdsh $server -t 1 "/usr/bin/ib_write_bw --use_cuda -d mlx5_$idx -p $port &>/dev/null &" &>/dev/null
        xdsh $client "/usr/bin/ib_write_bw --use_cuda -d mlx5_$idx -p $port $server 2>&1" &>>/tmp/$$
    done
    echo -e "\n\n" >>/tmp/$$
done

#Show results
cat /tmp/$$

# Check for errors ------------------------------------------------------------
err=$(grep "Couldn't connect to $server" /tmp/$$ | wc -l)
if [ "$err" -eq "0" ]; then
    echo -e "\n$(basename $0) test PASS, rc=0"
    exit 0
fi

echo -e "Unable to connect to $server for ib_write_bw"
echo -e "\n$(basename $0) test FAIL, rc=1"
exit 1
