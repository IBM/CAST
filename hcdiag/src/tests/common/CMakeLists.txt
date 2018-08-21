#================================================================================
#
#    hcdiag/src/tests/common/CMakeLists.txt
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

set(HCDIAG_TEST common)

file(GLOB HCDIAG_TEST_FILE
  "ClustConf.pm"
  "htx_functions"
  "functions"
  "gpu_functions"
  "jsm_functions"
  "network_functions"
)

file(GLOB HCDIAG_TEST_CONFIG
  "clustconf.yaml.sample"
)

install(FILES ${HCDIAG_TEST_FILE} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/tests/${HCDIAG_TEST})
install(FILES ${HCDIAG_TEST_CONFIG} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/etc)

