#================================================================================
#
#    csmi/src/launch/src/CMakeLists.txt
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

file(GLOB CSM_LAUNCH_SRC
  csm_launch_main.cc
)

add_executable(csmlaunch ${CSM_LAUNCH_SRC})

add_definitions(-DUSE_SC_LOGGER)

target_link_libraries(csmlaunch csmi csm_network_c csmutil)

install(TARGETS csmlaunch COMPONENT csm-unittest DESTINATION csm/tests/launch)

message("CSM_LAUNCH_SRC: ${CSM_LAUNCH_SRC}")
