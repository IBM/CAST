#================================================================================
#
#    csm_big_data/beats/CMakeLists.txt
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

set(SUBDIR beats)
set(CONFDIR beats/config)

file(GLOB INSTALL_PROGRAMS
    "config-scripts/deployConfigs.sh"
    "config-scripts/removeConfigs.sh"
    "utility-scripts/csm_transaction_rebuild.py"
)
install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

file(GLOB CONFIG
    "config/filebeat.yml"
)
install(FILES ${CONFIG} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${CONFDIR})

