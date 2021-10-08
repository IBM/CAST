#================================================================================
#   
#    setup/csm_uninstall.sh
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

# Script to uninstall CSM on test environment
# DEBUG:

#set -x

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi
 
LOG=${LOG_PATH}/setup/csm_uninstall.log
TEMP_LOG=${LOG_PATH}/setup/csm_uninstall.log

if [ -f "${BASH_SOURCE%/*}/../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../include/functions.sh"
else
        echo "Could not find functions file expected at /../include/functions.sh, exitting."
	exit 1
fi

# Output formatter
line1_out=$(printf "%0.s-" {1..90})

# Get list of compute nodes
#compute_node_list=`nodels ${COMPUTE_NODES}`

# Get list of utility nodes
#utility_node_list=`nodels ${UTILITY}`
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

# Comment out any entries for libcsmpam.so in /etc/pam.d/sshd
xdsh ${all} "sed -i '/libcsmpam.so/s/^#*/#/g' /etc/pam.d/sshd"
sleep 1

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

# Stop CSM daemon on Aggregator B
xdsh ${AGGREGATOR_B} "systemctl is-active csmd-aggregator" > /dev/null
if [ $? -eq 0 ]
	then
		xdsh ${AGGREGATOR_B} "systemctl stop csmd-aggregator" > /dev/null
fi

RPMS="ibm-csm-bds ibm-csm-bds-logstash ibm-csm-unittest ibm-csm-db ibm-csm-hcdiag ibm-csm-restd ibm-csm-api ibm-csm-core ibm-fshipmond ibm-fshipd ibm-fshipcld ibm-burstbuffer-tools ibm-burstbuffer-tests ibm-burstbuffer ibm-burstbuffer-mn ibm-burstbuffer-lsf ibm-csm-tools ibm-export_layout ibm-scripts ibm-transport-devel ibm-utilities-devel ibm-flightlog ibm-flightlog-devel"

echo "$line1_out"
for RPM in $RPMS ; do
  if rpm --quiet -q "$RPM" ; then
    rpm -e "$RPM" &> /dev/null
    printf "%-20s %-15s %-30s %-15s\n" "${MASTER}:" "Package:" "${RPM}" "Uninstalled"
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

  # Files cleanup
  xdsh ${all} "rm -rf /etc/ibm /var/log/ibm"
fi

if [ -n "${master}" ] ; then
  rm -rf /etc/ibm /var/log/ibm
fi

# RPM directory clean up
xdsh ${COMPUTE_NODES},${UTILITY} "rm -rf /root/rpms"
if [ `hostname` != ${AGGREGATOR_A} ]
	then
		xdsh ${AGGREGATOR_A} "rm -rf /root/rpms"
fi
if [ ${AGGREGATOR_A} != ${AGGREGATOR_B} ]
	then
		xdsh ${AGGREGATOR_B} "rm -rf /root/rpms"
fi

echo "$line1_out"
exit 0
