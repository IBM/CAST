#================================================================================
#
#    tools/enable_ssl.sh
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

if [ -f "${BASH_SOURCE%/*}/../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

if [ -f "${BASH_SOURCE%/*}/../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../include/functions.sh"
else
        echo "Could not find functions file expected at /../../include/functions.sh, exitting."
        exit 1
fi

# Copy cert files from xCAT MN to this node
mkdir -p /root/cert
if [[ "${XCATMN}" ]] ; then
  scp "${XCATMN}":/etc/xcat/cert/ca.pem /root/cert/
  scp "${XCATMN}":/etc/xcat/cert/server-cred.pem /root/cert/
else
  # if XCATMN is not set, try to copy locally in case we are already running on the xCAT MN
  cp /etc/xcat/cert/ca.pem /root/cert/
  cp /etc/xcat/cert/server-cred.pem /root/cert/
fi

# Create cert directory on remote nodes
xdsh ${MASTER} "mkdir -p /root/cert"
xdsh ${UTILITY} "mkdir -p /root/cert"
xdsh ${AGGREGATOR_A} "mkdir -p /root/cert"
xdsh ${AGGREGATOR_B} "mkdir -p /root/cert"
xdsh ${COMPUTE_NODES} "mkdir -p /root/cert"

# Distribute xCAT certificate authority, credential files
xdcp ${MASTER} /root/cert/ca.pem /root/cert/
xdcp ${MASTER} /root/cert/server-cred.pem /root/cert/
xdcp ${UTILITY} /root/cert/ca.pem /root/cert/
xdcp ${UTILITY} /root/cert/server-cred.pem /root/cert/
xdcp ${AGGREGATOR_A} /root/cert/ca.pem /root/cert/
xdcp ${AGGREGATOR_A} /root/cert/server-cred.pem /root/cert/
xdcp ${AGGREGATOR_B} /root/cert/ca.pem /root/cert/
xdcp ${AGGREGATOR_B} /root/cert/server-cred.pem /root/cert/
xdcp ${COMPUTE_NODES} /root/cert/ca.pem /root/cert/
xdcp ${COMPUTE_NODES} /root/cert/server-cred.pem /root/cert/

# Modify CSM config files
cd /etc/ibm/csm/
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_master.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_master.cfg
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_utility.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_utility.cfg
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_aggregator_A.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_aggregator_A.cfg
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_aggregator_B.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_aggregator_B.cfg
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_compute_A.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_compute_A.cfg
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_compute_B.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_compute_B.cfg
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_compute.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_compute.cfg
sed -i -- "/ca_file/c\                \"ca_file\": \"/root/cert/ca.pem\"," csm_aggregator.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/root/cert/server-cred.pem\"" csm_aggregator.cfg

# Distribute CSM config files 
xdcp ${MASTER} /etc/ibm/csm/csm_master.cfg /etc/ibm/csm/csm_master.cfg
xdcp ${UTILITY} /etc/ibm/csm/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_aggregator_A.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_aggregator_B.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp ${COMPUTE_A} /etc/ibm/csm/csm_compute_A.cfg /etc/ibm/csm/csm_compute.cfg
xdcp ${COMPUTE_B} /etc/ibm/csm/csm_compute_B.cfg /etc/ibm/csm/csm_compute.cfg
# If csm_compute_A/B.cfg does not exist, distribute in single aggregator config
if [ ! -f /etc/ibm/csm/csm_compute_A.cfg ]
then
	xdcp ${COMPUTE_NODES} /etc/ibm/csm/csm_compute.cfg /etc/ibm/csm/csm_compute.cfg
fi
