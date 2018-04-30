#!/bin/bash
#================================================================================
#
#    csmrestd/tests/rundaemons.sh
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
#================================================================================

# startup the csmrestd and csmd daemons....

thisdir=`dirname $0`
thisscript=`basename $0`

# use this in the environment when running the command line stuff as well,
# this matches what is in the local coral.cfg file...
# override the socket we will use
export CSM_SSOCKET=/run/csmd.sock


MOSQUITTO_PATH=/usr/sbin/mosquitto
MOSQUITTO_PORT=51883
CSMD_PATH=$thisdir/../../work/csmd/bin/csmd
CSMRESTD_PATH=$thisdir/../../work/csmrestd/bin/csmrestd

CSMRESTD_HOST=127.0.0.1
CSMRESTD_PORT=5555

function killpid {
    #set -x
    PID=$1;
    if [[ ! -z $PID  &&  -f /proc/$PID/exe  ]] ; then
    	kill $PID 
    fi
}

# trap to make sure we kill off all running programs...
CSMD_PID=
CSMRESTD_PID=
function finish {
    killpid $CSMRESTD_PID 
    killpid $CSMD_PID 
    killpid $MOSQUITTO_PID 
} 
trap finish EXIT

# todo, play with the config files for each daemon....



#gdb $CSMD_PATH --args master $thisdir/coral.cfg
#exit 0

# fire up misquitto daemon...
$MOSQUITTO_PATH -p $MOSQUITTO_PORT &
MOSQUITTO_PID=$!
echo MOSQUITTO_PID=$MOSQUITTO_PID


$CSMD_PATH master $thisdir/coral.cfg &
CSMD_PID=$!
echo CSMD_PID=$CSMD_PID

# give the server daemon a little bit of time to open up the connection...
sleep 1
$CSMRESTD_PATH 0.0.0.0 5555 &
CSMRESTD_PID=$!
echo CSMRESTD_PID=$CSMRESTD_PID

# this is basically a wait untl we ctlc the this script and that will toast the daemons...
wait













