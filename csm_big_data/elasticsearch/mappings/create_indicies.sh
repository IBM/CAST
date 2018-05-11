#================================================================================
#
#    csmi/src/wm/src/csmi_allocation_step_query_details.c
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
#!/bin/sh

#INDICES="indices"
TEMPLATES="templates"
target="localhost:9200"

for template in ${TEMPLATES}/*json
do
    name=$(basename $template)
    name=${name/.json}
    
    template_found=$(curl -s -o /dev/null -I -w "%{http_code}" "${target}/_template/${name}?pretty")

#    curl -X DELETE "${target}/_template/${name}?pretty"
    template_put=$(curl -s -o /dev/null -w "%{http_code}" -X PUT "${target}/_template/${name}?pretty"\
        -H 'Content-Type: application/json' -d @${template})

    echo "${template_found} ${template_put}"
done



