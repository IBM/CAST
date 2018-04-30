#! /bin/bash
#================================================================================
#
#    csmd/src/ras/bmc/convert_ibmpowerhwmon_policy_to_csm_ras_type.sh
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

# Remove * characters from the input stream
# sed 's/[*]//g'
# Only grab the 5 fields we care about
# Remove the keys, keep the values
# Replace any internal commas with spaces, leaving the final comma at the end of the line
# rev | sed 's/,/ /2g' | rev
# Combine the 5 fields into a single line
# Remove the duplicate lines
# cat /opt/ibm/ras/bin/plugins/ibm_csm/CSMpolicyTable.json | sed 's/[*]//g' | egrep "\"CommonEventID\":|\"EventType\":|\"LengthyDescription\":|\"Message\":|\"Severity\":" | awk -F"\": " '{print $2}' | rev | sed 's/,/ /2g' | rev | paste -d' ' - - - - - | tr -s ' ' | sort -u -t, -k2,2 -k1,1 

# print the header to match the csm_ras_type_data.csv format 
echo "#msg_id,severity,message,description,control_action,threshold_count,threshold_period,enabled,set_state,visible_to_users"

while IFS=, read -r CommonEventId EventType LengthyDescription Message Severity
do
  # Remove spaces
  CommonEventId="${CommonEventId// }"
  EventType="${EventType// }"
  Severity="${Severity// }"
  
  # Remove quotes
  CommonEventId="${CommonEventId//\"}"
  EventType="${EventType//\"}"
  Severity="${Severity//\"}"

  # Remove initial space and quote
  Message="${Message// \"}"
  LengthyDescription="${LengthyDescription// \"}"

  # Remove trailing quote  
  Message="${Message//\"}"
  LengthyDescription="${LengthyDescription//\"}"
  
  # null value checking
  if [ "$CommonEventID" == "null" ] ; then  
    CommonEventID=""
  fi
  
  if [ "$EventType" == "null" ] ; then  
    EventType=""
  fi
  
  if [ "$LengthyDescription" == "null" ] ; then  
    LengthyDescription=""
  fi
  
  if [ "$Message" == "null" ] ; then  
    Message=""
  fi
  
  if [ "$Severity" == "null" ] ; then  
    Severity=""
  fi

  # Generate msg_id from EventType and CommonEventId
  MsgId="bmc.$EventType.$CommonEventId"
  
  # Map ibmpowerhwmon Severity to CSM RAS Severity
  # ibmpowerhwmon support Information, Warning, Critical
  # CSM RAS supports INFO, WARNING, FATAL
  if [ "$Severity" == "Information" ] ; then
    Severity="INFO"
  elif [ "$Severity" == "Critical" ] ; then
    Severity="FATAL"
  else
    Severity="WARNING"
  fi

  #Message=
  #Description
  
  # Default all of these for now
  ControlAction="NONE"
  ThresholdCount="1"
  ThresholdPeriod="0"
  Enabled="true"
  SetState=""
  VisibleToUsers="true"

  #msg_id,severity,message,description,control_action,threshold_count,threshold_period,enabled,set_state,visible_to_users
  echo "$MsgId,$Severity,$Message,$LengthyDescription,$ControlAction,$ThresholdCount,$ThresholdPeriod,$Enabled,$SetState,$VisibleToUsers"
  
done < <( cat /opt/ibm/ras/bin/plugins/ibm_csm/CSMpolicyTable.json | sed 's/[*]//g' | egrep "\"CommonEventID\":|\"EventType\":|\"LengthyDescription\":|\"Message\":|\"Severity\":" | awk -F"\": " '{print $2}' | rev | sed 's/,/ /2g' | rev | paste -d' ' - - - - - | tr -s ' ' | sort -u -t, -k2,2 -k1,1 ) | sort

