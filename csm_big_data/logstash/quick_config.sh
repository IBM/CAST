#!/bin/bash
# encoding: utf-8
# ================================================================================
#
# quick_config.sh
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

#================================================================================
# CHANGE THIS VARIABLE TO SUIT YOUR ENVIRONMENT, will try to use the defualt URL.
#================================================================================
# This is the IP Address of the L
scala_ip_new="CHANGE_THIS"
#==============================================

#==========================================================
# CHANGE THESE IF YOUR ENVIRONMENT DIFFERS FROM THE DEFAULT
#==========================================================
# This the the port that the Unity webapp is running on
scala_port="9987"

# The root of the logstash install.
#Logstash_Home="/opt/Logstash/logstash*"
Logstash_Home="/opt/Logstash/$(ls /opt/Logstash/ | grep logstash-* | head -1)";

# The Default scala user name and password.
scala_user_new="unityadmin"
scala_password_new="unityadmin"
#=========================================================

# Allow the user to specifiy the ip address.
while getopts "i:" opt
do
    case $opt in
     i)
        scala_ip_new=$OPTARG;
        ;;
    esac
done


#============================================================
# Directory Values
#============================================================
# Compute where this script is located.
Script_Home=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# The source directories for the automated install script
pattern_source="patterns"
config_source="config"
grok_dynamic_source="plugins/grok_dynamic/"
#============================================================

# Prompt the user for a separate Logstash Home
if [[ ! -d ${Logstash_Home} ]]
then
    echo "\""${Logstash_Home}"\" not found, is Logstash installed?"
    exit 1
fi

#============================================================
# Computed Values
#============================================================
Logstash_Dest=${Logstash_Home}/logstash-scala/logstash/

if [ ${scala_ip_new} != "CHANGE_THIS" ]
then
    scala_url="https://${scala_ip_new}:${scala_port}/Unity/DataCollector"
    echo "scala_url computed to be ${scala_url}"

else
    sed -n 's:.*\(scala_[up][^ ]*\)[ ]*=>[ ]*\(.*\):\1=\2:gp' ${Logstash_Dest}/config/*\
        > /tmp/scala_scratch
    source /tmp/scala_scratch
    rm -f /tmp/scala_scratch

    echo "IP Address was not set, used existing value for scala_url: ${scala_url}"
fi
#============================================================

# grok_dynamic Plugin
# Install ruby on the node, because this assumes that ruby hasn't been installed.
yum install ruby

# Build then deploy gem file.
cd  ${grok_dynamic_source}
gem_file=$(./build.sh | awk '/File:/{print $2}')
${Logstash_Home}/bin/plugin install ${gem_file}
cd ${Script_Home}

# Copy the files:
cp -r ${pattern_source} ${Logstash_Home}/logstash-scala/logstash/
cp -r ${config_source} ${Logstash_Home}/logstash-scala/logstash/

# Replace the scala_url and user info
sed -i -e "s|\(.*scala_url[^ ]*[ ]*=>[ ]*\).*|\1\"${scala_url}\"|g" \
    -e "s:\(.*scala_user[^ ]*[ ]*=>[ ]*\).*:\1\"${scala_user_new}\":g" \
    -e "s:\(.*scala_password[^ ]*[ ]*=>[ ]*\).*:\1\"${scala_password_new}\":g" \
    ${Logstash_Dest}/config/*

# Restart Logstash
/opt/utilities/logstash-util.sh restart

