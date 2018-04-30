#================================================================================
#
#    csm_big_data/DataAggregators/CMakeLists.txt
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

add_subdirectory(config_scripts)
add_subdirectory(sensor_monitoring)
add_subdirectory(zimon)


set(SUBDIR DataAggregators)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
  "README.rst"
 )

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})
