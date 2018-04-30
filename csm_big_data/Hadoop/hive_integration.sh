#!/bin/bash
# encoding: utf-8
# ================================================================================
#
#    hive_integration.sh
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

#=========================================================
# Return Codes
#=========================================================
#: The directories and mounts were properly built.
RET_COMPLETE=0

#: Exit code for input events.
RET_INPUT_EXIT=1

#: Exit code for A input state that specifies an ip address for la server, but no la dir.
RET_INPUT_BAD_DIR=2
#=========================================================


usage()
{
    cat << EOF >&2
    Usage $0
    =========
    -h              Displays this message.
    -u <user name>  The User Name of the log analysis user [default: ioala].
    -n <name node>  The Name Node server ip or hostname (Required).

    -l <la server>  The Log Analysis Server ip or hostname (Required).
                     If an IP address is supplied an HDFS Log Analysis 
                       root directory must be specified [-d].

    -d <la dir>     The HDFS Log Analysis root directory.
                     If not specified the string /ioala-root-\${la_server} is used.
                     See "Habanero Install Guide: Log Analysis Integration: Section 4"
                        for what this value should be.
    Error Codes
    ===========
    0 - Script executed with no problems.
    1 - Execution terminated unexpectedly.
    2 - Log Analysis Server was an ip address, but the root dir was undefined.

    WARNING 1: This script assumes a previously unconfigured version of Hadoop.
    WARNING 2: This is an automation of "Habanero Install Guide".
EOF
}

# Echo to stderr.
echoe()
{
    >&2 echo $@
}

commands="hu:n:l:d:"

#: User name of the LA user.
user_name="ioala"

#: The Log Analysis Server.
la_server="LOG_ANALYSIS_SERVER_NOT_SET"
opt_ioala_home="/opt/IBM/ioala/"
utilities="${opt_ioala_home}LogAnalysis/utilities/"
web_inf_dir="${opt_ioala_home}LogAnalysis/wlp/usr/servers/Unity/apps/Unity.war/WEB-INF/"
la_lib_dir="${web_inf_dir}lib/hive-jars/"
properties="${web_inf_dir}unitysetup.properties";



#: The name node server
nn_server="NAME_NODE_NOT_SET"
hive_jars="/usr/iop/4.1.0.0/hive/lib/*.jar"
hive_port=10000
ioala_hdfs_root_dir=""

while getopts ${commands} opt
do
    case ${opt} in
        h) 
            usage; exit ${RET_INPUT_EXIT};;
        n)
            nn_server=${OPTARG}
            ;;
        l)
            la_server=${OPTARG}
            ;;
        u)
            user_name=${OPTARG}
            ;;
        d)
            ioala_hdfs_root_dir=${OPTARG}
            ;;
        *)
            echoe "Invaild option: ${opt}"
            usage
            exit ${RET_INPUT_EXIT}
            ;;
    esac
done

if [ -z ${ioala_hdfs_root_dir} ]
then
    if [[ ${la_server} =~ ^[0-9]*\.[0-9]*\.[0-9]*\.[0-9]*$ ]]
    then
        echoe "ERROR: Detected an ip address for the Log Analysis Server, but the hdfs root dir was not set."
        echoe "ERROR: Either supply a Log Analysis Directory [-d], or set the Log Analysis Server to a hostname [-l]."
        exit ${RET_INPUT_BAD_DIR}
    fi

    ioala_hdfs_root_dir="/ioala-root-${la_server}"
fi

#: Print the user name and server settings.
echoe "==========================================="
echoe "User Name           : ${user_name} "
echoe "Log Analysis Server : ${la_server} "
echoe "Name Node Server    : ${nn_server} "
echoe "==========================================="
echoe

#: Check the variables.
ping -c 1 ${la_server}
if [[ $? -ne 0 ]]
then 
    echo "Log Analysis server inacessible: ${la_server}"
    exit ${RET_INPUT_EXIT}
fi 

ping -c 1 ${nn_server} 
if [[ $? -ne 0 ]]
then 
    echo "Name Node server inacessible: ${nn_server}"
    exit ${RET_INPUT_EXIT}
fi 

if [[ $(whoami) != ${user_name} ]]
then
    echo "$(whoami) is not user ${user_name}"
    exit ${RET_INPUT_EXIT}
fi


# Stop unity.
cd ${utilities};
./unity.sh -stop;

# Make jar dir.
mkdir -p ${la_lib_dir};

# Copy over the jars.
scp ${nn_server}:${hive_jars} ${la_lib_dir};

# Build the site-xml
cat << EOF > ${la_lib_dir}hive-site.xml
<configuration>
    <property>
        <name>hive.server2.thrift.bind.host</name>
        <value>${nn_server}</value>
    </property>

    <property>
        <name>hive.server2.thrift.port</name>
        <value>${hive_port}</value>
    </property>
</configuration>
EOF

# Set the properties file.
properties="${web_inf_dir}unitysetup.properties";
cp ${properties} ${properties}.bak
cat << EOF >> ${properties}

HADOOP_TIER_ENABLE_HIVE=true
HADOOP_TIER_HIVE_SCHEMA_NAME=unity
HADOOP_TIER_HIVE_USER_NAME=hive
HADOOP_TIER_HIVE_USER_PASSWORD=hive

EOF
sed -i "s:AUDIT_ACTIONS = LOG,INDEX:AUDIT_ACTIONS = LOG:g" ${properties};

# Execute on the name node.
ssh root@${nn_server} "\su - hive -c 'hive -e \"create schema unity;\"';\
    su - hdfs -c 'hadoop fs -chmod -R 777 ${ioala_hdfs_root_dir}';"

echoe ${properties}

./unity.sh -start;


