#================================================================================
#   
#    tools/single_aggregator/start_daemons.sh
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

if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../../csm_test.cfg", exitting."
        exit 1
fi

systemctl start csmd-master
systemctl start csmd-aggregator
xdsh ${UTILITY} "systemctl start csmd-utility"
xdsh ${COMPUTE_NODES} "systemctl start csmd-compute"
