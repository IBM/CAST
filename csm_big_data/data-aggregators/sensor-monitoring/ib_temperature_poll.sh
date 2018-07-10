#!/bin/bash
# encoding: utf-8
# ================================================================================
#
# ip_temperature_poll.sh
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
# ================================================================================

# Author: John Dunham (jdunham@us.ibm.com)

# The logstash server IP or hostname and port for mellanox sensor data.
logstash_server=CHANGE THIS!
logstash_port=10517

# The user name for mellanox ssh switches and the xcat group.
xcat_switch_user="admin"
xcat_groups="mswitch"

# Headers for the query.
csv_headers="CPU_Core_Sensor_T1,CPU_Core_Sensor_T2,CPU_package_Sensor,power-mon_PS1,power-mon_PS2,Board_AMB_temp,Ports_AMB_temp,SIB"

# Build the tcp ports.
tcp_port="/dev/tcp/$logstash_server/$logstash_port"

/opt/xcat/bin/xdsh $xcat_groups -l $xcat_switch_user --devicetype IBSwitch::Mellanox 'enable;show temperature' |\
awk -v headers=${csv_headers}  -v call_date=$(date --iso-8601=s) -f $(dirname $0)/ib_temperature_parse.awk > ${tcp_port}

