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
# Determine if the logstash service is installed on the node.
service logstash status >/dev/null 2>&1
logstash_status=$?

if [ ${logstash_status} == 4 ]
then
    echoe "Logstash was not detected on the node, exiting $0"
    exit 1
fi

# Stop the logstash process before updating.
service logstash stop >/dev/null 2>&1

# Copy the configuration to the local configuration.
cp -r ${RPM_SRC}config/logstash.conf "${LOGSTASH_TARGET}conf.d"
cp -r ${RPM_SRC}config/jvm.options "${LOGSTASH_TARGET}"

cp -r ${RPM_SRC}patterns/* "${LOGSTASH_TARGET}patterns"
chown -R logstash:logstash ${LOGSTASH_TARGET}

echoe "Installing csm-event-correlator"
## Install the logstash plugin.
#/usr/share/logstash/bin/logstash-plugin install \
#    `ls ${RPM_SRC}plugins/logstash-filter-csm-event-correlator-*.gem` 1>&2
#
# Restart the logstash service.
service logstash start >/dev/null 2>&1

