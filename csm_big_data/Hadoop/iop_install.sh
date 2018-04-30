#!/bin/bash
# encoding: utf-8
# ================================================================================
#
#    hadoop_prereqs.sh
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

#: Exit code for a bad tar directory.
RET_TAR_DIR_ERROR=2

#: Exit code for a postgresql error.
RET_POSTGRES_ERROR=3

#: Exit code for being unable to start ambari-server.
RET_SERVER_ERROR=4

#: Exit code for being unable to start ambari-agent.
RET_AGENT_ERROR=5

#: Running `ambari-server setup` failed.
RET_AMBARI_SETUP_ERROR=6
#=========================================================

commands="hd:j:p:o:a:f"

tar_directory="iop"
jdk_dir="/usr/lib/jvm/jre-1.8.0-openjdk"
#: Hive settings, only password is configurable from commandline.
hive_password="passw0rd"
hive_user="hive"
hive_db="hive"
#: Oozie settings, only password is configurable from commandline.
oozie_password="passw0rd"
oozie_user="oozie"
oozie_db="oozie"

ambari_server=$(hostname)

force=0

usage()
{
    cat << EOF >&2
    Usage $0

    This script assumes that iop, iop-utils and ambari tars are all in one directory.
    =========
    -h               Displays this message
    -f               Forces the execution accepts the default options.
    -d {directory}   A directory containing the iop/ambari tars (iop is default)
    -j {jdk}         The location of the jdk directory (/usr/lib/jvm/jre-1.8.0-openjdk is default)
    -p {Hive Pass}   The password of the Hive User (passw0rd is default)
    -o {Oozie Pass}  The password of the Oozie User (passw0rd is default)
    -a {Ambari Server} The hostname of the ambari server ($(hostname) is default)

    Error Codes
    ===========
    0 - Script executed with no problems.
    1 - Execution terminated in the execution of getopts.
    2 - The directory containing the tarballs could not be accessed.
    3 - The script was not able to start postgresql
    4 - Start of ambari-server failed.
    5 - Start of ambari-agent failed.
    6 - Running "ambari-server setup" failed
EOF
}

display_header()
{
    echo "============="
    echo $1
    echo "============="
}
# Echo to stderr.
echoe()
{
    >&2 echo $@
}

while getopts ${commands} opt
do
    case ${opt} in
        h) 
            usage; exit ${RET_INPUT_EXIT};;

        d)
            tar_directory=${OPTARG}
            ;;
        j)
            jdk_dir=${OPTARG}
            ;;
        o)
            oozie_password=${OPTARG}
            ;;
        p)
            hive_password=${OPTARG}
            ;;
        a)
            ambari_server=${OPTARG}
            ;;
        f)
            force=1
            ;;
        *)
            echoe "Invaild option: ${opt}"
            usage
            exit ${RET_INPUT_EXIT}
            ;;
    esac
done


#: Verify the environment is ready.
display_header "Environment Check"
if [[ $(yum list installed |  grep -ic "postgresql-server") -eq 0  ]]
then
    echo "postgresql-server was not installed, was hadoop_prereqs.sh run?"
    exit ${RET_POSTGRES_ERROR}
fi


service postgresql status
if [[ $? -ne 0 ]]
then
    echoe "postgresql was not running, initalizing db and starting."

    su - postgres -c 'initdb;'
    service postgresql restart
    if [[ $? -ne 0 ]]
    then
        echoe "Couldn't start postgresql."
        exit ${RET_POSTGRES_ERROR}
    fi
fi

echo "Environment Ready!"

#: Unpack the tarballs
display_header "Unpacking iop tarballs"
cd ${tar_directory} &>/dev/null

if [[ $? -ne 0 ]]
then
    echo "${tar_directory} is a bad directory, exiting $0"
    exit ${RET_TAR_DIR_ERROR}
fi

for tarball in $(ls -1  | grep "tar" )
do
    echo "Unpacking ${tarball}"
    #tar -xvf ${tarball} &>/dev/null
done

#: Create the ambari repo.
display_header "Creating Ambari Repo"
createrepo .

