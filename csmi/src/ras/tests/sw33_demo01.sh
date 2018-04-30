#! /bin/bash
#================================================================================
#
#    csmi/src/ras/tests/sw33_demo01.sh
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

# This script is intended to be run from the management node

# Set the compute_node to use for the test, in this case, default to *-vm03
compute_node=`nodels /*-vm03`
echo $compute_node

PATH=$PATH:/opt/ibm/csm/bin

# Start the CSM Master and CSM Aggregator on the management node
systemctl start csmd-master.service
systemctl start csmd-aggregator.service

# Start the compute agent on vm03
xdsh $compute_node 'systemctl start csmd-compute.service'

# Wait for the daemons to come up
sleep 2;

# Set the compute node to ready='y'
csm_node_attributes_update -n "$compute_node" -r y
csm_node_attributes_query -n "$compute_node" | egrep "node_name|ready|state"

eventdate=`date "+%Y-%m-%dT%H:%M:%S %z"`;

# Confirm that there are no RAS events recorded yet
csm_ras_event_query -b "$eventdate" 

# Stop the compute agent on vm03
xdsh $compute_node 'pkill csmd'

# Wait a few seconds for the event to get logged
sleep 20;

# Confirm that a csm.status.down event has been recorded
csm_ras_event_query -b "$eventdate" 

# Confirm that the node has been set to ready='n'
csm_node_attributes_query -n "$compute_node" | egrep "node_name|ready|state"

# Stop the CSM Aggregator and CSM Master on the management node
systemctl stop csmd-aggregator.service
systemctl stop csmd-master.service
