#================================================================================
#
#    csmd/src/inv/ib_and_switch/src/CMakeLists.txt
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

set(CSM_IB_SRC
# put the source file list here
        standalone_ib_and_switch_collection.cc
        inv_ib_connector_access.cc
	inv_switch_connector_access.cc
)

add_definitions(-DUSE_SC_LOGGER)
add_definitions(-DIB_CONNECTOR)
add_definitions(-DSWITCH_CONNECTOR)

# tried
#add_executable(standalone_ib_and_switch_collection ${CSM_IB_SRC})
#install(TARGETS standalone_ib_and_switch_collection COMPONENT csm-api csm-core DESTINATION csm/sbin)
#add_dependencies(standalone_ib_and_switch_collection csmi csm_network_c csmutil)
#target_include_directories(standalone_ib_and_switch_collection PRIVATE ./)
#target_link_libraries(standalone_ib_and_switch_collection csmi csm_network_c csmutil ${Boost_LIBRARIES} -lpthread)

include_directories(
	../include
        ../../include
)

# trying
add_executable(standalone_ib_and_switch_collection ${CSM_IB_SRC})
target_link_libraries(standalone_ib_and_switch_collection csmi csm_network_c csmutil ${Boost_LIBRARIES} -lpthread)
install(TARGETS standalone_ib_and_switch_collection COMPONENT csm-api DESTINATION csm/sbin)

# working normal
#add_executable(standalone_ib_and_switch_collection ${CSM_IB_SRC})
#target_link_libraries(standalone_ib_and_switch_collection csmutil ${Boost_LIBRARIES} -lpthread)
#install(TARGETS standalone_ib_and_switch_collection COMPONENT csm-core DESTINATION csm/sbin)

message("STANDALONE_IB_AND_SWITCH_COLLECTION_SRC: ${CSM_IB_SRC}")


file(GLOB INSTALL_FILES
  "example_ssl.txt"
  "csm_ufm_ssl_key.txt"
)
install(FILES ${INSTALL_FILES} COMPONENT csm-core DESTINATION csm/share/etc)
