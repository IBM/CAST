#! /bin/bash
#================================================================================
#
#    csmi/src/ras/tests/sw40_demo_set_not_ready.sh 
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

set -v

# This script is intended to be run on the management node

# Set the compute_node to use for the test, in this case, default to *-vm03
compute_node=`nodels /*-vm03`
echo $compute_node

msg_id="custom.trigger.set_not_ready"
echo $msg_id

PATH=$PATH:/opt/ibm/csm/bin

# Start the CSM Master and CSM Aggregator on the management node
systemctl start csmd-master.service
sleep 2

systemctl start csmd-aggregator.service
sleep 2

# Start the compute agent
xdsh $compute_node 'systemctl start csmd-compute.service'
sleep 2

# Set the compute node to ready='y'
csm_node_attributes_update -n "$compute_node" -r y
csm_node_attributes_query -n "$compute_node" | egrep "node_name|ready|state"

# Create a new RAS msg type
csm_ras_msg_type_create --msg_id "$msg_id" \
--message 'Setting $(location_name) to ready=n' \
--severity FATAL --threshold_count 1 --threshold_period 1 \
--set_not_ready 't'

# Query the newly created RAS msg type
csm_ras_msg_type_query -m "$msg_id" 

# Confirm that there are no existing RAS events, starting now 
eventdate=`date "+%Y-%m-%dT%H:%M:%S %z"`;
echo $eventdate

csm_ras_event_query -b "$eventdate" 

# Create an event
csm_ras_event_create -m "$msg_id" -l "$compute_node"
sleep 1

# Confirm that an event has been recorded
csm_ras_event_query -b "$eventdate" 

# Confirm that the compute node has been set to ready='n'
csm_node_attributes_query -n "$compute_node" | egrep "node_name|ready|state"

# Delete the newly defined RAS msg type
csm_ras_msg_type_delete -m "$msg_id" 

csm_ras_msg_type_query -m "$msg_id"

# Stop the compute agent
xdsh $compute_node 'systemctl start csmd-compute.service'

# Stop the CSM Aggregator and CSM Master on the management node
systemctl stop csmd-aggregator.service
systemctl stop csmd-master.service
