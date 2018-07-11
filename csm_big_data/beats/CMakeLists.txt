#================================================================================
#
#    csm_big_data/elasticsearch/CMakeLists.txt
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

set(SUBDIR elasticsearch)
set(CONFDIR elasticsearch/config)
set(TEMPLATESDIR elasticsearch/templates)

file(GLOB INSTALL_PROGRAMS
    "mappings/createIndices.sh" 
    "mappings/removeIndices.sh"
    "config-scripts/deployConfigs.sh"
    "config-scripts/removeConfigs.sh"
)
install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

file(GLOB CONFIG
    "config/elasticsearch.yml"
    "config/jvm.options"
)
install(FILES ${CONFIG} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${CONFDIR})

file(GLOB TEMPLATES
    "mappings/templates/*json"
)
install(FILES ${TEMPLATES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${TEMPLATESDIR})

