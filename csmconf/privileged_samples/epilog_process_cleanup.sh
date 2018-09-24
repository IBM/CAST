#!/bin/bash
#================================================================================
#
#    epilog_process_cleanup.sh
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================
# ===================================================================================
# The following script attempts to kill any processes associated with the
#   user in the CSM "CSM_USER_NAME" evironment variable. The script will attempt
#   to kill the processes "KILL_ATTEMPTS" times, executing 'killall' with no
#   special signal followed by a SIGSEV. A sleep of "SLEEP_TIMER" seconds will
#   be injected between each invocation of 'killall' 
#
# In the event of a failure "FAILURE_ERROR_CODE" will be returned.
#
# If using the provided CSM Epilog sample there are several ways to execute the code.
#   1. Using the os module: 
#       import os
#       ...
#       os.system(<script-name>)
#
#   2. Using the subprocess module:
#       import subprocess
#       ...
#       process = subprocess.Popen(['<script-name>'], stdout=subprocess.PIPE)
#       out, err = process.communicate()
# ===================================================================================

# Constants
# ================
KILL_ATTEMPTS=10 # The number of attempts to be made to kill the user's processes.
SLEEP_TIMER=10   # The amount of time (in seconds) to sleep between kill attempts.

LOG_LEVEL="local1.info" # The logger priority level for logger calls.
LOG_TAG="ZOMBIEUSER"    # The logger tag.

# A debug output file, this is optional.
DEBUG_FILE="/tmp/epilog-remaining-pids-${CSM_USER_NAME}-${CSM_PRIMARY_JOB_ID}.log"
#DEBUG_FILE="/dev/null"  # Set to disable error log.

FAILURE_ERROR_CODE=1    # The error code for the failure condition.
# ================

# Helper functions
# ================
# ================

# Verify that "CSM_USER_NAME" was exported before continuing.
if [[ ${CSM_USER_NAME:=missing} == "missing" ]]
then
    log_string="$0: Job ID: ${CSM_PRIMARY_JOB_ID}; CSM_USER_NAME not specified; FAILING epilog execution;"
    echo "${log_string}"      >> ${DEBUG_FILE}
    logger -p ${LOG_LEVEL} -t ${LOG_TAG} ${log_string}

    exit ${FAILURE_ERROR_CODE}
fi

# Iterate SLEEP_TIMER times to 
for attempt in $(seq 1 ${KILL_ATTEMPTS})
do 
    remaining_procs=$(ps h -u ${CSM_USER_NAME} | wc -l)

    # If the number of remaining processes is greater than zero,
    #   attempt to kill all the processes (2 attempts per loop). 
    # Else exit the for loop.
    if [[ ${remaining_procs} -gt 0 ]] 
    then
        # Log atempt
        log_string="$0: Job ID: ${CSM_PRIMARY_JOB_ID}; ${remaining_procs} processes remain for user ${CSM_USER_NAME} after ${attempt} iterations;"
        echo "${log_string}"      >> ${DEBUG_FILE}
        ps hl -u ${CSM_USER_NAME} >> ${DEBUG_FILE}
        logger -p ${LOG_LEVEL} -t ${LOG_TAG} ${log_string}

        # Attempt a normal kill, sleep, run a SIGSEV kill, the sleep again.
        killall -u ${CSM_USER_NAME}
        sleep ${SLEEP_TIMER}
        killall -s 9 -u ${CSM_USER_NAME}
        sleep ${SLEEP_TIMER}
    else
        break
    fi
    break
done

# If the script failed to kill any processes log it, then exit with a failure condition.
if [[ ${remaining_procs} -gt 0 ]]
then
    log_string="$0: Job ID: ${CSM_PRIMARY_JOB_ID}; ${remaining_procs} processes remain for user $CSM_USER_NAME} after ${remaining_procs} iterations; FAILING epilog execution;"
    ps hl -u ${CSM_USER_NAME} >> ${DEBUG_FILE}
    echo "${log_string}"      >> ${DEBUG_FILE}
    logger -p ${LOG_LEVEL} -t ${LOG_TAG} ${log_string}
    
    exit 1
fi

logger -p ${LOG_LEVEL} -t ${LOG_TAG} "$0: Job ID: ${CSM_PRIMARY_JOB_ID}; All processes cleaned up."

