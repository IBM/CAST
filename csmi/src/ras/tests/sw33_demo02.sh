#! /bin/bash
#================================================================================
#
#    csmi/src/ras/tests/sw33_demo02.sh
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

set -v

# This script is intended to be run on the management node

# Set the compute_node to use for the test, in this case, default to *-vm03
compute_node=`nodels /*-vm03`
echo $compute_node

PATH=$PATH:/opt/ibm/csm/bin

# Remove any old logs
rm /var/log/ibm/csm/ras/actions/custom_ras_action.log

# Start the CSM Master on the management node
systemctl start csmd-master.service

# Wait for the daemon to come up
sleep 2;

# Create a new RAS msg type
csm_ras_msg_type_create --msg_id custom.service.start \
--message 'Service started on $(location_name)' \
--severity INFO --threshold_count 1 --threshold_period 3

# Query the newly created RAS msg type
csm_ras_msg_type_query -m custom.service.start

# Create a custom ras action script to trigger when the event is detected
cat << \EOF > /opt/ibm/csm/ras/actions/custom_ras_action 
#! /bin/bash

echo 'The user defined script receives the following json string passed via $1:'
echo "$1"

echo "This script can parse the json string and use the values as inputs to additional user defined behaviors."
location_name=`echo $1 | sed -e 's/^.*"location_name"[ ]*:[ ]*"//' -e 's/".*//'`
control_action=`echo $1 | sed -e 's/^.*"control_action"[ ]*:[ ]*"//' -e 's/".*//'`

echo "Running control_action=$control_action for location_name=$location_name"
EOF

chmod +x /opt/ibm/csm/ras/actions/custom_ras_action

cat /opt/ibm/csm/ras/actions/custom_ras_action

# Set the new custom_ras_action script as the control_action for custom.service.start
csm_ras_msg_type_update -m custom.service.start -c custom_ras_action

# Confirm the change 
csm_ras_msg_type_query -m custom.service.start

# Confirm that there are no existing RAS events, starting now 
eventdate=`date "+%Y-%m-%dT%H:%M:%S %z"`;

csm_ras_event_query -b "$eventdate" 

# Create a custom.service.start event
csm_ras_event_create -m custom.service.start -l "$compute_node"

# Wait a few seconds for the event to get logged
sleep 20;

# Confirm that a custom.service.start event has been recorded
csm_ras_event_query -b "$eventdate" 

# View the custom_ras_action script log
cat /var/log/ibm/csm/ras/actions/custom_ras_action.log

# Delete the event from the RAS event action table
psql csmdb postgres -c "DELETE from csm_ras_event_action WHERE msg_id='custom.service.start';"

sleep 2;

# Delete the newly defined RAS msg type
csm_ras_msg_type_delete -m custom.service.start

csm_ras_msg_type_query -m custom.service.start

# Remove the custom action script
rm /opt/ibm/csm/ras/actions/custom_ras_action

# Stop the CSM Master on the management node
systemctl stop csmd-master.service
