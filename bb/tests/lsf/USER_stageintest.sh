#!/bin/bash
###########################################################
#     USER_stageintest.sh
#
#     Copyright IBM Corporation 2017,2018. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################
# exit on any script failure
set -eo pipefail

#Need to set GPFS directory for test
GPFSDIR=/gpfs/gpfst/jake
SOURCEFILE=$GPFSDIR/stagin_$LSB_JOBID
TARGETFILE=$BBPATH/stagin_$LSB_JOBID
TRANSFERFILE=$GPFSDIR/transferfile.txt

#RANDSIZEGB=$( echo $((2 + RANDOM % 14)) )
RANDSIZEKB=$( echo $((128 + RANDOM % 1024)) )
#dd if=/dev/urandom of=$SOURCEFILE  bs="$RANDSIZEGB"GB count=1
dd if=/dev/urandom of=$SOURCFILE  bs="$RANDSIZEKB"KB count=1 iflag=fullblock

DATACHECK=$( md5sum /gpfst/jake/staginKB_$LSF_STAGE_JOBID )

if [ -z "$LSF_STAGE_JOBID" ]
then
  LSF_STAGE_JOBID=0
fi

echo "$SOURCEFILE  $TARGETFILE" > $TRANSFERFILE

echo "Jobid $LSF_STAGE_JOBID:" $( date ) inside user stagein script $( basename $BASH_SOURCE )

REAL_LSB_HOSTS=$( echo $LSF_STAGE_HOSTS |awk '{print $2}' )
if [ -z "$REAL_LSB_HOSTS" ]
then
  REAL_LSB_HOSTS=LOCAL
fi

gethandlecmd="/opt/ibm/bb/bin/bbcmd gethandle --jobstepid=1 --target=0 --hostlist=$REAL_LSB_HOSTS --tag=1 --contrib=0" 
echo "Running command= $gethandlecmd"
gethandleout=$( $gethandlecmd )
echo "result gethandleout=$gethandleout"
echo "result LSB_HOSTS=$LSB_HOSTS"
echo "result REAL_LSB_HOSTS=$REAL_LSB_HOSTS"
echo "result DATACHECK=$DATACHECK"
handle=$( echo $gethandleout |awk -F"transferHandle" '{print $2}' |awk -F\" '{print $3}' )
echo "result handle=$handle"

if [ -z "$handle" ] 
 then
    echo "ERROR--no handle returned"
 exit -1
fi

copycmd="/opt/ibm/bb/bin/bbcmd copy --contribid=0 --handle=$handle --hostlist=$REAL_LSB_HOSTS --jobstepid=1 --target=0 --filelist=$TRANSFERFILE
echo "Running command= $copycmd"
getcopyout=$( $copycmd )
echo "result getcopyout=$getcopyout"
rc=$(  echo "$getcopyout" | tr "[\"]" "[ ]" | awk -F"rc : " '{print $2}' | awk '{print $1}' )
exit $rc
