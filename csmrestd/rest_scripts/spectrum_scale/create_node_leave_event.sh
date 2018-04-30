#! /bin/bash
#================================================================================
#
#    csmrestd/rest_scripts/spectrum_scale/create_node_leave_event.sh
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

# The following parameters must be set appropriately for the cluster
CSMRESTD_IP="__CSMRESTD_IP__"
CSMRESTD_PORT="4213"
LOCATION_NAME="compute001"


# No changes needed below this comment
MSG_ID="spectrumscale.node.nodeLeave"

# Helper function to create the json string
# Example string:
# {"msg_id": "spectrumscale.node.nodeLeave", "location_name": "compute001"}
#
generate_json_string()
{
  cat << EOF
{
  "msg_id": "$MSG_ID",
  "location_name": "$LOCATION_NAME"
}
EOF
}

# POST the event to csmrestd
curl -X POST -d "$(generate_json_string)" http://$CSMRESTD_IP:$CSMRESTD_PORT/csmi/V1.0/ras/event/create
