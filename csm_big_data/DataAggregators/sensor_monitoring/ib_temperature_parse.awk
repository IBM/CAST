#!/bin/awk -f
# encoding: utf-8
# ================================================================================
#
# ib_temperature_poll.cfg
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
# ================================================================================

# Author: John Dunham (jdunham@us.ibm.com)
# An awk file for producing an ib sensor csv line.

function init_host_sensors(hostname){
   sensors[hostname]["CPU_Core_Sensor_T1"]=-274
   sensors[hostname]["CPU_Core_Sensor_T2"]=-274
   sensors[hostname]["CPU_package_Sensor"]=-274
   sensors[hostname]["power-mon_PS1"]=-274
   sensors[hostname]["power-mon_PS2"]=-274
   sensors[hostname]["Board_AMB_temp"]=-274
   sensors[hostname]["Ports_AMB_temp"]=-274
   sensors[hostname]["SIB"]=-274
   
   hostnames[++hostname_num]=hostname
   
}

BEGIN{
   FS=" "
   ORS=""

   header_length=split(headers,header_collection, ",")
   hostname_num=0
}

# Store the temperature of the cpu core.
# This action stores the temperature in a computed array location.
# Differentiates based on Reg fieled.
/CPU Core Sensor/{
   if( sensors[$1]["SIB"] == ""){
      init_host_sensors($1)
   }
   
   sensors[$1]["CPU_Core_Sensor_"$6]=$7
}

# Store the temperature of the cpu package sensor.
/CPU package Sensor/{
   if( sensors[$1]["SIB"] == ""){
      init_host_sensors($1)
   }   
   sensors[$1]["CPU_package_Sensor"]=$7
}

# Store power monitor in the appropriate Module.
/power-mon/{
   if( sensors[$1]["SIB"] == ""){
      init_host_sensors($1)
   }
   
   sensors[$1]["power-mon_"$2]=$5
}

# Board Ambient Temperature, should only be one.
/Board AMB temp/{
   if( sensors[$1]["SIB"] == ""){
      init_host_sensors($1)
   }
   sensors[$1]["Board_AMB_temp"]=$7
}

# Port Ambient Temperature, should only be one.
/Ports AMB temp/{
   if( sensors[$1]["SIB"] == ""){
      init_host_sensors($1)
   }

   sensors[$1]["Ports_AMB_temp"]=$7
}

# ?
/SIB/{
   if( sensors[$1]["SIB"] == ""){
      init_host_sensors($1)
   }

   sensors[$1]["SIB"]=$5
}

END{
   for(host=1; host <= hostname_num; host++)
   {
     print(call_date)
     print (hostnames[host])
     for(i=1;i <= header_length;i++){
        print (sensors[hostnames[host]][header_collection[i]])

        if(i+1 <= header_length)
           print (":") 
     }
     print("\n")
   }
}

