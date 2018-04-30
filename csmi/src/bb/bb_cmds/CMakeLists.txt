#================================================================================
#
#    csmi/src/bb/bb_cmds/CMakeLists.txt
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

# helloWorld testing executable for csm_bb_cmd
add_executable(helloWorld bb_cmds_helloWorld.c)
install(TARGETS helloWorld COMPONENT csm-api DESTINATION csm/bb_cmds)
target_include_directories(helloWorld PRIVATE ./)
target_link_libraries(helloWorld csmi csm_network_c csmutil)

add_definitions(-DUSE_SC_LOGGER)
