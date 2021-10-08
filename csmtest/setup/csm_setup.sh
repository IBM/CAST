#================================================================================
#   
#    setup/csm_setup.sh
# 
#  © Copyright IBM Corporation 2015-2021. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

# Script to install CSM on test environment
# DEBUG:

# set -x

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

LOG=${LOG_PATH}/setup/csm_setup.log
TEMP_LOG=${LOG_PATH}/setup/csm_install.log

if [ -f "${BASH_SOURCE%/*}/../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../include/functions.sh"
else
        echo "Could not find functions file expected at /../include/functions.sh, exitting."
        exit 1
fi

# Output formatter
line1_out=$(printf "%0.s-" {1..90})

# Get hostname of master node
compute=""
compute="${COMPUTE_NODES}"

utility=""
utility="${UTILITY}"

service=""
service="${AGGREGATOR_A},${AGGREGATOR_B}"

master=""
master="${MASTER}"

all=""

if [ -n "${compute}" ] ; then
  all="${all}${compute},"
fi

if [ -n "${utility}" ] ; then
  all="${all}${utility},"
fi

if [ -n "${service}" ] ; then
  all="${all}${service},"
fi

if [ -n "${all}" ] ; then
  # Remove trailing comma
  all=${all%?}
fi

master_node=`hostname`

# Get list of compute nodes
compute_node_list="$compute"

# Get list of utility nodes
utility_node_list="$utility"

# Clean local rpms directory
rm -f ${INSTALL_DIR}/ibm-*

# Clean local rpms directory on Aggregator, Utility, Comp nodes
xdsh ${AGGREGATOR_A} "rm -rf /root/rpms/*.rpm"
xdsh ${COMPUTE_NODES},${UTILITY} "rm -rf /root/rpms"

# Grab Daily Build
cp ${RPM_DIR}/ibm-csm-* ${INSTALL_DIR}
cp ${RPM_DIR}/ibm-flightlog-1.* ${INSTALL_DIR}

# Stop CSM daemons on Compute
for node in $( echo $COMPUTE_NODES | sed "s/,/ /g")
do
    ssh ${node} "systemctl is-active csmd-compute" > /dev/null
    if [ $? -eq 0 ]
        then
            ssh ${node} "systemctl stop csmd-compute" > /dev/null
    fi
done

# Stop CSM daemons on Utility
xdsh ${UTILITY} "systemctl is-active csmd-utility" > /dev/null
if [ $? -eq 0 ]
    then
        xdsh ${UTILITY} "systemctl stop csmd-utility" > /dev/null
fi

# Stop CSM daemons on Aggregator
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator" > /dev/null
if [ $? -eq 0 ]
    then
        xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator" > /dev/null
fi

# Stop CSM daemons on Master
systemctl is-active csmd-master > /dev/null
if [ $? -eq 0 ]
    then
        systemctl stop csmd-master > /dev/null
fi
systemctl is-active csmd-aggregator > /dev/null
if [ $? -eq 0 ]
    then
        systemctl stop csmd-aggregator > /dev/null
fi

# Remove old CSM RPMs on Master
RPMS="ibm-csm-bds ibm-csm-bds-logstash ibm-csm-unittest ibm-csm-db ibm-csm-hcdiag ibm-csm-restd ibm-csm-api ibm-csm-core ibm-fshipmond ibm-fshipd ibm-fshipcld ibm-burstbuffer-tools ibm-burstbuffer-tests ibm-burstbuffer ibm-burstbuffer-mn ibm-burstbuffer-lsf ibm-csm-tools ibm-export_layout ibm-scripts ibm-transport-devel ibm-utilities-devel ibm-flightlog ibm-flightlog-devel"
echo "$line1_out"
for RPM in $RPMS ; do
  if rpm --quiet -q "$RPM" ; then
    rpm -e "$RPM" &> /dev/null
    printf "%-20s %-15s %-25s %-15s\n" "${MASTER}:" "Package:" "${RPM}" "Uninstalled"
  else
    printf "%-20s %-15s %-25s %-15s\n" "${MASTER}:" "Package:" "${RPM}" "Is Not installed"
  fi
done

# Remove all existing rpms from utility, aggregator, and compute nodes
if [ -n "${all}" ] ; then
echo "$line1_out"
xdsh ${all} \
"for RPM in $RPMS ; do \
   if rpm --quiet -q \$RPM ; then \
     rpm -e \$RPM &> /dev/null ; \
     if [ $? -eq 0 ]; then \
       printf \"%-24s %-25s %16s\n\" \"         Package:\" \$RPM \"Uninstalled\" ; \
     fi \
   else
       printf \"%-24s %-25s %15s\n\" \"         Package:\" \$RPM \"Is Not installed\" ; \
   fi \
done"

# File clean up
xdsh ${all} "rm -rf /etc/ibm /var/log/ibm"
fi
echo "$line1_out"

# Install new CSM RPMs - Management node
rpm -ivh ${INSTALL_DIR}/ibm-flightlog-1.*
rpm -ivh ${INSTALL_DIR}/ibm-csm-core-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-api-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-db-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-hcdiag-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-restd-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-bds-logstash-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-bds-1*

# Install new CSM RPMs - Aggregator node
xdcp ${AGGREGATOR_A} -R ${INSTALL_DIR} /root/
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-core-*"
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-api-*"

# Install new CSM RPMs - Utility node (login + launch)
xdcp ${UTILITY} -R ${INSTALL_DIR} /root/ 
xdsh ${UTILITY} "rpm -ivh /root/rpms/ibm-flightlog-1.*" 
xdsh ${UTILITY} "rpm -ivh /root/rpms/ibm-csm-core-*"
xdsh ${UTILITY} "rpm -ivh /root/rpms/ibm-csm-api-*"
xdsh ${UTILITY} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
#xdsh ${UTILITY} "rpm -ivh ${INSTALL_DIR}/ibm-flightlog-0.2.0-*"
#xdsh ${UTILITY} "rpm -ivh ${INSTALL_DIR}/ibm-csm-core-*"
#xdsh ${UTILITY} "rpm -ivh ${INSTALL_DIR}/ibm-csm-api-*"
#xdsh ${UTILITY} "rpm -ivh ${INSTALL_DIR}/ibm-csm-hcdiag-*"

# Install new CSM RPMs - Compute node 
xdcp ${COMPUTE_NODES} -R ${INSTALL_DIR} /root
xdsh ${COMPUTE_NODES} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
xdsh ${COMPUTE_NODES} "rpm -ivh /root/rpms/ibm-csm-core-*"
xdsh ${COMPUTE_NODES} "rpm -ivh /root/rpms/ibm-csm-api-*"
xdsh ${COMPUTE_NODES} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
#xdsh ${COMPUTE_NODES} "rpm -ivh ${INSTALL_DIR}/ibm-flightlog-0.2.0-*"
#xdsh ${COMPUTE_NODES} "rpm -ivh ${INSTALL_DIR}/ibm-csm-core-*"
#xdsh ${COMPUTE_NODES} "rpm -ivh ${INSTALL_DIR}/ibm-csm-api-*"
#xdsh ${COMPUTE_NODES} "rpm -ivh ${INSTALL_DIR}/ibm-csm-hcdiag-*"

# CSM DB cleanup
echo "y" | /opt/ibm/csm/db/csm_db_script.sh -d csmdb

# CSM DB create
/opt/ibm/csm/db/csm_db_script.sh -n csmdb

# Configuration file edit, distribution OLD
mkdir -p /etc/ibm/csm
cp /opt/ibm/csm/share/etc/*.cfg /etc/ibm/csm
cp /opt/ibm/csm/share/etc/csm_api.acl /etc/ibm/csm
cd /etc/ibm/csm/
sed -i -- "s/__MASTER__/${MASTER}/g" *
sed -i -- "s/__AGGREGATOR_A__/${AGGREGATOR_A}/g" *
sed -i -- "s/__AGGREGATOR__/${AGGREGATOR_A}/g" *
sed -i -- "s/\"socket\"              : \"\/run\/csmd\.sock\",/\"socket\"              : \"\/run\/csmd_agg\.sock\",/g" csm_aggregator.cfg
sed -i -- "s/__UFM_REST_ADDRESS__/${UFM_ADDR}/g" *
#sed -i -- "s/__AGGREGATOR_B__/${AGGREGATOR_B}/g" *
#sed -i -- "s/__CSMRESTD_IP__/${localhost}/g" *
cd ${FVT_PATH}/setup/
xdsh ${COMPUTE_NODES},${UTILITY} "mkdir -p /etc/ibm/csm/"
xdcp ${COMPUTE_NODES} /etc/ibm/csm/csm_compute.cfg /etc/ibm/csm/csm_compute.cfg
xdcp ${UTILITY} /etc/ibm/csm/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_aggregator.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp ${COMPUTE_NODES},${UTILITY} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl

# 4.2.3 Prolog/Epilog Scripts Compute
xdcp ${COMPUTE_NODES} -p /opt/ibm/csm/share/prologs/* /opt/ibm/csm/prologs
xdsh ${COMPUTE_NODES},${UTILITY} "/usr/bin/cp -p /opt/ibm/csm/share/recovery/soft_failure_recovery /opt/ibm/csm/recovery/soft_failure_recovery"

# Daemon Reload
systemctl daemon-reload
xdsh ${COMPUTE_NODES},${UTILITY} "systemctl daemon-reload"

# Start Nvidia daemons
if [ -n "${all}" ] ; then
  xdsh "${all}" "systemctl start nvidia-persistenced"
  xdsh "${all}" "systemctl start dcgm"
fi

# Start Daemons
systemctl start csmd-master
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
xdsh ${UTILITY} "systemctl start csmd-utility"
xdsh ${COMPUTE_NODES} "systemctl start csmd-compute"
