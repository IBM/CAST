#================================================================================
#   
#    tools/single_aggregator/daemon_status.sh
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

echo "MASTER"
systemctl is-active csmd-master
echo "AGGREGATOR"
systemctl is-active csmd-aggregator
echo "UTILITY"
xdsh utility "systemctl is-active csmd-utility"
echo "COMPUTE"
xdsh csm_comp "systemctl is-active csmd-compute"
