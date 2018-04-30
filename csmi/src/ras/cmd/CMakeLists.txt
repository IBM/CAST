#================================================================================
#
#    csmi/src/ras/cmd/CMakeLists.txt
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

#CMakeLists for RAS Command line APIs.

include (${CMAKE_SOURCE_DIR}/csmd/csmd.cmake)

# cmd line interface for csm_ras_msg_type_create
add_executable(csm_ras_msg_type_create ras_msg_type_create.c)
install(TARGETS csm_ras_msg_type_create COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ras_msg_type_create PRIVATE ./)
target_link_libraries(csm_ras_msg_type_create csmi csm_network_c csmutil)

# cmd line interface for csm_ras_msg_type_delete
add_executable(csm_ras_msg_type_delete ras_msg_type_delete.c)
install(TARGETS csm_ras_msg_type_delete COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ras_msg_type_delete PRIVATE ./)
target_link_libraries(csm_ras_msg_type_delete csmi csm_network_c csmutil fsutil)

# cmd line interface for csm_ras_msg_type_update
add_executable(csm_ras_msg_type_update ras_msg_type_update.c)
install(TARGETS csm_ras_msg_type_update COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ras_msg_type_update PRIVATE ./)
target_link_libraries(csm_ras_msg_type_update csmi csm_network_c csmutil)

# cmd line interface for csm_ras_msg_type_query
add_executable(csm_ras_msg_type_query ras_msg_type_query.c)
install(TARGETS csm_ras_msg_type_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ras_msg_type_query PRIVATE ./)
target_link_libraries(csm_ras_msg_type_query csmi csm_network_c csmutil)

# cmd line interface for csm_ras_event_create
add_executable(csm_ras_event_create csm_ras_event_create.c)
install(TARGETS csm_ras_event_create COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ras_event_create PRIVATE ./)
target_link_libraries(csm_ras_event_create csmi csm_network_c csmutil)

# cmd line interface for csm_ras_event_query
add_executable(csm_ras_event_query csm_ras_event_query.c)
install(TARGETS csm_ras_event_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ras_event_query PRIVATE ./)
target_link_libraries(csm_ras_event_query csmi csm_network_c csmutil)

# cmd line interface for ras_event_query_allocation.c
add_executable(csm_ras_event_query_allocation ras_event_query_allocation.c)
install(TARGETS csm_ras_event_query_allocation COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ras_event_query_allocation PRIVATE ./)
target_link_libraries(csm_ras_event_query_allocation csmi csm_network_c csmutil)

add_definitions(-DUSE_SC_LOGGER)
