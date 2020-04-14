#================================================================================
#   
#    setup/csm_setup.sh
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

# Clean local rpms directory
rm -f ${INSTALL_DIR}/ibm-*

# Clean local rpms directory on Aggregator, Utility, Comp nodes
xdsh ${AGGREGATOR_A} "rm -rf /root/rpms/*.rpm"
xdsh csm_comp,utility "rm -rf /root/rpms"

# Grab Daily Build
cp ${RPM_DIR}/ibm-csm-* ${INSTALL_DIR}
cp ${RPM_DIR}/ibm-flightlog-1.* ${INSTALL_DIR}

# Stop old CSM deamons on Utility, Computes & Aggregator
xdsh csm_comp "systemctl stop csmd-compute"
xdsh utility "systemctl stop csmd-utility"
xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator"

# Stop old CSM deamons on Master
systemctl stop csmd-aggregator
systemctl stop csmd-master

# Remove old CSM RPMs on Master
rpm -e ibm-csm-core-* ibm-csm-api-* ibm-csm-db-* ibm-csm-hcdiag-* ibm-csm-restd-* ibm-flightlog-*

# Remove old CSM RPMs on Utility, Computes & Aggregator 
xdsh csm_comp,utility "rpm -e ibm-csm-core-* ibm-csm-api-* ibm-csm-hcdiag-* ibm-flightlog-*"
xdsh ${AGGREGATOR_A} "rpm -e ibm-csm-core-* ibm-csm-api-* ibm-csm-hcdiag-* ibm-flightlog-*"

# File clean up
xdsh csm_comp,utility "rm -rf /etc/ibm /var/log/ibm"
rm -rf /etc/ibm /var/log/ibm

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
xdcp utility -R ${INSTALL_DIR} /root/ 
xdsh utility "rpm -ivh /root/rpms/ibm-flightlog-1.*" 
xdsh utility "rpm -ivh /root/rpms/ibm-csm-core-*"
xdsh utility "rpm -ivh /root/rpms/ibm-csm-api-*"
xdsh utility "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
#xdsh utility "rpm -ivh ${INSTALL_DIR}/ibm-flightlog-0.2.0-*"
#xdsh utility "rpm -ivh ${INSTALL_DIR}/ibm-csm-core-*"
#xdsh utility "rpm -ivh ${INSTALL_DIR}/ibm-csm-api-*"
#xdsh utility "rpm -ivh ${INSTALL_DIR}/ibm-csm-hcdiag-*"

# Install new CSM RPMs - Compute node 
xdcp csm_comp -R ${INSTALL_DIR} /root
xdsh csm_comp "rpm -ivh /root/rpms/ibm-flightlog-1.*"
xdsh csm_comp "rpm -ivh /root/rpms/ibm-csm-core-*"
xdsh csm_comp "rpm -ivh /root/rpms/ibm-csm-api-*"
xdsh csm_comp "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
#xdsh csm_comp "rpm -ivh ${INSTALL_DIR}/ibm-flightlog-0.2.0-*"
#xdsh csm_comp "rpm -ivh ${INSTALL_DIR}/ibm-csm-core-*"
#xdsh csm_comp "rpm -ivh ${INSTALL_DIR}/ibm-csm-api-*"
#xdsh csm_comp "rpm -ivh ${INSTALL_DIR}/ibm-csm-hcdiag-*"

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
xdsh csm_comp,utility "mkdir -p /etc/ibm/csm/"
xdcp csm_comp /etc/ibm/csm/csm_compute.cfg /etc/ibm/csm/csm_compute.cfg
xdcp utility /etc/ibm/csm/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_aggregator.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp csm_comp,utility /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl

# 4.2.3 Prolog/Epilog Scripts Compute
xdcp csm_comp -p /opt/ibm/csm/share/prologs/* /opt/ibm/csm/prologs
xdsh csm_comp,utility "/usr/bin/cp -p /opt/ibm/csm/share/recovery/soft_failure_recovery /opt/ibm/csm/recovery/soft_failure_recovery"

# Start Daemons
systemctl start mosquitto
systemctl start csmd-master
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
xdsh utility "systemctl start csmd-utility"
xdsh csm_comp "systemctl start csmd-compute"

# Daemon Reload
systemctl daemon-reload
xdsh csm_comp,utility "systemctl daemon-reload"

# Start Nvidia daemons
xdsh csm_comp,utility,service "systemctl start nvidia-persistenced"
xdsh csm_comp,utility,service "systemctl start dcgm"
wait
