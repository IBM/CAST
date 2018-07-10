#!/bin/sh
#================================================================================
#
#    removeIndices.sh
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

# Get the script directory for moving the templates.
script_dir=$(dirname $0)
[[ $1 = "" ]] && host=$HOSTNAME || host=$1

TEMPLATES="${script_dir}/templates"
target="${host}:9200"

# Iterate over the templates specified by CAST and remove them from the elasticsearch templates.
for template in ${TEMPLATES}/*json
do
    name=$(basename $template)
    name=${name/.json}
    
    curl -X DELETE "${target}/_template/${name}?pretty" >/dev/null 2>&1
done



