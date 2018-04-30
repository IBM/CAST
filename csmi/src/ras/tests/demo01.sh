#!/bin/bash 
#================================================================================
#
#    csmi/src/ras/tests/demo01.sh
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
#export CSM_SSOCKET=/run/csmd.sock

PATH=$PATH:../../../../work/csmi/tests/bin:/opt/ibm/csm/bin
#bindir=../../../../work/csmi/tests/bin

#
# usage: ./csm_ras_event_create <msg_id> [-t|--timesampp time_stamp ] [-l|--location location_name ] \
#      [-r|--rawdata raw_data ] [-k|--kvcsv kvcsv ]
#              msg_id -- message id of record to create.
#              -t time_stamp -- time stamp to use if not specified, then timestamp is now
#              -l location_name (location name/host name for this event
#              -r raw_data raw data for evetn
#              -k kvcsv -- key/value csv string, for variable substutition in ras meesage
#

eventdate=`date "+%Y-%m-%dT%H:%M:%S %z"`;

set -x
csm_ras_event_create test.testcat01.test01 \
  -t "$eventdate" \
  -l mycnode01 \
  -r "test ras message" \
  -k "k1=v1,k2=v2"

# Wait a few seconds for the event to get logged
sleep 5;

csm_ras_event_query -b "$eventdate" -e "$eventdate"
