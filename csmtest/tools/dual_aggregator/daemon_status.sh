#================================================================================
#   
#    tools/dual_aggregator/daemon_status.sh
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

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

echo "MASTER"
systemctl is-active csmd-master
echo "AGGREGATORS"
xdsh ${AGGREGATOR_A} "systemctl is-active csmd-aggregator"
xdsh ${AGGREGATOR_B} "systemctl is-active csmd-aggregator"
echo "UTILITY"
xdsh utility "systemctl is-active csmd-utility"
echo "COMPUTE"
xdsh csm_comp "systemctl is-active csmd-compute"
