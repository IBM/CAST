#================================================================================
#
#    csmd/src/daemon/tests/CMakeLists.txt
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

add_definitions(-DUSE_SC_LOGGER)

##############################################################
# define sources to create a lib instead of daemon executable
#
# todo: extract daemon objects into a static lib so that we can just include it for tests

include_directories(
  "${CMAKE_SOURCE_DIR}/csmnet/include"
  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
  ${CSMD_INCLUDE_DIRS}
 )

#####################################################
# define and add test sources
set(CSM_DAEMON_TEST_SOURCES
  csm_daemon_config_test.cc
  csm_event_test.cc
  #csm_network_engine_test.cc
  #csm_reliable_msg_test.cc
  csm_network_pool_test.cc
  csm_item_scheduler_test.cc
  csm_event_source_set_test.cc
  csm_retry_backoff_test.cc
  csm_timer_queue_test.cc
)

foreach(_test ${CSM_DAEMON_TEST_SOURCES})
  get_filename_component(TEST_NAME ${_test} NAME_WE)
  add_executable(${TEST_NAME} ${_test})
  add_dependencies(${TEST_NAME} csmd_lib ${CSMD_BUILD_DEPENDENCIES})
  target_link_libraries(${TEST_NAME} csmd_lib csm_network_cc csmi_serialization csmras csmutil fsutil -lpthread -lpq )
  add_test(CSM_DAEMON_${TEST_NAME} ${TEST_NAME} )
endforeach()


############################################################
# define executables that are not included the build tests
set(CSM_DAEMON_EXCLUDE_TEST_CPP_SOURCES
  csm_daemon_echo_test.cc
  csm_reliable_msg_test_client.cc
  csm_c_client_ack_test.cc
  csm_ctrl_cmd.cc
  csm_infrastructure_health_check.cc
  endpoint_performance_test.cc
  csm_nodeset_test.cc
  error_inject.cc
)

foreach(_excl_test ${CSM_DAEMON_EXCLUDE_TEST_CPP_SOURCES})
  get_filename_component(TEST_NAME ${_excl_test} NAME_WE)
  add_executable(${TEST_NAME} ${_excl_test})
  target_link_libraries(${TEST_NAME} csmd_lib csm_network_cc csm_network_c csmi_serialization csmi csmutil fsutil -lpthread -lpq )
endforeach()

install(TARGETS csm_ctrl_cmd COMPONENT csm-core DESTINATION csm/sbin)
install(TARGETS csm_infrastructure_health_check COMPONENT csm-api DESTINATION csm/bin)

############################################################
# define executables that are not included the build tests
set(CSM_DAEMON_EXCLUDE_TEST_C_SOURCES
  csm_daemon_echo_test.c
)

foreach(_excl_test ${CSM_DAEMON_EXCLUDE_TEST_C_SOURCES})
  get_filename_component(TEST_NAME_PRE ${_excl_test} NAME_WE)
  set(TEST_NAME "${TEST_NAME_PRE}_c")
  add_executable(${TEST_NAME} ${_excl_test})
  target_link_libraries(${TEST_NAME} csmd_lib csm_network_c fsutil -lpthread )
endforeach()

############################################################
# define executables that are not included the build tests

set(CSM_DAEMON_TEST_CPP_SOURCES_WITH_PARAM
  csm_api_acl_test.cc
  csm_bitmap_test.cc
)

foreach(_excl_test ${CSM_DAEMON_TEST_CPP_SOURCES_WITH_PARAM})
  get_filename_component(TEST_NAME ${_excl_test} NAME_WE)
  add_executable(${TEST_NAME} ${_excl_test})
  target_link_libraries(${TEST_NAME} csmd_lib csm_network_cc csmi_serialization ${MOSQUITTO_LIBS} -lpthread)
  add_test(${TEST_NAME} ${TEST_NAME} ${CMAKE_SOURCE_DIR}/csmconf/csm_api.acl)
endforeach()

########## Integration testing using shell script ############
# Node Inventory Testing with Agent, Agg, Master
#find_program(BASH_PROGRAM bash)
#if (BASH_PROGRAM)
#  add_test(node_inv_master_first ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/csm_node_inventory_master_first.sh ${CMAKE_INSTALL_PREFIX})
#  add_test(node_inv_agg_first ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/csm_node_inventory_agg_first.sh ${CMAKE_INSTALL_PREFIX})
#endif (BASH_PROGRAM)
