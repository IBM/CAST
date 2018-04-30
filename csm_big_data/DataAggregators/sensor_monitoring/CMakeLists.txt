#================================================================================
#
#    csm_big_data/DataAggregators/sensor_monitoring/CMakeLists.txt
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

set(SUBDIR DataAggregators/sensor_monitoring)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
  "bmc_temperature_parse.awk"
  "bmc_temperature_poll.sh"
  "ib_temperature_parse.awk"
  "ib_temperature_poll.sh"
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

# Files that should not be executable when shipped
file(GLOB INSTALL_FILES
  "bmc_hosts"
  "bmc_temperature_poll.cfg"
  "README"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})
