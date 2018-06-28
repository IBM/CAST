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

# Clear any files matching the CSM example config.
clean_dir()
{
    cd $1
    for f in *
    do
        diff -s "${1}/${f}" "${2}/${f}" >/dev/null 2>&1
        if [ $? == 0 ] 
        then
            rm -f "${2}/${f}"
        fi
    done
    rmdir "${2}" 2>/dev/null
}

# ==================
# Constants
# ==================

# Source directory in the CAST RPM.
RPM_SRC="/opt/ibm/csm/bigdata/logstash/"

# The target directory for the logstash config files.
LOGSTASH_TARGET="/etc/logstash/"

# ==================
# Script
# ==================
# Copy the configuration to the local configuration.
clean_dir "${RPM_SRC}patterns" "${LOGSTASH_TARGET}patterns"
clean_dir "${RPM_SRC}config" "${LOGSTASH_TARGET}conf.d"

# Install the logstash plugin.
#/usr/share/logstash/bin/logstash-plugin remove logstash-filter-csm-event-correlator


