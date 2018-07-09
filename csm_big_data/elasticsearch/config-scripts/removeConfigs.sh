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
RPM_SRC="/opt/ibm/csm/bigdata/elasticsearch/"

# The target directory for the elasticsearch config files.
ELASTICSEARCH_TARGET="/etc/elasticsearch/"

# ==================
# Script
# ==================
# Determine if the elasticsearch service is installed on the node.
service elasticsearch status >/dev/null 2>&1
elasticsearch_status=$?

if [ ${elasticsearch_status} == 4 ]
then
    echoe "Elasticsearch was not detected on the node, exiting $0"
    exit 1
fi

if [ ${elasticsearch_status} == 0 ]
then
    # Create the elastic indices.
    ${RPM_SRC}removeIndices.sh
fi

# Remove the local configuration files and replace with the backups.
cd "${RPM_SRC}config/"
for file in  *
do
    diff -s ${ELASTICSEARCH_TARGET}${file} ${file} >/dev/null 2>&1
    if [ $? == 0 ]
    then
        rm -f ${ELASTICSEARCH_TARGET}${file}
        mv -f ${ELASTICSEARCH_TARGET}${file}.bak ${ELASTICSEARCH_TARGET}${file}
    fi
done

# Restart elasticsearch.
service elasticsearch restart >/dev/null 2>&1

