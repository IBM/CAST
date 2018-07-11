#!/bin/sh
#================================================================================
#
#    deployConfigs.sh
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


config_filebeats()
{
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
service filebeats status >/dev/null 2>&1
filebeats_status=$?

if [ ${filebeats_status} == 4 ]
then
    echoe "Elasticsearch was not detected"
else
    cd "${RPM_SRC}config/"
    mv -f  "${FILEBEATS_TARGET}filebeat.yml" "${FILEBEATS_TARGET}filebeat.yml.bak"
    cp "${RPM_CONF_SRC}filebeat.yml" "${FILEBEATS_TARGET}filebeat.yml"
fi

# Restart filebeats
service filebeats restart >/dev/null 2>&1

