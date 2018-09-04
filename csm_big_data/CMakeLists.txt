#================================================================================
#
#    csm_big_data/CMakeLists.txt
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

set(BDS_BASE_NAME csm/bigdata)
set(BDS_RPM_NAME csm-bds)

add_subdirectory(elasticsearch)
add_subdirectory(logstash)
add_subdirectory(data-aggregators)
add_subdirectory(beats)
add_subdirectory(config-scripts)
add_subdirectory(python)

set(SCRIPTDIR scripts)


#install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME})


