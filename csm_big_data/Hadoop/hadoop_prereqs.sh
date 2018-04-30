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

#=========================================================

commands="hu:n:"

#: The base user id for the hadoop users.
uid_base=1050

#: A list of ntp servers to connect to.
ntp_servers=""

usage()
{
    cat << EOF >&2
    Usage $0
    =========
    -h               Displays this message.
    -u {uid}         The base UID for generating hadoop users.
    -n {ntp servers} The ntp servers to point the node at.

    Error Codes
    ===========
    0 - Script executed with no problems.
    1 - Execution terminated in the execution of getopts.
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

# Create the hadoop group and users.
# 1 - The starting id of the user 
configure_hadoop_users()
{
    id=$1

    if [[ $(grep -c hadoop /etc/group) -eq 0 ]]
    then
        groupadd --gid $((${id})) hadoop
    fi

    if [[ $(grep -c rddcached /etc/group) -eq 0 ]]
    then
        groupadd --gid $((${id}+16)) rddcached 
    fi

    useradd -u $((${id}   ))  -g hadoop ams       &> /dev/null 
    useradd -u $((${id}+1 ))  -g hadoop hive      &> /dev/null
    useradd -u $((${id}+2 ))  -g hadoop oozie     &> /dev/null
    useradd -u $((${id}+3 ))  -g hadoop ambari-qa &> /dev/null
    useradd -u $((${id}+4 ))  -g hadoop flume     &> /dev/null
    useradd -u $((${id}+5 ))  -g hadoop hdfs      &> /dev/null
    useradd -u $((${id}+6 ))  -g hadoop solr      &> /dev/null
    useradd -u $((${id}+7 ))  -g hadoop knox      &> /dev/null
    useradd -u $((${id}+8 ))  -g hadoop spark     &> /dev/null
    useradd -u $((${id}+9 ))  -g hadoop mapred    &> /dev/null
    useradd -u $((${id}+10))  -g hadoop hbase     &> /dev/null
    useradd -u $((${id}+11))  -g hadoop zookeeper &> /dev/null
    useradd -u $((${id}+12))  -g hadoop sqoop     &> /dev/null
    useradd -u $((${id}+13))  -g hadoop yarn      &> /dev/null
    useradd -u $((${id}+14))  -g hadoop hcat      &> /dev/null
    useradd -u $((${id}+15))  -g hadoop kafka     &> /dev/null
                       
    useradd -u $((${id}+16)) -g rddcached rddcached &> /dev/null 
    useradd -u $((${id}+17)) -g users ioala         &> /dev/null

    echo "Changing home directory access to 755."
    chmod 755 -R /home
}

while getopts ${commands} opt
do
    case ${opt} in
        h) 
            usage; exit ${RET_INPUT_EXIT};;
        u)
            uid_base=${OPTARG}
            ;;
        n)
            ntp_servers=(${OPTARG})
            ;;
        *)
            echoe "Invaild option: ${opt}"
            usage
            exit ${RET_INPUT_EXIT}
            ;;
    esac
done

display_header "Fixing permissions"
chmod 755 /
chmod 755 /bin
chmod 755 /lib
echo "'/', '/bin', and '/lib' changed to 755 for su commands"

#: Installing the required packages:
display_header "Installing required packages:"
yum install -y createrepo postgresql-jdbc* java-1.8.0 postgresql-server

#: Configure the users for the Hadoop serivces.
display_header "Configuring hadoop users:"
configure_hadoop_users ${uid_base}

#: Fully qualified domain name.
FQDN=$(hostname -a)
display_header "Fully Qualified Domain Name: ${FQDN}"
if [[ ${FQDN} =~ [A-Z] ]]
then
    echo "WARNING: Hostname has an uppercase character, please fix this issue before continuing."
fi

#: Disable the firewall
display_header "Disabling Firewall:"
systemctl stop firewalld.service
systemctl disable firewalld.service

#: Disable IPv6
display_header "Disabling IPv6"
echo "Modifying /etc/sysctl.d/00-system.conf"
cat << EOF > /etc/sysctl.d/00-system.conf 
sysctl -w net.ipv6.conf.all.disable_ipv6=1
net.ipv6.conf.all.disable_ipv6 = 1
sysctl -w net.ipv6.conf.default.disable_ipv6=1
net.ipv6.conf.default.disable_ipv6 = 1
EOF

echo "Modifying /etc/sysctl.conf"
sed -ni '/net.ipv6.conf.default.disable_ipv6/!p' /etc/sysctl.conf
echo "net.ipv6.conf.default.disable_ipv6 = 1" >> /etc/sysctl.conf

#: Find out if ambari is installed.
display_header "Checking for Ambari"
if [[ $(yum list installed | grep -ic ambari) -ne 0 ]]
then
    echo "WARNING: Ambari was already installed on this node!"
    echo "A cleanup must be performed."
    echo "Please see https://www.ibm.com/support/knowledgecenter/en/SSPT3X_4.1.0/com.ibm.swg.im.infosphere.biginsights.install.doc/doc/bi_install_cleanup_nodes_for_reinstall.html"
else
    echo "Ambari was not found"
fi

#: Set the ulimit of the node.
display_header "Setting ulimits"
sed -ni '/nofile/!p'      /etc/security/limits.conf
sed -ni '/nproc/!p'       /etc/security/limits.conf
sed -ni '/fs.file-max/!p' /etc/sysctl.conf

echo "*   soft nofile 65536" >> /etc/security/limits.conf
echo "*   hard nofile 65536" >> /etc/security/limits.conf
echo "*   soft nproc 65536"  >> /etc/security/limits.conf
echo "*   hard nproc 65536"  >> /etc/security/limits.conf
echo "fs.file-max" = 65536   >> /etc/sysctl.conf 
sysctl -p

#: Set ntp
display_header "Setting up ntp"

for ntp_server in ${ntp_servers[@]}
do
    ping -c1 ${ntp_server} &>/dev/null
    if [[ $? -eq 0 ]]
    then
        if [[ $(grep -c ${ntp_server} /etc/ntp.conf ) = 0 ]]
        then
            echo "Adding ${ntp_server} to /etc/ntp.conf"
            echo "server ${ntp_server}" >> /etc/ntp.conf
        else
            echo "${ntp_server} was already present in the ntp.conf file."
        fi 
    else
        echo "${ntp_server} was not reachable."
    fi
done

systemctl stop ntpd.service
systemctl start ntpd.service
systemctl enable ntpd.service

#: Disable SELinux
display_header "Disabling SELinux"
setenforce 0
sed -i 's/^SELINUX=\(.*\)/SELINUX=disabled/g' /etc/selinux/config

exit ${RET_COMPLETE}

