#!/bin/awk -f
# encoding: utf-8
# ================================================================================
#
# bmc_temperature_parse.awk
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

# An awk file for producing a sensor csv line.

# Processes the minimum and maximum value of a sensor.
function process_min_max(sensor, temp_string){
   split(temp_string, temp, " ")

   if(sensors[sensor]["min"]+0 > temp[1]+0)
      sensors[sensor]["min"]= temp[1]+0

   if(sensors[sensor]["max"]+0 < temp[1]+0)
      sensors[sensor]["max"]= temp[1]+0
   
}


BEGIN{
   FS="|"
   ORS=""

   header_length=split(headers,header_collection, ",")
  
   # Build the sensor array. 
   # Note this is not terribly dynamic, but the assumption is that the sensor data will be consistent.
   sensors["Ambient"]=-274 

   sensors["CPU"]["min"]=5000
   sensors["CPU"]["max"]=-274

   sensors["CPU_Core"]["min"]=5000
   sensors["CPU_Core"]["max"]=-274
   
   sensors["DIMM"]["min"]=5000
   sensors["DIMM"]["max"]=-274
   
   sensors["GPU"]["min"]=5000
   sensors["GPU"]["max"]=-274
   
   sensors["Mem_Buff"]["min"]=5000
   sensors["Mem_Buff"]["max"]=-274

}

/ns/{
   next
}
/Ambient Temp/{
   split($5, temp, " ")
   sensors["Ambient"]=temp[1]
}
/CPU[0-9]+ Temp/{
   process_min_max("CPU",$5)  
}
/CPU Core Temp [0-9]+/{
   process_min_max("CPU_Core",$5)
}
/DIMM[0-9]+ Temp/{
   process_min_max("DIMM",$5)
}
/GPU Temp [0-9]+/{
   process_min_max("GPU",$5)
}
/Mem Buf Temp [0-9]+/{
   process_min_max("Mem_Buff",$5)
}


END{
   for(i=1;i <= header_length;i++){
      sensor=header_collection[i]
      if( sensor == "Ambient" )
      {
         if( sensors[sensor] != -274 )
         {
            print (sensors[sensor])
         }
      }
      else{
         if( sensors[sensor]["min"] != 5000 )
         {
	    print (sensors[sensor]["min"]":")
         }
         else
         {
            print (":")
         }

         if( sensors[sensor]["max"] != -274 )
         {
            print (sensors[sensor]["max"])
         }
      }

      if( i+1 <= header_length )
         print (":") 
   }
}

