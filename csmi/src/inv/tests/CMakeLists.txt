#================================================================================
#
#    csmi/src/inv/tests/CMakeLists.txt
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

# test case for csmi <-> master (has to start server manually) UPDATE
add_executable(test_csmi_node_attributes_update_cli ../../common/tests/test_csmi_cli.c test_csmi_node_attributes_update.c)
install(TARGETS test_csmi_node_attributes_update_cli COMPONENT csm-unittest DESTINATION csm/tests/inv)
target_include_directories(test_csmi_node_attributes_update_cli PRIVATE ./)
target_link_libraries(test_csmi_node_attributes_update_cli csmi)

# test case for UPDATE csmi <-> master
#add_executable(test_csmi_node_attributes_update_srv_cli ../../common/tests/test_csmi_srv_cli.cc ../../common/tests/csmi_srv_cli_common.cc test_csmi_node_attributes_update.c)
#install(TARGETS test_csmi_node_attributes_update_srv_cli COMPONENT csm-unittest DESTINATION csm/tests/inv)
#target_include_directories(test_csmi_node_attributes_update_srv_cli PRIVATE ./
#  "${CMAKE_SOURCE_DIR}/csmnet/include"
#  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
#  ${CSMD_INCLUDE_DIRS}
#)
#target_link_libraries(test_csmi_node_attributes_update_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES}  -lpq -lpthread)
#add_test(test_csmi_node_attributes_update_srv_cli test_csmi_node_attributes_update_srv_cli)
add_definitions(-DUSE_SC_LOGGER)

# # test case for csmi <-> master (has to start server manually) QUERY
# add_executable(test_csmi_node_attributes_query_cli ../../common/tests/test_csmi_cli.c test_csmi_node_attributes_query.c)
# install(TARGETS test_csmi_node_attributes_query_cli COMPONENT csm-unittest DESTINATION csm/tests/inv)
# target_include_directories(test_csmi_node_attributes_query_cli PRIVATE ./)
# target_link_libraries(test_csmi_node_attributes_query_cli csmi)
# #add_test(test_csmi_node_attributes_query_cli test_csmi_node_attributes_query_cli)

# test program for node_attributes_query
add_executable(test_program_csm_node_attributes_query test_csmi_node_attributes_query.c)
install(TARGETS test_program_csm_node_attributes_query COMPONENT csm-unittest DESTINATION csm/tests/inv)
target_include_directories(test_program_csm_node_attributes_query PRIVATE ./)
target_link_libraries(test_program_csm_node_attributes_query csmi csm_network_c csmutil fsutil)



#csm_node_attributes_query_history
# add_executable(test_csmi_node_attributes_query_history_cli ../../common/tests/test_csmi_cli.c test_csmi_node_attributes_query_history.c)
# install(TARGETS test_csmi_node_attributes_query_history_cli COMPONENT csm-unittest DESTINATION csm/tests/inv)
# target_include_directories(test_csmi_node_attributes_query_history_cli PRIVATE ./)
# target_link_libraries(test_csmi_node_attributes_query_history_cli csmi csm_network_c csmutil)


# test case for QUERY csmi <-> master
#add_executable(test_csmi_node_attributes_query_srv_cli ../../common/tests/test_csmi_srv_cli.cc ../../common/tests/csmi_srv_cli_common.cc test_csmi_node_attributes_query.c)
#install(TARGETS test_csmi_node_attributes_query_srv_cli COMPONENT csm-unittest DESTINATION csm/tests/inv)
#target_include_directories(test_csmi_node_attributes_query_srv_cli PRIVATE ./
#  "${CMAKE_SOURCE_DIR}/csmnet/include"
#  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
#  ${CSMD_INCLUDE_DIRS}
#)
#target_link_libraries(test_csmi_node_attributes_query_srv_cli csmi csmd_lib csm_network_cc fsutil csmras ${Boost_LIBRARIES}  -lpq -lpthread)
#add_test(test_csmi_node_attributes_query_srv_cli test_csmi_node_attributes_query_srv_cli -f ${CMAKE_INSTALL_PREFIX}/bin/coral.cfg)
#add_definitions(-DUSE_SC_LOGGER)

# test case for csmi <-> master (has to start server manually) CMD LINE
add_executable(test_csmi_cmd_line_cli ../../common/tests/test_csmi_cli.c test_csmi_cmd_line.c)
install(TARGETS test_csmi_cmd_line_cli COMPONENT csm-unittest DESTINATION csm/tests/inv)
target_include_directories(test_csmi_cmd_line_cli PRIVATE ./)
target_link_libraries(test_csmi_cmd_line_cli csmi)

###################################################################################
### Below is the testing setup for the CSM infrastructure. Pleaes do not touch! ###
###################################################################################
#add_executable(test_infrastructure_test_serialization
#               test_infrastructure_test_serialization.c)

#target_link_libraries(test_infrastructure_test_serialization csmi_serialization csmutil)
#install(TARGETS test_infrastructure_test_serialization COMPONENT csmi DESTINATION csmi/tests/bin)
#add_test(test_infrastructure_test_serialization test_infrastructure_test_serialization)

#add_executable(csm_infrastructure_health_check
#               ../../common/tests/test_csmi_cli.c test_csm_infrastructure_test.c)
#install(TARGETS csm_infrastructure_health_check COMPONENT csmi DESTINATION csmi/tests/bin)
#target_include_directories(csm_infrastructure_health_check PRIVATE ./)
#target_link_libraries(csm_infrastructure_health_check csmi)
