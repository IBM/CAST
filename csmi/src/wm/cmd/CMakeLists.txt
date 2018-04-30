#================================================================================
#
#    csmi/src/wm/cmd/CMakeLists.txt
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

# cmd line interface for allocation_create
add_executable(csm_allocation_create allocation_create.c)
install(TARGETS csm_allocation_create COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_create PRIVATE ./)
target_link_libraries(csm_allocation_create csmi csm_network_c csmutil)

# cmd line interface for allocation_delete
add_executable(csm_allocation_delete allocation_delete.c)
install(TARGETS csm_allocation_delete COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_delete PRIVATE ./)
target_link_libraries(csm_allocation_delete csmi csm_network_c csmutil)

# cmd line interface for allocation_query
add_executable(csm_allocation_query allocation_query.c)
install(TARGETS csm_allocation_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_query PRIVATE ./)
target_link_libraries(csm_allocation_query csmi csm_network_c csmutil)

# cmd line interface for allocation_query_details
add_executable(csm_allocation_query_details allocation_query_details.c)
install(TARGETS csm_allocation_query_details COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_query_details PRIVATE ./)
target_link_libraries(csm_allocation_query_details csmi csm_network_c csmutil)

# cmd line interface for allocation_resources_query
add_executable(csm_allocation_resources_query allocation_resources_query.c)
install(TARGETS csm_allocation_resources_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_resources_query PRIVATE ./)
target_link_libraries(csm_allocation_resources_query csmi csm_network_c csmutil)

# cmd line interface for allocation_step_begin
add_executable(csm_allocation_step_begin allocation_step_begin.c)
install(TARGETS csm_allocation_step_begin COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_step_begin PRIVATE ./)
target_link_libraries(csm_allocation_step_begin csmi csm_network_c csmutil fsutil)

# cmd line interface for allocation_step_end
add_executable(csm_allocation_step_end allocation_step_end.c)
install(TARGETS csm_allocation_step_end COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_step_end PRIVATE ./)
target_link_libraries(csm_allocation_step_end csmi csm_network_c csmutil)

# cmd line interface for allocation_step_query
add_executable(csm_allocation_step_query allocation_step_query.c)
install(TARGETS csm_allocation_step_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_step_query PRIVATE ./)
target_link_libraries(csm_allocation_step_query csmi csm_network_c csmutil)

# cmd line interface for allocation_step_query_active_all
add_executable(csm_allocation_step_query_active_all allocation_step_query_active_all.c)
install(TARGETS csm_allocation_step_query_active_all COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_step_query_active_all PRIVATE ./)
target_link_libraries(csm_allocation_step_query_active_all csmi csm_network_c csmutil)

# cmd line interface for allocation_step_query_details
add_executable(csm_allocation_step_query_details allocation_step_query_details.c)
install(TARGETS csm_allocation_step_query_details COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_step_query_details PRIVATE ./)
target_link_libraries(csm_allocation_step_query_details csmi csm_network_c csmutil)

# cmd line interface for node_resources_query
add_executable(csm_node_resources_query node_resources_query.c)
install(TARGETS csm_node_resources_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_resources_query PRIVATE ./)
target_link_libraries(csm_node_resources_query csmi csm_network_c csmutil fsutil)

# cmd line interface for node_resources_query_all
add_executable(csm_node_resources_query_all node_resources_query_all.c)
install(TARGETS csm_node_resources_query_all COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_resources_query_all PRIVATE ./)
target_link_libraries(csm_node_resources_query_all csmi csm_network_c csmutil)

# cmd line interface for allocation_update_history 
add_executable(csm_allocation_update_history allocation_update_history.c)
install(TARGETS csm_allocation_update_history COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_update_history PRIVATE ./)
target_link_libraries(csm_allocation_update_history csmi csm_network_c csmutil)

# cmd line interface for allocation_update_state FIXME do we want this? -John
add_executable(csm_allocation_update_state allocation_update_state.c)
install(TARGETS csm_allocation_update_state COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_update_state PRIVATE ./)
target_link_libraries(csm_allocation_update_state csmi csm_network_c csmutil)

# cmd line interface for allocation_query_active_all
add_executable(csm_allocation_query_active_all allocation_query_active_all.c)
install(TARGETS csm_allocation_query_active_all COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_allocation_query_active_all PRIVATE ./)
target_link_libraries(csm_allocation_query_active_all csmi csm_network_c csmutil)

add_definitions(-DUSE_SC_LOGGER)
