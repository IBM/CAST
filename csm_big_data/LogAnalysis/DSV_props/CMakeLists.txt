#================================================================================
#
#    csm_big_data/LogAnalysis/DSV_props/CMakeLists.txt
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

set(SUBDIR LogAnalysis/DSV_props)

file(GLOB INSTALL_FILES
  "bmc_sel.props"
  "bmc_temp_sensor.props"
  "ib_temp_sensor.props"
  "mellanox_event_log.props"
  "mmfs.props"
  "syslog.props"
  "zimon.props"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})
