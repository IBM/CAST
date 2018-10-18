#! /bin/bash
#================================================================================
#
#    csmd/src/ras/ufm/convert_ufm_policy_to_csm_ras_types.sh 
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

# print the header to match the csm_ras_type_data.csv format 
echo "#msg_id,severity,message,description,control_action,threshold_count,threshold_period,enabled,set_state,visible_to_users"

while IFS=, read -r AlarmId
do

  # Generate msg_id from AlarmId 
  MsgId="ufm.$AlarmId"
  
  # Default all of these for now
  Severity="INFO"
  Message=""
  Description=""
  ControlAction="NONE"
  ThresholdCount="1"
  ThresholdPeriod="0"
  Enabled="true"
  SetState=""
  VisibleToUsers="true"

  #msg_id,severity,message,description,control_action,threshold_count,threshold_period,enabled,set_state,visible_to_users
  echo "$MsgId,$Severity,$Message,$Description,$ControlAction,$ThresholdCount,$ThresholdPeriod,$Enabled,$SetState,$VisibleToUsers"
  
done < <( cat /opt/ufm/scripts/policy.csv | awk -F, '{print $1}' ) | sort

