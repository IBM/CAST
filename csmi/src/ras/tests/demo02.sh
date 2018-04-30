#!/bin/bash 
#================================================================================
#
#    csmi/src/ras/tests/demo02.sh
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

# 
# demonstration of csmi ras api functions...

# use this in the environment when running the command line stuff as well,
# this matches what is in the local coral.cfg file...
# override the socket we will use
export CSM_SSOCKET=/run/csmd.sock


bindir=../../../../work/csmi/tests/bin

#csm_ras_msg_type_create <msg_id> \
#                   [-t|--min_time_in_pool <min_time_in_pool] \
#                   [-p|--suppressids <suppress_ids>] \
#                   [-s|--severity <severity> ] [-m|--message <message> ] [-d <decoder>] \
#                   [-c|--control_aciton <control_action> ] \
#                   [-n|--threshold_count <threshold_count> ] \
#                   [-p|--threshold_period <threshold_period ] \
#                   [-r--relevant_diags <relevant_diags> ]
#              msg_id -- message id of record to create.
#              -t|--min_time_in_pool min_time_in_pool -- minimum time in ras pool (in seconds)
#                    to allow for ras event suppression
#              -p|--suppressids <suppress_ids> comma separated
#                   list of ras id's to suppress with this event
#              -s|--severity <severity> INFO | WARNING | FATAL
#              -m|--message <message> message to display for this ras
#                   (required parameter)
#              -d|--decoder <decoder> name of decoder to use
#                   (currently unused)
#              -c|--control_aciton <control_action> name of control
#                   action script to invoke for this event
#              -n|--threshold_count <threshold_count> threshold count,
#                   number of times this event has to happen
#                   during the threshold period before being considered
#                   a ras event
#              -o|--threshold_period <threshold period> time interval
#                    to consider threshold events in
#                       for example, 1H -- 1 hour, 30M -- 30 minutes. 30S = 30 seconds
#                       valid suffex are D|H|M|S
#              -r|--relevant_diags <relevant_diagnostics>
#                    -- recommended diagsnotics to run after seeing this event
#

# make sure we don't have test id around from the last time we did this...
set -x
$bindir/csm_ras_msg_type_delete test.msg.id || true

$bindir/csm_ras_msg_type_create test.msg.id \
   -t 0 \
   -s WARNING \
   -m "this is a test message for test.msg.id" \
   -c "none" 

$bindir/csm_ras_msg_type_get test.msg.id

$bindir/csm_ras_msg_type_update test.msg.id \
   -s INFO \

$bindir/csm_ras_msg_type_get test.msg.id

$bindir/csm_ras_msg_type_delete test.msg.id 

$bindir/csm_ras_msg_type_get test.msg.id








