#!/bin/bash
#================================================================================
#   
#    hcdiag/src/tests/chk-zombies/chk-zombies.sh
# 
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#=============================================================================


set -e
model=$(grep model /proc/cpuinfo|cut -d ':' -f2)
echo -e "Running $(basename $0) on $(hostname -s), machine type $model.\n"          

thisdir=`dirname $0`
$thisdir/chk-zombies.pm $@


