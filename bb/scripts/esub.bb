#!/bin/sh
###########################################################
#     esub.bb
#
#     Copyright IBM Corporation 2017,2017. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

JOBHASH=$(env | md5sum -t | cut -d ' ' -f 1)
echo BBPATH=/mnt/bb_$JOBHASH >> $LSB_SUB_MODIFY_ENVFILE
echo BBHASH=$JOBHASH >> $LSB_SUB_MODIFY_ENVFILE

if [[ $LSB_SUB_ADDITIONAL == *"bscfs"* ]]; then
    echo BSCFS_MNT_PATH=/bscfs >> $LSB_SUB_MODIFY_ENVFILE
    echo LSB_SUB_PRE_EXEC=\"/opt/ibm/bb/scripts/BSCFS_start\" >> $LSB_SUB_MODIFY_FILE
    echo LSB_SUB3_POST_EXEC=\"/opt/ibm/bb/scripts/BSCFS_stop\" >> $LSB_SUB_MODIFY_FILE
fi
