#================================================================================
#
#    hcdiag/CMakeLists.txt
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
set(HCDIAG_BASENAME hcdiag)
set(HCDIAG_RPM_NAME csm-hcdiag)

add_subdirectory(samples)
add_subdirectory(src/framework)
add_subdirectory(src/tests)

file(GLOB HCDIAG_README_FILE
  "README"
)
install(FILES ${HCDIAG_README_FILE} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag)

