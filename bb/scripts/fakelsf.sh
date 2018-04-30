#!/bin/bash
###########################################################
#     fakelsf.sh
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

function Usage {
    echo "Usage:"
    echo "    $0"
    echo "        [ --help" ]
    echo "        [ --hostlist=<host0,...>" ]
    echo "        [ --ssd_min=<size>" ]
    echo "        [ --ssd_max=<size>" ]
    echo "        [ --time_limit=<seconds>" ]
    echo "        [ --bscfs" ]
    echo "        [ --bscfs_pfs_path=<path>" ]
    echo "        [ --bscfs_stgin_listfile=<list_file>" ]
    echo "        [ --bscfs_work_dir=<path>" ]
    echo "        [ --libfuse_path=<path>" ]
    echo "        [ --stagein_script=<script_file>" ]
    echo "        [ --stageout_phase1_script=<script_file>" ]
    echo "        [ --stageout_phase2_script=<script_file>" ]
    echo "        -- <command> <arg> ..."
}

FLOOR=$(realpath $(dirname $0)/../..)

GETOPT_OUT=$(getopt -o "" \
		-l "help, \
		    hostlist:, \
		    ssd_min:, \
		    ssd_max:, \
		    time_limit:, \
		    bscfs, \
		    bscfs_pfs_path:, \
		    bscfs_stgin_listfile:, \
		    bscfs_work_dir:, \
		    libfuse_path:, \
		    stagein_script:, \
		    stageout_phase1_script:, \
		    stageout_phase2_script:" \
		-n $0 -- "$@")

if [ $? != 0 ]; then
    Usage
    exit -1
fi

eval set -- "$GETOPT_OUT"

LSB_HOSTS=""
if [ "$SUDO_USER" != "" ]; then
    LSB_JOB_EXECUSER=$SUDO_USER
else
    LSB_JOB_EXECUSER=$USER
fi

LSB_JOBID=1
BB_SSD_MIN=""
BB_SSD_MAX=""
USER_TIME_LIMIT=""
BB_STGIN_SCRIPT=""
BB_STGOUT1_SCRIPT=""
BB_STGOUT2_SCRIPT=""
BSCFS_NEEDED=""
BSCFS_PFS_PATH=""
BSCFS_STGIN_LISTFILE=""
BSCFS_WORK_PATH=""
USER_LIBFUSE_PATH=""

while true; do
    case "$1" in
	--help) Usage; exit 0;;
	--hostlist) LSB_HOSTS=$2; shift 2;;
	--ssd_min) BB_SSD_MIN=$2; shift 2;;
	--ssd_max) BB_SSD_MAX=$2; shift 2;;
	--time_limit) USER_TIME_LIMIT=$2; shift 2;;
	--bscfs) BSCFS_NEEDED=1; shift 1;;
	--bscfs_pfs_path) BSCFS_PFS_PATH=$2; shift 2;;
	--bscfs_stgin_listfile) BSCFS_STGIN_LISTFILE=$2; shift 2;;
	--bscfs_work_dir) BSCFS_WORK_PATH=$2; shift 2;;
	--libfuse_path) USER_LIBFUSE_PATH=$2; shift 2;;
	--stagein_script) BB_STGIN_SCRIPT=$2; shift 2;;
	--stageout_phase1_script) BB_STGOUT1_SCRIPT=$2; shift 2;;
	--stageout_phase2_script) BB_STGOUT2_SCRIPT=$2; shift 2;;
	--) shift; break;;
	*) echo "$0: unexpected output from getopt"; exit -1;;
    esac
done

USER_COMMAND="$@"

if [ "$LSB_HOSTS" == "" ]; then LSB_HOSTS="localhost localhost"; fi

if [ "$BB_SSD_MIN" == "" ]; then BB_SSD_MIN=16G; fi
if [ "$BB_SSD_MAX" == "" ]; then BB_SSD_MAX=$BB_SSD_MIN; fi

if [ "$BSCFS_PFS_PATH" == "" ]; then
    BSCFS_PFS_PATH=$(eval echo ~$LSB_JOB_EXECUSER)
fi

if [ "$BSCFS_NEEDED" != "" -a "$BSCFS_WORK_PATH" == "" ]; then
    BSCFS_WORK_PATH="$BSCFS_PFS_PATH/.bscfs"
fi


BBPATH="/tmp/bblv_${LSB_JOB_EXECUSER}_${LSB_JOBID}"

if [ "$BSCFS_NEEDED" != "" ]; then
    BSCFS_MNT_PATH="$BBPATH/bscfs"
fi

export FLOOR;               echo "FLOOR:               " $FLOOR

export BBPATH;              echo "BBPATH:              " $BBPATH
export BB_SSD_MIN;          echo "BB_SSD_MIN:          " $BB_SSD_MIN
export BB_SSD_MAX;          echo "BB_SSD_MAX:          " $BB_SSD_MAX
export BB_STGIN_SCRIPT;     echo "BB_STGIN_SCRIPT:     " $BB_STGIN_SCRIPT
export BB_STGOUT1_SCRIPT;   echo "BB_STGOUT1_SCRIPT:   " $BB_STGOUT1_SCRIPT
export BB_STGOUT2_SCRIPT;   echo "BB_STGOUT2_SCRIPT:   " $BB_STGOUT2_SCRIPT

export LSB_JOB_EXECUSER;    echo "LSB_JOB_EXECUSER:    " $LSB_JOB_EXECUSER
export LSB_JOBID;           echo "LSB_JOBID:           " $LSB_JOBID
export LSB_HOSTS;           echo "LSB_HOSTS:           " $LSB_HOSTS

export USER_TIME_LIMIT;     echo "USER_TIME_LIMIT:     " $USER_TIME_LIMIT
export BSCFS_MNT_PATH;      echo "BSCFS_MNT_PATH:      " $BSCFS_MNT_PATH
export BSCFS_PFS_PATH;      echo "BSCFS_PFS_PATH:      " $BSCFS_PFS_PATH
export BSCFS_STGIN_LISTFILE;echo "BSCFS_STGIN_LISTFILE:" $BSCFS_STGIN_LISTFILE
export BSCFS_WORK_PATH;     echo "BSCFS_WORK_PATH:     " $BSCFS_WORK_PATH
export USER_LIBFUSE_PATH;   echo "USER_LIBFUSE_PATH:   " $USER_LIBFUSE_PATH
export USER_COMMAND;        echo "USER_COMMAND:        " $USER_COMMAND


export BBTOOLS_QUIET=1

#$FLOOR/bb/scripts/stagein_admin.pl
#su $LSB_JOB_EXECUSER -c $FLOOR/bb/scripts/launch_job.sh
#$FLOOR/bb/scripts/stageout_admin.pl

sudo -E $FLOOR/bb/scripts/stagein_admin.pl 1
$FLOOR/bb/scripts/stagein_admin.pl 2
sudo -E $FLOOR/bb/scripts/stagein_admin.pl 3

$FLOOR/bb/scripts/launch_job.sh 

$FLOOR/bb/scripts/stageout_admin.pl 1
sudo -E $FLOOR/bb/scripts/stageout_admin.pl 2
$FLOOR/bb/scripts/stageout_admin.pl 3
sudo -E $FLOOR/bb/scripts/stageout_admin.pl 4
