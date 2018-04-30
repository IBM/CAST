#================================================================================
#
#    csmd/src/daemon/tests/csm_node_inventory_master_first.sh
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

DAEMON="${INSTALL_PATH}/csmd/bin/csmd"
BROKER="/usr/sbin/mosquitto"

if [ ! -f $CONFIG ] || [ ! -f $CONFIG_AGG ] || [ ! -f $CONFIG_AGENT ]
then
  echo "Configuration files missing..."
  exit -1
fi

pid_list=""
if [ -f $DAEMON ] && [ -f $BROKER ]
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

  #Start Aggregator after Master
  CMD="${DAEMON} -r a -f ${CONFIG_AGG}"
  $CMD &
  pid_list="$pid_list $!"
  sleep 0.5

  #Start Compute Agent after Aggregator
  CMD="${DAEMON} -r c -f ${CONFIG_AGENT}"
  $CMD &
  pid_list="$pid_list $!"

  sleep 2
  
  # kill all spawned processes
  for pid in ${pid_list}; do
    kill -9 $pid
  done

else
  echo "daemon or broker executable not found!"
fi
