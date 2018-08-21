#================================================================================
#
#    csmi/src/inv/tests/CMakeLists.txt
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
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

# cmd line interface for cluster_query_state
add_executable(csm_cluster_query_state cluster_query_state.c)
install(TARGETS csm_cluster_query_state COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_cluster_query_state PRIVATE ./)
target_link_libraries(csm_cluster_query_state csmi csm_network_c csmutil fsutil)

# cmd line interface for ib_cable_query
add_executable(csm_ib_cable_query ib_cable_query.c)
install(TARGETS csm_ib_cable_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ib_cable_query PRIVATE ./)
target_link_libraries(csm_ib_cable_query csmi csm_network_c csmutil fsutil)

# cmd line interface for ib_cable_query_history
add_executable(csm_ib_cable_query_history ib_cable_query_history.c)
install(TARGETS csm_ib_cable_query_history COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ib_cable_query_history PRIVATE ./)
target_link_libraries(csm_ib_cable_query_history csmi csm_network_c csmutil fsutil)

# cmd line interface for ib_cable_update
add_executable(csm_ib_cable_update ib_cable_update.c)
install(TARGETS csm_ib_cable_update COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_ib_cable_update PRIVATE ./)
target_link_libraries(csm_ib_cable_update csmi csm_network_c csmutil fsutil)

# cmd line interface for node_attributes_query
add_executable(csm_node_attributes_query node_attributes_query.c)
install(TARGETS csm_node_attributes_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_attributes_query PRIVATE ./)
target_link_libraries(csm_node_attributes_query csmi csm_network_c csmutil fsutil)

# cmd line interface for node_attributes_query_details
add_executable(csm_node_attributes_query_details node_attributes_query_details.c)
install(TARGETS csm_node_attributes_query_details COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_attributes_query_details PRIVATE ./)
target_link_libraries(csm_node_attributes_query_details csmi csm_network_c csmutil)

# cmd line interface for node_attributes_query_history
add_executable(csm_node_attributes_query_history node_attributes_query_history.c)
install(TARGETS csm_node_attributes_query_history COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_attributes_query_history PRIVATE ./)
target_link_libraries(csm_node_attributes_query_history csmi csm_network_c csmutil)

# cmd line interface for node_query_state_history
add_executable(csm_node_query_state_history node_query_state_history.c)
install(TARGETS csm_node_query_state_history COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_query_state_history PRIVATE ./)
target_link_libraries(csm_node_query_state_history csmi csm_network_c csmutil)

# cmd line interface for node_attributes_update
add_executable(csm_node_attributes_update node_attributes_update.c)
install(TARGETS csm_node_attributes_update COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_attributes_update PRIVATE ./)
target_link_libraries(csm_node_attributes_update csmi csm_network_c csmutil fsutil)

# cmd line interface for node_delete
add_executable(csm_node_delete node_delete.c)
install(TARGETS csm_node_delete COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_delete PRIVATE ./)
target_link_libraries(csm_node_delete csmi csm_network_c csmutil fsutil)

# cmd line interface for node_find_job
add_executable(csm_node_find_job node_find_job.c)
install(TARGETS csm_node_find_job COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_node_find_job PRIVATE ./)
target_link_libraries(csm_node_find_job csmi csm_network_c csmutil fsutil)

# cmd line interface for switch_attributes_query
add_executable(csm_switch_attributes_query switch_attributes_query.c)
install(TARGETS csm_switch_attributes_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_switch_attributes_query PRIVATE ./)
target_link_libraries(csm_switch_attributes_query csmi csm_network_c csmutil)

# cmd line interface for switch_attributes_query_details
add_executable(csm_switch_attributes_query_details switch_attributes_query_details.c)
install(TARGETS csm_switch_attributes_query_details COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_switch_attributes_query_details PRIVATE ./)
target_link_libraries(csm_switch_attributes_query_details csmi csm_network_c csmutil)

# cmd line interface for switch_attributes_query_history
add_executable(csm_switch_attributes_query_history switch_attributes_query_history.c)
install(TARGETS csm_switch_attributes_query_history COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_switch_attributes_query_history PRIVATE ./)
target_link_libraries(csm_switch_attributes_query_history csmi csm_network_c csmutil)

# cmd line interface for switch_attributes_update
add_executable(csm_switch_attributes_update switch_attributes_update.c)
install(TARGETS csm_switch_attributes_update COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_switch_attributes_update PRIVATE ./)
target_link_libraries(csm_switch_attributes_update csmi csm_network_c csmutil fsutil)

add_definitions(-DUSE_SC_LOGGER)
