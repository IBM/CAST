#!/bin/bash
###########################################################
#     fuseUmount.sh
#
#     Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

umask 0027

# exit on any script failure
#set -eo pipefail
BSCFS_MNT_PATH="/bscfs"
UNMOUNTRETRIES=12
SLEEPSEC=5
if [ "$#" -ne 0 ]; then
  BSCFS_MNT_PATH=$1
fi
grep -c $BSCFS_MNT_PATH  /proc/mounts &>>/dev/null
grepRC=$?
if [ "$grepRC" -ne 0 ] ; then
  echo $BSCFS_MNT_PATH not mounted `date`  > /tmp/fuseUnmount.debug
  exit 0
fi 

if [ "$#" -gt 1 ]; then
  UNMOUNTRETRIES=$2
fi

if [ "$#" -gt 2 ]; then
  SLEEPSEC=$3
fi

echo $0  BSCFS_MNT_PATH=$BSCFS_MNT_PATH UNMOUNTRETRIES=$UNMOUNTRETRIES SLEEPSEC=$SLEEPSEC > /tmp/fuseUnmount.debug
date >> /tmp/fuseUnmount.debug
CMDrc=29
for (( i=1; i<=$UNMOUNTRETRIES; i++ ))
do
  fusermount -u $BSCFS_MNT_PATH &>> /tmp/fuseUnmount.debug
  CMDrc=$?
  if [ CMDrc == 0 ]; then
    exit 0
  fi
  echo CMDrc=$CMDrc &>> /tmp/fuseUnmount.debug
  sleep($SLEEPSEC)
done
exit CMDrc
