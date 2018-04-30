#================================================================================
#
#    csm_big_data/CMakeLists.txt
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
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

add_subdirectory(DataAggregators)
add_subdirectory(LogAnalysis)
add_subdirectory(Logstash)
add_subdirectory(Python)
add_subdirectory(Hadoop)


# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
    "configure_sphinx.sh"
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME})

file(GLOB INSTALL_FILES
    "README.rst"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME})
