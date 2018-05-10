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

DATA_STATE="data_types"
INDEX_STATE="index_types"
TEMPLATES="templates"


#curl -X DELETE "localhost:9200/_template/template_cast"
#curl -X PUT "localhost:9200/_template/template_cast?pretty" -H 'Content-Type: application/json' -d @${TEMPLATES}/cast-template.json

curl -X DELETE "localhost:9200/log-syslog"
curl -X PUT "localhost:9200/log-syslog?pretty" -H 'Content-Type: application/json' -d @${DATA_STATE}/log-syslog-mapping.json




