#!/bin/bash
###########################################################
#     accesscheck.sh
#
#     Copyright IBM Corporation 2018,2018. All Rights Reserved
#
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
#
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

set -e

echo disk stats:
df

echo root directory
ls -l /

echo writing to /bscfs/myfile
/opt/ibm/bb/tools/randfile --file=/bscfs/myfile

echo reading from /bscfs/myfile
cat /bscfs/myfile
