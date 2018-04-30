#================================================================================
#
#    hcdiag/src/tests/chk-nvidia-clocks/CMakeLists.txt
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

set(HCDIAG_TEST chk-nvidia-clocks)

file(GLOB HCDIAG_TEST_PROGRAM
  "chk-nvidia-clocks.sh"
  "chk-nvidia-clocks.pm"
)


install(PROGRAMS ${HCDIAG_TEST_PROGRAM} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/tests/${HCDIAG_TEST})

