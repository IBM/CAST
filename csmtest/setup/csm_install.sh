#================================================================================
#   
#    setup/csm_install.sh
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

LOG=${LOG_PATH}/setup/csm_install.log
TEMP_LOG=${LOG_PATH}/setup/csm_install.log

if [ -f "${BASH_SOURCE%/*}/../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../include/functions.sh"
else
        echo "Could not find functions file expected at /../include/functions.sh, exitting."
        exit 1
fi

# Get hostname of master node
master_node=`hostname`

# Get list of compute nodes
compute_node_list=`nodels csm_comp`

# Get list of utility nodes
utility_node_list=`nodels utility`

#----------------------------------------------------------------------
# RPM Replace and Download Section
#----------------------------------------------------------------------

# Replace Old RPMs in INSTALL_DIR on Master with RPMs from RPM_DIR
ls ${INSTALL_DIR}/ibm-csm-core-* > /dev/null 2>&1
if [ $? -eq 0 ]
	then
		rm -rf ${INSTALL_DIR}/*.rpm > /dev/null
		cp ${RPM_DIR}/ibm-csm-*.rpm ${INSTALL_DIR} > /dev/null
		cp ${RPM_DIR}/ibm-flightlog-1.* ${INSTALL_DIR} > /dev/null
	else
		cp ${RPM_DIR}/ibm-csm-*.rpm ${INSTALL_DIR} > /dev/null
                cp ${RPM_DIR}/ibm-flightlog-1.* ${INSTALL_DIR} > /dev/null
fi

# Replace Old RPMs in /root/rpms on Aggregators with RPMs from INSTALL_DIR
if [ ${master_node} != ${AGGREGATOR_A} ]
	then
		ssh ${AGGREGATOR_A} "ls /root/rpms/ibm-csm-core-*" > /dev/null 2>&1
		if [ $? -eq 0 ]
			then
				rm -rf /root/rpms/*.rpm > /dev/null
				xdcp ${AGGREGATOR_A} -R ${INSTALL_DIR} /root/ > /dev/null
			else
				xdcp ${AGGREGATOR_A} -R ${INSTALL_DIR} /root/ > /dev/null
		fi
fi

if [ ${AGGREGATOR_A} != ${AGGREGATOR_B} ]
	then
		ssh ${AGGREGATOR_B} "ls /root/rpms/ibm-csm-core-*" > /dev/null 2>&1
		if [ $? -eq 0 ]
			then
				rm -rf /root/rpms/*.rpm > /dev/null
				xdcp ${AGGREGATOR_B} -R ${INSTALL_DIR} /root/ > /dev/null
			else
				xdcp ${AGGREGATOR_B} -R ${INSTALL_DIR} /root/ > /dev/null
		fi
fi

# Replace Old RPMs in /root/rpms on Utility nodes with RPMs from INSTALL_DIR
for node in ${utility_node_list}
do
	ssh ${node} "ls /root/rpms/ibm-csm-core-*" > /dev/null 2>&1
	if [ $? -eq 0 ]
		then
			rm -rf /root/rpms/*.rpm > /dev/null
			xdcp ${node} -R ${INSTALL_DIR} /root/ > /dev/null
		else
			xdcp ${node} -R ${INSTALL_DIR} /root/ > /dev/null
	fi
done

# Replace Old RPMs in /root/rpms on Compute nodes with RPMs from INSTALL_DIR
for node in ${compute_node_list}
do
        ssh ${node} "ls /root/rpms/ibm-csm-core-*" > /dev/null 2>&1
        if [ $? -eq 0 ]
                then
                        rm -rf /root/rpms/*.rpm > /dev/null
                        xdcp ${node} -R ${INSTALL_DIR} /root/ > /dev/null
                else
                        xdcp ${node} -R ${INSTALL_DIR} /root/ > /dev/null
        fi
done

#----------------------------------------------------------------------
# Install Section
#----------------------------------------------------------------------

# Install RPMs on Master
rpm -ivh ${INSTALL_DIR}/ibm-flightlog-1.*
rpm -ivh ${INSTALL_DIR}/ibm-csm-core-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-api-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-db-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-hcdiag-*
rpm -ivh ${INSTALL_DIR}/ibm-csm-restd-*

# Install RPMs on Aggregators
if [ ${master_node} != ${AGGREGATOR_A} ]
	then
		xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
		xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-core-*"
		xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-api-*"
		xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
		xdsh ${AGGREGATOR_A} "rpm -ivh /root/rpms/ibm-csm-restd-*"
fi

if [ ${AGGREGATOR_A} != ${AGGREGATOR_B} ]
	then
		xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
                xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-csm-core-*"
                xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-csm-api-*"
                xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
                xdsh ${AGGREGATOR_B} "rpm -ivh /root/rpms/ibm-csm-restd-*"
fi

# Install RPMs on Utility Nodes
for node in ${utility_node_list}
do
	xdsh ${node} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
	xdsh ${node} "rpm -ivh /root/rpms/ibm-csm-core-*"
	xdsh ${node} "rpm -ivh /root/rpms/ibm-csm-api-*"
	xdsh ${node} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
done

# Install RPMs on Compute Nodes
for node in ${compute_node_list}
do
	xdsh ${node} "rpm -ivh /root/rpms/ibm-flightlog-1.*"
	xdsh ${node} "rpm -ivh /root/rpms/ibm-csm-core-*"
	xdsh ${node} "rpm -ivh /root/rpms/ibm-csm-api-*"
	xdsh ${node} "rpm -ivh /root/rpms/ibm-csm-hcdiag-*"
done

#----------------------------------------------------------------------
# DB Section
#----------------------------------------------------------------------

# CSM DB cleanup
echo "y" | /opt/ibm/csm/db/csm_db_script.sh -d csmdb

# CSM DB create
/opt/ibm/csm/db/csm_db_script.sh -n csmdb

#----------------------------------------------------------------------
# Config File Section
#----------------------------------------------------------------------

mkdir -p /etc/ibm/csm
cp /opt/ibm/csm/share/etc/csm_aggregator.cfg /etc/ibm/csm/csm_aggregator_A.cfg
cp /opt/ibm/csm/share/etc/csm_aggregator.cfg /etc/ibm/csm/csm_aggregator_B.cfg
cp /opt/ibm/csm/share/etc/csm_master.cfg /etc/ibm/csm/csm_master.cfg
cp /opt/ibm/csm/share/etc/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
cp /opt/ibm/csm/share/etc/csm_api.acl /etc/ibm/csm/csm_api.acl
cp /opt/ibm/csm/share/etc/csm_api.cfg /etc/ibm/csm/csm_api.cfg
if [ ${AGGREGATOR_A} == ${AGGREGATOR_B} ]
	then
		cp /opt/ibm/csm/share/etc/csm_compute.cfg /etc/ibm/csm/csm_compute.cfg
	else
		cp /opt/ibm/csm/share/etc/csm_compute.cfg /etc/ibm/csm/csm_compute_A.cfg
		cp /opt/ibm/csm/share/etc/csm_compute.cfg /etc/ibm/csm/csm_compute_B.cfg
fi

# Configuration file edit
cd /etc/ibm/csm/
sed -i -- "s/__MASTER__/${MASTER}/g" *
sed -i -- "s/__UFM_REST_ADDRESS__/${UFM_ADDR}/g" *
sed -i -- "s/\"socket\"              : \"\/run\/csmd\.sock\",/\"socket\"              : \"\/run\/csmd_agg\.sock\",/g" csm_aggregator_A.cfg
sed -i -- "s/\"socket\"              : \"\/run\/csmd\.sock\",/\"socket\"              : \"\/run\/csmd_agg\.sock\",/g" csm_aggregator_B.cfg
sed -i -- "s/__AGGREGATOR__/${AGGREGATOR_A}/g" csm_aggregator_A.cfg
sed -i -- "s/__AGGREGATOR__/${AGGREGATOR_B}/g" csm_aggregator_B.cfg
if [ ${AGGREGATOR_A} == ${AGGREGATOR_B} ]
	then
		sed -i -- "s/__AGGREGATOR_A__/${AGGREGATOR_A}/g" csm_compute.cfg
	else	
		sed -i -- "s/__AGGREGATOR_A__/${AGGREGATOR_A}/g" csm_compute_A.cfg
		sed -i -- "s/__AGGREGATOR_B__/${AGGREGATOR_B}/g" csm_compute_A.cfg
		sed -i -- "s/__AGGREGATOR_A__/${AGGREGATOR_B}/g" csm_compute_B.cfg
		sed -i -- "s/__AGGREGATOR_B__/${AGGREGATOR_A}/g" csm_compute_B.cfg
fi
if [ ! -z $LOGSTASH ]
	then
		sed -i -- "s/__LOGSTASH__/${LOGSTASH}/g" csm_aggregator_A.cfg
		sed -i -- "s/__LOGSTASH__/${LOGSTASH}/g" csm_aggregator_B.cfg
	else
		echo "LOGSTASH not defined in csm_test.cfg.  Skipping BDS setup" 
fi

# Configuration file distribution
xdsh csm_comp,utility "mkdir -p /etc/ibm/csm"
xdsh ${AGGREGATOR_A} "mkdir -p /etc/ibm/csm"
xdsh ${AGGREGATOR_B} "mkdir -p /etc/ibm/csm"
xdcp utility /etc/ibm/csm/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_aggregator_A.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_aggregator_B.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp csm_comp,utility /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_api.acl /etc/ibm/csm/csm_api.acl
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_api.cfg /etc/ibm/csm/csm_api.cfg
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_api.cfg /etc/ibm/csm/csm_api.cfg
xdcp csm_comp,utility /etc/ibm/csm/csm_api.cfg /etc/ibm/csm/csm_api.cfg
if [ ${AGGREGATOR_A} == ${AGGREGATOR_B} ]
	then
		xdcp csm_comp /etc/ibm/csm/csm_compute.cfg /etc/ibm/csm/csm_compute.cfg
	else
		xdcp compute_A /etc/ibm/csm/csm_compute_A.cfg /etc/ibm/csm/csm_compute.cfg
		xdcp compute_B /etc/ibm/csm/csm_compute_B.cfg /etc/ibm/csm/csm_compute.cfg
fi

# 4.2.3 Prolog/Epilog Scripts Compute
xdcp csm_comp /opt/ibm/csm/share/prologs/* /opt/ibm/csm/prologs

# Start Daemons
systemctl start csmd-master
xdsh ${AGGREGATOR_A} "systemctl start csmd-aggregator"
xdsh ${AGGREGATOR_B} "systemctl start csmd-aggregator"
xdsh utility "systemctl start csmd-utility"
xdsh csm_comp "systemctl start csmd-compute"

# Daemon Reload
systemctl daemon-reload
xdsh csm_comp,utility "systemctl daemon-reload"
