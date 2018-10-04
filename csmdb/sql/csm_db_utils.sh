#!/bin/bash
#================================================================================
#   
#    csm_db_utils.sh
# 
#    © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

function echoe () {
    >&2 echo $@
}

function start_timer () {
    SECONDS=0
}

function end_timer () {
    duration=${SECONDS}
    seconds=$(printf "%02d" $(( $duration % 60 )))
    log_time=$(date '+%Y-%m-%d.%H:%M:%S')


    echoe "${log_time} ($$) ($(whoami)) [trace ] TIMING ($@) | $(( $duration / 60 )):${seconds}"
}

