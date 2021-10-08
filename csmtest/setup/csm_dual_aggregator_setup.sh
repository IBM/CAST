#================================================================================
#   
#    setup/csm_dual_aggregator_setup.sh
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

# Clean local rpms directory
rm -f ${INSTALL_DIR}/ibm-*

# Clean local rpms directory on Aggregator, Utility, Comp nodes
xdsh ${AGGREGATOR_A} "rm -rf /root/rpms/*.rpm"
xdsh ${AGGREGATOR_B} "rm -rf /root/rpms/*.rpm"
xdsh ${COMPUTE_NODES},${UTILITY} "rm -rf /root/rpms"

# Grab Daily Build
cp ${RPM_DIR}/ibm-csm-* ${INSTALL_DIR}
cp ${RPM_DIR}/ibm-flightlog-1.* ${INSTALL_DIR}

# Stop old CSM deamons on Utility, Computes & Aggregator
xdsh ${COMPUTE_NODES} "systemctl stop csmd-compute"
xdsh ${UTILITY} "systemctl stop csmd-utility"
xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator"
xdsh ${AGGREGATOR_B} "systemctl stop csmd-aggregator"

# Stop old CSM deamons on Master
systemctl stop csmd-aggregator
systemctl stop csmd-master

# Remove old CSM RPMs on Master
rpm -e ibm-csm-core-* ibm-csm-api-* ibm-csm-db-* ibm-csm-hcdiag-* ibm-csm-restd-* ibm-flightlog-*

# Remove old CSM RPMs on Utility, Computes & Aggregator 
xdsh ${COMPUTE_NODES},${UTILITY} "rpm -e ibm-csm-core-* ibm-csm-api-* ibm-csm-hcdiag-* ibm-flightlog-*"
xdsh ${AGGREGATOR_A} "rpm -e ibm-csm-core-* ibm-csm-api-* ibm-csm-hcdiag-* ibm-flightlog-*"
xdsh ${AGGREGATOR_B} "rpm -e ibm-csm-core-* ibm-csm-api-* ibm-csm-hcdiag-* ibm-flightlog-*"

# File clean up
xdsh ${COMPUTE_NODES},${UTILITY} "rm -rf /etc/ibm /var/log/ibm"
rm -rf /etc/ibm /var/log/ibm
xdsh ${AGGREGATOR_A} "rm -rf /etc/ibm /var/log/ibm"
xdsh ${AGGREGATOR_B} "rm -rf /etc/ibm /var/log/ibm"

# Install new CSM RPMs - Management node
rpm -ivh ${INSTALL_DIR}/ibm-flightlog-1.*
rpm -ivh ${INSTALL_DIR}/ibm-csm-core-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-api-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-db-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-hcdiag-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-restd-*

# Install new CSM RPMs - Aggregator node
xdcp ${AGGREGATOR_A} -R ${INSTALL_DIR} /root/
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-core-*"
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-api-*"

# Install new CSM RPMs - Aggregator B
xdcp ${AGGREGATOR_B} -R ${INSTALL_DIR} /root/
xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-csm-core-*"
xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-csm-api-*"

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

# Configuration file creation
mkdir -p /etc/ibm/csm
cp /opt/ibm/csm/share/etc/csm_aggregator.cfg /etc/ibm/csm/csm_aggregator_A.cfg
cp /opt/ibm/csm/share/etc/csm_aggregator.cfg /etc/ibm/csm/csm_aggregator_B.cfg
cp /opt/ibm/csm/share/etc/csm_compute.cfg /etc/ibm/csm/csm_compute_A.cfg
cp /opt/ibm/csm/share/etc/csm_compute.cfg /etc/ibm/csm/csm_compute_B.cfg
cp /opt/ibm/csm/share/etc/csm_master.cfg /etc/ibm/csm/csm_master.cfg
cp /opt/ibm/csm/share/etc/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
cp /opt/ibm/csm/share/etc/csm_api.acl /etc/ibm/csm/csm_api.acl
cp /opt/ibm/csm/share/etc/csm_api.cfg /etc/ibm/csm/csm_api.cfg

# Configuration file edit
cd /etc/ibm/csm/
sed -i -- "s/__MASTER__/${MASTER}/g" *
sed -i -- "s/__UFM_REST_ADDRESS__/${UFM_ADDR}/g" *
sed -i -- "s/\"socket\"              : \"\/run\/csmd\.sock\",/\"socket\"              : \"\/run\/csmd_agg\.sock\",/g" csm_aggregator_A.cfg
sed -i -- "s/\"socket\"              : \"\/run\/csmd\.sock\",/\"socket\"              : \"\/run\/csmd_agg\.sock\",/g" csm_aggregator_B.cfg
sed -i -- "s/__AGGREGATOR__/${AGGREGATOR_A}/g" csm_aggregator_A.cfg
sed -i -- "s/__AGGREGATOR__/${AGGREGATOR_B}/g" csm_aggregator_B.cfg
sed -i -- "s/__AGGREGATOR_A__/${AGGREGATOR_A}/g" csm_compute_A.cfg
sed -i -- "s/__AGGREGATOR_B__/${AGGREGATOR_B}/g" csm_compute_A.cfg
sed -i -- "s/__AGGREGATOR_A__/${AGGREGATOR_B}/g" csm_compute_B.cfg
sed -i -- "s/__AGGREGATOR_B__/${AGGREGATOR_A}/g" csm_compute_B.cfg

# Configuration file distribution
cd ${FVT_PATH}/setup/
xdsh ${COMPUTE_NODES},${UTILITY} "mkdir -p /etc/ibm/csm"
xdsh ${AGGREGATOR_A} "mkdir -p /etc/ibm/csm"
xdsh ${AGGREGATOR_B} "mkdir -p /etc/ibm/csm"
xdcp ${COMPUTE_A} /etc/ibm/csm/csm_compute_A.cfg /etc/ibm/csm/csm_compute.cfg
xdcp ${COMPUTE_B} /etc/ibm/csm/csm_compute_B.cfg /etc/ibm/csm/csm_compute.cfg
xdcp ${UTILITY} /etc/ibm/csm/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_aggregator_A.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_aggregator_B.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp ${COMPUTE_NODES},${UTILITY} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_api.cfg /etc/ibm/csm/csm_api.cfg
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_api.cfg /etc/ibm/csm/csm_api.cfg
xdcp ${COMPUTE_NODES},${UTILITY} /etc/ibm/csm/csm_api.cfg /etc/ibm/csm/csm_api.cfg

# 4.2.3 Prolog/Epilog Scripts Compute
xdcp ${COMPUTE_NODES} /opt/ibm/csm/share/prologs/* /opt/ibm/csm/prologs

# Start Daemons
systemctl start csmd-master
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
xdsh ${AGGREGATOR_B} "systemctl start csmd-aggregator"
xdsh ${UTILITY} "systemctl start csmd-utility"
xdsh ${COMPUTE_NODES} "systemctl start csmd-compute"

# Daemon Reload
systemctl daemon-reload
xdsh ${COMPUTE_NODES},${UTILITY} "systemctl daemon-reload"
