#================================================================================
#   
#    setup/csm_uninstall.sh
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

# Script to uninstall CSM on test environment
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
 
LOG=${LOG_PATH}/setup/csm_uninstall.log
TEMP_LOG=${LOG_PATH}/setup/csm_uninstall.log

if [ -f "${BASH_SOURCE%/*}/../include/functions.sh" ]
then
        . "${BASH_SOURCE%/*}/../include/functions.sh"
else
        echo "Could not find functions file expected at /../include/functions.sh, exitting."
	exit 1
fi

# Get list of compute nodes
compute_node_list=`nodels csm_comp`

# Get list of utility nodes
utility_node_list=`nodels utility`

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
for node in ${compute_node_list}
do
	ssh ${node} "systemctl is-active csmd-compute" > /dev/null
	if [ $? -eq 0 ]
		then
			ssh ${node} "systemctl stop csmd-compute" > /dev/null
	fi
done

# Stop CSM daemons on Utility
xdsh utility "systemctl is-active csmd-utility" > /dev/null
if [ $? -eq 0 ]
	then
		xdsh utility "systemctl stop csmd-utility" > /dev/null
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

# Check for RPMs installed on Master, store names
curr_rpm_list=""
curr_rpm_list+=`rpm -qa | grep ibm-csm-api`
curr_rpm_list+=`rpm -qa | grep ibm-csm-core`
curr_rpm_list+=`rpm -qa | grep ibm-csm-hcdiag`
curr_rpm_list+=`rpm -qa | grep ibm-flightlog`
curr_rpm_list+=`rpm -qa | grep ibm-csm-db`
curr_rpm_list+=`rpm -qa | grep ibm-csm-restd`
curr_rpm_list=${curr_rpm_list//.ppc64le/.ppc64le }
curr_rpm_list=${curr_rpm_list//.noarch/.noarch }

# Uninstall old CSM RPMs on Master
if [ "$curr_rpm_list" ]
	then
		rpm -e ${curr_rpm_list}
		if [ $? -ne 0 ]
			then
				echo "Failed to Uninstall CSM RPMs on Master"
				echo "rpm -e ${curr_rpm_list}"
				exit 1
			else
				echo "Uninstalled old CSM RPMs on Master"
		fi
	else
		echo "No RPMs installed on Master"
fi

# Check for RPMs installed on Utility, store names, then uninstall them all
for node in ${utility_node_list}
do
	curr_rpm_list=""
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-csm-api"`
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-csm-core"`
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-csm-hcdiag"`
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-flightlog"`
	curr_rpm_list=${curr_rpm_list//.ppc64le/.ppc64le }
	curr_rpm_list=${curr_rpm_list//.noarch/.noarch }
	# uninstall step
	if [ "$curr_rpm_list" ]
		then
			ssh ${node} "rpm -e ${curr_rpm_list}"
			if [ $? -ne 0 ]
				then
					echo "Failed to uninstall CSM RPMs on utility node ${node}"
					echo "ssh ${node} \"rpm -e ${curr_rpm_list}\""
					exit 1
				else
					echo "Uninstalled CSM RPMs on utility node ${node}"
			fi
		else
			echo "No RPMs installed on utility node ${node}"
	fi
done

# Check for RPMs installed on Aggregator, store names 
if [ `hostname` != ${AGGREGATOR_A} ]
	then
		curr_rpm_list=""
		curr_rpm_list+=`ssh ${AGGREGATOR_A} "rpm -qa | grep ibm-csm-api"`
		curr_rpm_list+=`ssh ${AGGREGATOR_A} "rpm -qa | grep ibm-csm-core"`
		curr_rpm_list+=`ssh ${AGGREGATOR_A} "rpm -qa | grep ibm-csm-hcdiag"`
		curr_rpm_list+=`ssh ${AGGREGATOR_A} "rpm -qa | grep ibm-flightlog"`
		curr_rpm_list+=`ssh ${AGGREGATOR_A} "rpm -qa | grep ibm-csm-restd"`
		curr_rpm_list=${curr_rpm_list//.ppc64le/.ppc64le }
		curr_rpm_list=${curr_rpm_list//.noarch/.noarch }
		# uninstall step
		if [ "$curr_rpm_list" ]
			then
				ssh ${AGGREGATOR_A} "rpm -e ${curr_rpm_list}"
				if [ $? -ne 0 ]
					then
						echo "Failed to uninstall CSM RPMs on aggregator node ${AGGREGATOR_A}"
        		                	echo "ssh ${AGGREGATOR_A} \"rpm -e ${curr_rpm_list}\""
                       		 		exit 1
                			else
                        			echo "Uninstalled CSM RPMs on aggregator node ${AGGREGATOR_A}"
        			fi
			else
				echo "No RPMs installed on agggregator node ${AGGREGATOR_A}"
		fi
fi

# Check for RPMs installed on Aggregator B, store names
if [ ${AGGREGATOR_A} != ${AGGREGATOR_B} ]
	then
		curr_rpm_list=""
                curr_rpm_list+=`ssh ${AGGREGATOR_B} "rpm -qa | grep ibm-csm-api"`
                curr_rpm_list+=`ssh ${AGGREGATOR_B} "rpm -qa | grep ibm-csm-core"`
                curr_rpm_list+=`ssh ${AGGREGATOR_B} "rpm -qa | grep ibm-csm-hcdiag"`
                curr_rpm_list+=`ssh ${AGGREGATOR_B} "rpm -qa | grep ibm-flightlog"`
		curr_rpm_list+=`ssh ${AGGREGATOR_B} "rpm -qa | grep ibm-csm-restd"`
                curr_rpm_list=${curr_rpm_list//.ppc64le/.ppc64le }
                curr_rpm_list=${curr_rpm_list//.noarch/.noarch }
                # uninstall step
		if [ "$curr_rpm_list" ]
			then
 		               	ssh ${AGGREGATOR_B} "rpm -e ${curr_rpm_list}"
              			if [ $? -ne 0 ]
                        		then
						echo "Failed to uninstall CSM RPMs on aggregator node ${AGGREGATOR_B}"
                             			echo "ssh ${AGGREGATOR_B} \"rpm -e ${curr_rpm_list}\""
                             			exit 1
                        		else
                                		echo "Uninstalled CSM RPMs on aggregator node ${AGGREGATOR_B}"
                		fi
			else
				echo "No RPMs installed on aggregator node ${AGGREGATOR_B}"
		fi
fi		

# Check for RPMs installed on compute nodes, store names
for node in ${compute_node_list}
do
	curr_rpm_list=""
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-csm-api"`
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-csm-core"`
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-csm-hcdiag"`
	curr_rpm_list+=`ssh ${node} "rpm -qa | grep ibm-flightlog"`
        curr_rpm_list=${curr_rpm_list//.ppc64le/.ppc64le }
	curr_rpm_list=${curr_rpm_list//.noarch/.noarch }
	# uninstall step
	if [ "$curr_rpm_list" ]
		then
			ssh ${node} "rpm -e ${curr_rpm_list}"
			if [ $? -ne 0 ]
				then
					echo "Failed to uninstall CSM RPMs on compute node ${node}"
					echo "ssh ${node} \"rpm -e ${curr_rpm_list}\""
					exit 1
				else
					echo "Uninstalled CSM RPMs on compute node ${node}"
			fi
		else
			echo "No RPMs installed on compute node ${node}"
	fi
done

# File clean up
xdsh csm_comp,utility "rm -rf /etc/ibm /var/log/ibm"
xdsh ${AGGREGATOR_A} "rm -rf /etc/ibm /var/log/ibm"
xdsh ${AGGREGATOR_B} "rm -rf /etc/ibm /var/log/ibm"
rm -rf /etc/ibm /var/log/ibm

# RPM directory clean up
xdsh csm_comp,utility "rm -rf /root/rpms"
if [ `hostname` != ${AGGREGATOR_A} ]
	then
		xdsh ${AGGREGATOR_A} "rm -rf /root/rpms"
fi
if [ ${AGGREGATOR_A} != ${AGGREGATOR_B} ]
	then
		xdsh ${AGGREGATOR_B} "rm -rf /root/rpms"
fi

exit 0
