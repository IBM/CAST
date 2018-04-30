#================================================================================
#
#    csmd/src/inv/tests/dcgm/CMakeLists.txt
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
include (${CMAKE_SOURCE_DIR}/csmd/csmd.cmake)

file(GLOB TEST_DCGM 
  test_dcgm.cc
)

add_executable(test_dcgm ${TEST_DCGM})
target_link_libraries(test_dcgm -ldcgm)
install(TARGETS test_dcgm COMPONENT csm-unittest DESTINATION csm/tests/dcgm)

include_directories(
  ${CSMD_INCLUDE_DIRS}
)

target_include_directories(test_dcgm PRIVATE ./)
