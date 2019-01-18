#================================================================================
#
#    tools/complete_fvt.sh
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

# Create cert directory on remote nodes
xdsh utility "mkdir /root/cert"
xdsh ${AGGREGATOR_A} "mkdir /root/cert"
xdsh ${AGGREGATOR_B} "mkdir /root/cert"
xdsh ${COMPUTE_NODES} "mkdir /root/cert"

# Distribute xCAT certificate authority, credential files
xdcp utility /etc/xcat/cert/ca.pem /root/cert/
xdcp utility /etc/xcat/cert/server-cred.pem /root/cert
xdcp ${AGGREGATOR_A} /etc/xcat/cert/ca.pem /root/cert/
xdcp ${AGGREGATOR_A} /etc/xcat/cert/server-cred.pem /root/cert
xdcp ${AGGREGATOR_B} /etc/xcat/cert/ca.pem /root/cert/
xdcp ${AGGREGATOR_B} /etc/xcat/cert/server-cred.pem /root/cert
xdcp ${COMPUTE_NODES} /etc/xcat/cert/ca.pem /root/cert/
xdcp ${COMPUTE_NODES} /etc/xcat/cert/server-cred.pem /root/cert

# Modify CSM config files
cd /etc/ibm/csm/
sed -i -- "/ca_file/c\                \"ca_file\": \"/etc/xcat/cert/ca.pem\"," csm_master.cfg
sed -i -- "/cred_pem/c\                \"cred_pem\": \"/etc/xcat/cert/server-cred.pem\"" csm_master.cfg
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
xdcp utility /etc/ibm/csm/csm_utility.cfg /etc/ibm/csm/csm_utility.cfg
xdcp ${AGGREGATOR_A} /etc/ibm/csm/csm_aggregator_A.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp ${AGGREGATOR_B} /etc/ibm/csm/csm_aggregator_B.cfg /etc/ibm/csm/csm_aggregator.cfg
xdcp compute_A /etc/ibm/csm/csm_compute_A.cfg /etc/ibm/csm/csm_compute.cfg
xdcp compute_B /etc/ibm/csm/csm_compute_B.cfg /etc/ibm/csm/csm_compute.cfg
# If csm_compute_A/B.cfg does not exist, distribute in single aggregator config
if [ ! -f /etc/ibm/csm/csm_compute_A.cfg ]
then
	xdcp csm_comp /etc/ibm/csm/csm_compute.cfg /etc/ibm/csm/csm_compute.cfg
fi
