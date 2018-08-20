#!/bin/sh
#================================================================================
#
#    removeConfigs.sh
#
#    © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# ==================
# Functions
# ==================

# Echo to stderr.
echoe()
{
    >&2 echo $@
}

# ==================
# Constants
# ==================

# Source directory in the CAST RPM.
RPM_SRC="/opt/ibm/csm/bigdata/beats/"
RPM_CONF_SRC="${RPM_SRC}config/"

# The target directory for the filebeat config files.
FILEBEATS_TARGET="/etc/filebeats/"

# ==================
# Script
# ==================
# Determine if the elasticsearch service is installed on the node.
service filebeat status >/dev/null 2>&1
filebeats_status=$?

if [ ${filebeats_status} == 4 ]
then
    echoe "Filebeats was not detected."
else
    diff -s "${FILEBEATS_TARGET}filebeats.yml" "${RPM_CONF_SRC}filebeats.yml"
    if [ $? == 0 ]
    then
        rm -f "${FILEBEATS_TARGET}filebeats.yml"
        mv -f "${FILEBEATS_TARGET}filebeats.yml.bak" "${FILEBEATS_TARGET}filebeats.yml"
    fi
    # Restart filebeats
    service filebeat restart >/dev/null 2>&1
fi


