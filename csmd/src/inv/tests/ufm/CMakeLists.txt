#================================================================================
#
#    csmd/src/inv/tests/ufm/CMakeLists.txt
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
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

file(GLOB TEST_UFM_REST
  test_ufm_rest.cc
)

add_executable(test_ufm_rest ${TEST_UFM_REST})
target_link_libraries(test_ufm_rest ${Boost_LIBRARIES} -lpthread)
install(TARGETS test_ufm_rest COMPONENT csm-unittest DESTINATION csm/tests/ufm)

include_directories(
  ${CSMD_INCLUDE_DIRS}
)

target_include_directories(test_ufm_rest PRIVATE ./)
