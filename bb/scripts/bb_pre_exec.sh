#!/bin/bash
###########################################################
#     bb_pre_exec.sh
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

#FLOOR=$(realpath $(dirname $0)/../..)
FLOOR=/opt/ibm/
export FLOOR
umask 0027

# exit on any script failure
set -eo pipefail

echo "Job $LSF_STAGE_JOBID: $( date ) stage-in process script $( basename $BASH_SOURCE )" &>> /var/log/bb_stagein.log

LSF_STAGE_STORAGE_MINSIZE="${LSF_STAGE_STORAGE_MINSIZE}G" #append unit
export LSF_STAGE_STORAGE_MINSIZE
export PATH_PRESERVE=$PATH

$FLOOR/bb/scripts/stagein_admin.pl 1 | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stagein.log
su $LSF_STAGE_USER -p -c "$FLOOR/bb/scripts/stagein_admin.pl 2" | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stagein.log
$FLOOR/bb/scripts/stagein_admin.pl 3 | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stagein.log

if [ "$BSCFS_MNT_PATH" != "" ]; then
    $FLOOR/bb/scripts/BSCFS_start | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stagein.log
fi
