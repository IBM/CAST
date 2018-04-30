#!/bin/bash
###########################################################
#     launch_job.sh
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

echo "*** Entering user launch_job script ***"

if [ "$BSCFS_MNT_PATH" != "" ]; then
    export BSCFS_BB_PATH=$BBPATH/bscfs_bb
    CLEANUP_LIST="$BSCFS_WORK_PATH/$LSB_JOBID/cleanup_list"
    PRE_INSTALL_LIST="$BSCFS_WORK_PATH/$LSB_JOBID/pre_install_list"
    PRE_INSTALL_OPTION=""
    if [ -r $PRE_INSTALL_LIST ]; then
	PRE_INSTALL_OPTION="--pre_install_list $PRE_INSTALL_LIST"
    fi

    if [ "$BSCFS_WRITE_BUFFER_SIZE" == "" ]; then
	BSCFS_WRITE_BUFFER_SIZE=4194304
    fi
    if [ "$BSCFS_READ_BUFFER_SIZE" == "" ]; then
	BSCFS_READ_BUFFER_SIZE=16777216
    fi
    if [ "$BSCFS_DATA_FALLOC_SIZE" == "" ]; then
	BSCFS_DATA_FALLOC_SIZE=0
    fi
    if [ "$BSCFS_MAX_INDEX_SIZE" == "" ]; then
	BSCFS_MAX_INDEX_SIZE=4294967296
    fi

    LIBFUSE_ENV=""
    if [ "$USER_LIBFUSE_PATH" != "" ]; then
	LIBFUSE_ENV="LD_LIBRARY_PATH=$USER_LIBFUSE_PATH"
    fi
    NODE_COUNT=$(echo ${LSB_HOSTS#* } | wc -w)
    NODE=0
    for HOST in ${LSB_HOSTS#* }; do
	ssh $HOST " \
	    mkdir $BSCFS_MNT_PATH;
	    $LIBFUSE_ENV \
		$FLOOR/bscfs/agent/bscfsAgent $BSCFS_MNT_PATH \
		    --node_count $NODE_COUNT --node_number $NODE \
		    --config $FLOOR/bb/scripts/bb.cfg \
		    --pfs_path $BSCFS_PFS_PATH \
		    --bb_path $BSCFS_BB_PATH \
		    --write_buffer_size $BSCFS_WRITE_BUFFER_SIZE \
		    --read_buffer_size $BSCFS_READ_BUFFER_SIZE \
		    --data_falloc_size $BSCFS_DATA_FALLOC_SIZE \
		    --max_index_size $BSCFS_MAX_INDEX_SIZE \
		    --cleanup_list $CLEANUP_LIST.$NODE \
		    $PRE_INSTALL_OPTION \
	"
	((NODE = NODE + 1))
    done
fi

if [ "$USER_COMMAND" != "" ]; then
    echo "Launching: $USER_COMMAND"
    if [ "$USER_TIME_LIMIT" == "" ]; then
	eval "$USER_COMMAND"
    else
	eval "$USER_COMMAND" &
	CMD_PID=$!
	(
	    sleep $USER_TIME_LIMIT
	    echo "Time limit expired. Terminating application."
	    pkill --parent $CMD_PID
	) &
	SLEEP_PID=$!
	wait $CMD_PID
	kill $SLEEP_PID > /dev/null 2>&1
	wait $SLEEP_PID > /dev/null 2>&1
    fi
fi

if [ "$BSCFS_MNT_PATH" != "" ]; then
    for HOST in ${LSB_HOSTS#* }; do
	ssh $HOST fusermount -u $BSCFS_MNT_PATH
    done
    rm -f $PRE_INSTALL_LIST
fi

echo "*** Exiting user launch_job script ***"
