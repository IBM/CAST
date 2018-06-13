#================================================================================
#   
#    tools/single_aggregator/shutdown_daemons.sh
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

systemctl stop csmd-aggregator
xdsh utility "systemctl stop csmd-utility"
xdsh csm_comp "systemctl stop csmd-compute"
systemctl stop csmd-master
echo "y" | /opt/ibm/csm/db/csm_db_script.sh -d csmdb
/opt/ibm/csm/db/csm_db_script.sh -n csmdb