cat << EOF > /etc/yum.repos.d/ambari.repo
[ambari]
name=IBM Open Platform with Apache Hadoop
baseurl=file://$(pwd)
enabled=1
gpgcheck=0
EOF

yum clean all

#: Install ambari.
display_header "Installing Ambari"
yum install -y ambari-*

#: Setup ambari
display_header "Setting up Ambari with JDK: ${jdk_dir}"
if [[ ${force} -eq 0 ]]
then
    ambari-server setup -j ${jdk_dir}
else
    ambari-server setup -j ${jdk_dir} -s
fi

if [[ $? -ne 0 ]]
then
    exit ${RET_AMBARI_SETUP_ERROR}
fi

#: Setup postgresql
display_header "Postgresql Configuration"
jdbc_jar="/usr/share/java/postgresql-jdbc.jar"
hba_conf="/var/lib/pgsql/data/pg_hba.conf"

chmod 644 ${jdbc_jar}
if [[ ${force} -eq 0 ]]
then
    ambari-server setup --jdbc-db=postgres --jdbc-driver=${jdbc_jar}
else
    ambari-server setup --jdbc-db=postgres --jdbc-driver=${jdbc_jar} -s
fi

if [[ $? -ne 0 ]]
then
    exit ${RET_AMBARI_SETUP_ERROR}
fi

echo "Ambari Postgresql Setup has been run" 
# Create Hive postgres database, users and password.
psql_create_query="CREATE DATABASE ${hive_db};CREATE USER ${hive_user} WITH PASSWORD \
    '${hive_password}';GRANT ALL PRIVILEGES ON DATABASE ${hive_db} TO ${hive_user};"
su postgres -c 'cd ~; echo $* | psql;' -- -- ${psql_create_query}

# Create Oozie postgres database, users and password.
psql_create_query="CREATE DATABASE ${oozie_db};CREATE USER ${oozie_user} WITH PASSWORD \
    '${oozie_password}';GRANT ALL PRIVILEGES ON DATABASE ${oozie_db} TO ${oozie_user};"
su postgres -c 'cd ~; echo $* | psql;' -- -- ${psql_create_query}

echo "Creation executed"

# Set up the hba config file.
echo "host oozie oozie $(hostname) md5" >> ${hba_conf} 
echo "host hive hive $(hostname) md5" >> ${hba_conf}
echo "HBA file populated"

#: Set the threads in the ambari properties.
thread_size=160
ambari_props="/etc/ambari-server/conf/ambari.properties"
display_header "Set ambari threadpool size to ${thread_size} in ${ambari_props}" 

sed -i -e "s/\(agent.threadpool.size.max=\).*/\1${thread_size}/g"\
    -e "s/\(server.execution.scheduler.maxThreads=\).*/\1${thread_size}/g"\
    -e "s/\(client.threadpool.size.max=\).*/\1${thread_size}/g" ${ambari_props}

#: Set the hostname field in ambari-agent ini file.
display_header "Set the hostname in the ambari-agent ini file"
ambari_ini="/etc/ambari-agent/conf/ambari-agent.ini"
sed -i "s/\(hostname=\).*/\1${ambari_server}/g" ${ambari_ini}


#: Start Ambari Server and Ambari Agent.
display_header "Starting ambari-server and ambari-agent"
ambari-server start
if [[ $? -ne 0 ]]
then
    echo "Unable to start ambari-server"
    exit ${RET_SERVER_ERROR}
fi

ambari-agent start
if [[ $? -ne 0 ]]
then
    echo "Unable to start ambari-agent"
    exit ${RET_AGENT_ERROR}
fi

echo "===================================================="
echo "Ambari should be started access the GUI at http://${ambari_server}:8080"
echo ""

echo "Some useful details:"
echo "--------------------"
echo "IOP URL:        file:///$(pwd)/IOP/rhel/7/ppc64le/4.1.x/GA/4.1.0.0/"
echo "IOP-UTILS-1.1.: file:///$(pwd)/IOP-UTILS/rhel/7/ppc64le/1.1/"
echo "FQDN:           $(hostname -a)"



exit ${RET_COMPLETE}



