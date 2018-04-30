#================================================================================
#
#    hcdiag/src/tests/dcgm-diag/CMakeLists.txt
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

set(HCDIAG_TEST healthmon)

file(GLOB HCDIAG_TEST_FILE
  "healthmon.sh"
)

file(GLOB HCDIAG_TEST_CONF
  "nvidia-healthmon-fst.conf"
)

install(PROGRAMS ${HCDIAG_TEST_FILE} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/tests/${HCDIAG_TEST})
install(FILES ${HCDIAG_TEST_CONF} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/tests/${HCDIAG_TEST})
