#================================================================================
#
#    csmd/src/daemon/tests/csm_infrastructure_health_check.sh
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
#!/bin/bash

echo "Testing Node Inventory: $1...";
INSTALL_PATH=$1

CONFIG="${INSTALL_PATH}/bin/csm_master.cfg"
CONFIG_AGG="${INSTALL_PATH}/bin/csm_aggregator.cfg"
CONFIG_AGENT="${INSTALL_PATH}/bin/csm_compute.cfg"
CONFIG_UTILITY="${INSTALL_PATH}/bin/csm_utility.cfg"

DAEMON="${INSTALL_PATH}/csmd/bin/csmd"
BROKER="/usr/sbin/mosquitto"
WLM="${INSTALL_PATH}/csmd/bin/csm_infrastructure_health_check"

# set up the number of csm api clients and wait time
# NOTE: There is a time out in csm_infrastructure_test. When testing a lot of
#       clients, the test handler needs to be changed accordingly.
FINAL_OUTPUT="/tmp/csmapi.output_"
NUM_CLIENTS=1
WAIT_TIME=2 # 2 seconds for one client
WAIT_TIME=$(($WAIT_TIME * $NUM_CLIENTS))

if [ ! -f $CONFIG ] || [ ! -f $CONFIG_AGG ] || [ ! -f $CONFIG_AGENT ] || [ ! -f $CONFIG_UTILITY ]
then
  echo "Configuration files missing..."
  exit -1
fi

# set up the environment variable for WLM to connect to Utility Agent
export CSM_SSOCKET="/run/csmd.sock"

pid_list=""
if [ -f $DAEMON ] && [ -f $BROKER ] && [ -f $WLM ]
then
  #Start MQTT broker
  $BROKER &
  pid_list="$pid_list $!"
  sleep 0.5

  #Then Start Master
  CMD="${DAEMON} -f ${CONFIG}"
  $CMD &
  pid_list="$pid_list $!"
  sleep 0.5

  #Start Aggregator
  CMD="${DAEMON} -r a -f ${CONFIG_AGG}"
  $CMD &
  pid_list="$pid_list $!"
  sleep 0.5

  #Start Utility
  CMD="${DAEMON} -r u -f ${CONFIG_UTILITY}"
  $CMD &
  pid_list="$pid_list $!"
  sleep 0.5

  #Start Compute Agent after Aggregator
  CMD="${DAEMON} -r c -f ${CONFIG_AGENT}"
  $CMD &
  pid_list="$pid_list $!"
  #Wait a little longer to populate the DB
  sleep 1

  #Start CSMAPI client after Utility
  for IDX in `seq 1 $NUM_CLIENTS`; do
    CMD="${WLM}"
    OUTPUT=${FINAL_OUTPUT}${IDX}
    $CMD > ${OUTPUT} &
    ## Though the client will exit, kill it anyway in case it hang
    pid_list="$pid_list $!"
  done

  sleep $WAIT_TIME
  
  # kill all spawned processes
  for pid in ${pid_list}; do
    kill -9 $pid
  done

else
  echo "daemon or broker or csmi client executable not found!"
fi

for IDX in `seq 1 $NUM_CLIENTS`; do
  OUTPUT=${FINAL_OUTPUT}${IDX}
  cat ${OUTPUT} | grep disconnected_nodes &> /dev/null
  if [ $? -ne 0 ]; then
    echo "CSM_INFRASTRUCTURE_TEST FAIL..."
    exit -1
  fi
done

echo "CSM_INFRASTRUCTURE_TEST SUCCESS..."
exit 0
