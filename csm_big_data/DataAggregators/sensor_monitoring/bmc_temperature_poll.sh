#!/bin/bash
# encoding: utf-8
# ================================================================================
#
# bmc_temperature_poll.sh
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
# Date: 3/24/16
# Polls the BMCs defined in the xcat database to determine sensor data for.

# 
# $1 IP 
# $2 Username
# $3 Password
# $4 pid_instance 
query_server(){
  # Verify that the bmc is responsive.
  ping -c 1 $bmc -w 1  > /dev/null
  if [[ $? -gt 0 ]]
  then
    >&2 echo "Could not reach $bmc."
    echo $bmc >> $unresponsive_bmcs
    continue;
  fi

  echo "$bmc is responsive, continuing."

  # Get the hostname, bmc ip is not useful.
  bmc_host=$(/opt/xcat/bin/lsdef -w bmc=$bmc | cut -d " " -f1)

  # TODO verify that this is the way we want the timezone to be recorded.
  dsv_line=$(date --iso-8601=s):${bmc_host}:${bmc}:
  dsv_line="${dsv_line}$(/opt/xcat/bin/ipmitool-xcat -H $bmc -U $bmcusername -P $bmcpassword sdr type Temperature | sort | awk -v headers=$csv_headers -f bmc_temperature_parse.awk )"

  # Write the data separated values to the logstash server.
  echo $dsv_line >> /dev/tcp/$logstash_server/$logstash_port
}

# Defines the file for unresponsive BMCs to be dumped into.
unresponsive_bmcs=/tmp/unresponsive_bmc

# Load in the configuration
source bmc_temperature_poll.cfg

>$unresponsive_bmcs
> sensor.dsv
echo "Polling BMCs in cluster"


if [ -e $hosts_file ]
then
    cp $hosts_file /tmp/sensor_hostnames$$
else

    /opt/xcat/bin/lsdef -w bmc!=" " > /tmp/sensor_hostnames$$
fi

#######################################
# TODO Document
declare -A active_processes

# Available indices for tracking.
stack_head=0
declare -A index_stack


# Populate a stack of available indicies in our active processes array.
# Basically represents a pool.
while [ $stack_head -lt $max_parallel_threads ]
do
  index_stack[$stack_head]=$(( max_parallel_threads-stack_head-1 ))
  active_processes[$stack_head]=-1 
  (( stack_head++ ))
done

# Solves an off by one error.
(( stack_head-- ))
#######################################

# Iterate over the hostnames from the sensor data.
while read -r -u3 hostname node
do  

  # If the active process count exceeds the maximum parallel thread, wait.
  ######################################
  while [ $stack_head -lt 0 ]
  do
      # Iterate over the active processes ( queue should be full if it reaches here )
      for (( i=0; i < $max_parallel_threads; i++ ))
      do
           ps -p ${active_processes[$i]} >/dev/null
	   
           # Verify the pid was found.
           if [[ $? -gt 0 ]]
           then
               # Increment the head of the stack and update the stack and process "queue"
               (( stack_head ++ ))
               active_processes[i]=-1
               index_stack[stack_head]=$i
           fi 
      done
      
      if [[ $index_stack -lt 0 ]]
      then
          #TODO Tweak this value
          sleep .2
      fi 
  done
  ######################################

  # Set up the variables.
  bmc=""
  bmcpassword=${default_passwd}
  bmcusername=${default_user}

  # Not using eval, because arbitrary string execution makes me nervous.
  while IFS="=" read -r key value
  do
      # Strip out the blanks with parameter expansion.
      case ${key//[[:blank:]]/} in
         bmc)
            bmc=$value;;
         bmcpassword)
            if [[ -z ${bmcpassword} ]]
            then
                bmcpassword=$value
            fi;;
         bmcusername)
            if [[ -z ${bmcusername} ]]
            then
                bmcusername=$value
            fi;;
         *)
            echo "failed $key"
      esac
  done < <(/opt/xcat/bin/lsdef -i bmc,bmcpassword,bmcusername $hostname | sed "1d")

  query_server $bmc $bmcpassword $bmcusername &


  # Add the pid to the script.
  ######################################
  pid_index=${index_stack[$stack_head]}
  active_processes[$pid_index]=$!
  index_stack[$stack_head]=-1 

  ((stack_head--))
  ######################################

done 3< "/tmp/sensor_hostnames$$"

wait

echo "Sensor poll completed"


# Clear out the sensor hostnames
rm /tmp/sensor_hostnames$$
# Old versions of the script feeding the while read loop.
#< <(lsdef -w mtm==8335-GTA -w bmc!=" ")
#(lsdef all | sed -n "s/^[ ]*bmc=\([0-9]*\.[0-9]*\.[0-9]*\.[0-9]*\)$/\1/p")

