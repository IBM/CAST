#================================================================================
#
#    hcdiag/framework/src/CMakeLists.txt
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

file(GLOB FRAMEWORK_PROGRAM
  "hcdiag_run.py"
  "hcdiag_query.py"
)

file(GLOB FRAMEWORK_FILE
  "base_harness.py"
  "config_parser.py"
  "csmi_interface.py"
  "harness.py"
  "log_handler.py"
  "node_interface.py"
  "proxy.py"
  "target_interface.py"
  "timeout_manager.py"
  "xcat_interface.py"
)


file(GLOB HCDIAG_PROPERTIES
  "test.properties"
  "hcdiag.properties"
)

install(PROGRAMS ${FRAMEWORK_PROGRAM} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/bin)
install(FILES ${FRAMEWORK_FILE} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/bin)
install(FILES ${HCDIAG_PROPERTIES} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/etc)

