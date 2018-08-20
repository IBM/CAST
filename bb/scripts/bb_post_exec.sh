#!/bin/bash
###########################################################
#     bb_post_exec.sh
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

FLOOR=/opt/ibm
export FLOOR
umask 0027

export PATH_PRESERVE=$PATH

echo "Job $LSF_STAGE_JOBID: $( date ) stage-out process script $( basename $BASH_SOURCE )" &>> /var/log/bb_stageout.log

su $LSF_STAGE_USER -p -c "$FLOOR/bb/scripts/stageout_admin.pl 1" | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stageout.log
$FLOOR/bb/scripts/stageout_admin.pl 2 | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stageout.log
su $LSF_STAGE_USER -p -c "$FLOOR/bb/scripts/stageout_admin.pl 3" | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stageout.log
$FLOOR/bb/scripts/stageout_admin.pl 4 | perl -ne 'printf("Job %4d: $_", $ENV{"LSF_STAGE_JOBID"});' &>> /var/log/bb_stageout.log
