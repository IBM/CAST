#!/bin/bash
###########################################################
#     bblvm_cleanup.sh
# 
#     Copyright IBM Corporation 2018. All Rights Reserved
# 
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
# 
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################

/bin/find /etc/lvm/archive/. -name 'bb_*' -mtime +7 -type f -delete
