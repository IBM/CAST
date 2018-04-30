#================================================================================
#
#    csmd/src/daemon/src/CMakeLists.txt
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

# Include the CSM_INVENTORY_SRC files 
include (${CMAKE_SOURCE_DIR}/csmd/src/inv/src/CMakeLists.txt)
# Include the CSM_DB_SRC files
include (${CMAKE_SOURCE_DIR}/csmd/src/db/src/db.cmake)

set(CSM_DAEMON_LIB_SRC
  csm_retry_backoff.cc
  message_control.cc
  thread_pool.cc
  csm_daemon_config.cc
  csm_daemon_state.cc
  connection_handling.cc
  csm_daemon_network_manager.cc
  ${CSM_INVENTORY_SRC}
  ${CSM_DB_SRC}

  csm_event_source_set.cc
)

file(GLOB CSM_DAEMON_SRC
  ${CSM_DAEMON_LIB_SRC}
  csm_daemon.cc
  csm_daemon_main.cc
  csm_daemon_core.cc
  csm_daemon_core_master.cc
  csm_daemon_core_utility.cc
  csm_daemon_core_agent.cc
  csm_daemon_core_aggregator.cc
  csm_event_sink_set.cc
  csm_timer_manager.cc
  csm_db_manager.cc
  csm_event_routing_master.cc
  csm_event_routing_agg.cc
  csm_event_routing_utility.cc
  csm_event_routing_agent.cc
  csmi_request_handler/CSMIAllocationQuery.cc
  csmi_request_handler/CSMIAllocationQueryDetails.cc
  csmi_request_handler/CSMIAllocationResourcesQuery.cc
  csmi_request_handler/CSMIAllocationStepBegin.cc
  csmi_request_handler/CSMIAllocationStepCGROUPCreate.cc
  csmi_request_handler/CSMIAllocationStepCGROUPDelete.cc
  csmi_request_handler/CSMIAllocationStepEnd.cc
  csmi_request_handler/CSMIAllocationStepQuery.cc
  csmi_request_handler/CSMIAllocationStepQueryActiveAll.cc
  csmi_request_handler/CSMIAllocationStepQueryDetails.cc
  csmi_request_handler/CSMINodeResourcesQuery.cc
  csmi_request_handler/CSMINodeResourcesQueryAll.cc
  csmi_request_handler/CSMIAllocationQueryActiveAll.cc
  csmi_request_handler/CSMIAllocationUpdateHistory.cc
  csmi_request_handler/CSMIAllocationUpdateState.cc
  csmi_request_handler/CSMIBBLVCreate.cc
  csmi_request_handler/CSMIBBLVDelete.cc
  csmi_request_handler/CSMIBBLVQuery.cc
  csmi_request_handler/CSMIBBLVUpdate.cc
  csmi_request_handler/CSMIBBVGCreate.cc
  csmi_request_handler/CSMIBBVGDelete.cc
  csmi_request_handler/CSMIBBVGQuery.cc
  csmi_request_handler/CSMICGROUPLogin.cc
  csmi_request_handler/CSMIDiagResultCreate.cc
  csmi_request_handler/CSMIDiagRunBegin.cc
  csmi_request_handler/CSMIDiagRunEnd.cc
  csmi_request_handler/CSMIDiagRunQuery.cc
  csmi_request_handler/CSMIDiagRunQueryDetails.cc
  csmi_request_handler/CSMIIbCableInventoryCollection.cc
  csmi_request_handler/CSMIIbCableQuery.cc
  csmi_request_handler/CSMIIbCableQueryHistory.cc
  csmi_request_handler/CSMIIbCableUpdate.cc
  csmi_request_handler/CSMINodeAttributesUpdate.cc
  csmi_request_handler/CSMINodeAttributesQuery.cc
  csmi_request_handler/CSMINodeAttributesQueryDetails.cc
  csmi_request_handler/CSMINodeAttributesQueryHistory.cc
  csmi_request_handler/CSMINodeQueryStateHistory.cc
  csmi_request_handler/CSMINodeDelete.cc
  csmi_request_handler/CSMIRasEventCreate.cc
  csmi_request_handler/CSMIRasEventQuery.cc
  csmi_request_handler/CSMIRasMsgTypeCreate.cc
  csmi_request_handler/CSMIRasMsgTypeUpdate.cc
  csmi_request_handler/CSMIRasMsgTypeDelete.cc
  csmi_request_handler/CSMIRasMsgTypeQuery.cc
  csmi_request_handler/CSMIRasEventQueryAllocation.cc
  csmi_request_handler/CSMISwitchAttributesQuery.cc
  csmi_request_handler/CSMISwitchAttributesQueryDetails.cc
  csmi_request_handler/CSMISwitchAttributesQueryHistory.cc
  csmi_request_handler/CSMISwitchAttributesUpdate.cc
  csmi_request_handler/CSMISwitchInventoryCollection.cc
  csmi_request_handler/CSMISwitchChildrenInventoryCollection.cc
  csmi_request_handler/csmi_forward_handler.cc
  csmi_request_handler/csmi_error_handler.cc
  csmi_request_handler/csmi_db_base.cc
  csmi_request_handler/csmi_csmi_db_stateful.cc
  csmi_request_handler/csmi_agg_init_handler.cc
  csmi_request_handler/csmi_master_init_handler.cc
  csmi_request_handler/csmi_utility_init_handler.cc
  csmi_request_handler/csm_conn_ctrl_handler.cc
  csmi_request_handler/csmi_echo_handler.cc
  csmi_request_handler/csm_error_case_handler.cc
  csmi_request_handler/csm_infrastructure_test.cc
  csmi_request_handler/csm_infrastructure_test_master.cc
  csmi_request_handler/csm_infrastructure_test_agg.cc
  csmi_request_handler/csm_infrastructure_test_agent.cc
  csmi_request_handler/csm_infrastructure_test_utility.cc
  csmi_request_handler/csmi_agg_mtc_handler.cc
  csmi_request_handler/InvGetNodeInventory.cc
  csmi_request_handler/csm_ctrl_cmd_handler.cc
  csmi_request_handler/csm_daemon_clock.cc
  csmi_request_handler/csm_environmental_handler.cc
  csmi_request_handler/csm_envdata_handler.cc
  csmi_request_handler/csm_handler_options.cc
  csmi_request_handler/csmi_stateful.cc
  csmi_request_handler/csmi_handler_state.cc
  csmi_request_handler/csmi_network_message_state.cc
  csmi_request_handler/csmi_system_event_state.cc
  csmi_request_handler/csmi_db_resp_state.cc
  csmi_request_handler/helpers/EventHelpers.cc
  csmi_request_handler/helpers/cgroup.cc
  csmi_request_handler/helpers/Agent.cc
  csmi_request_handler/helpers/DataAggregators.cc
  csmi_request_handler/helpers/OCCSensorData.cc

  # Stateful Classes:
  csmi_request_handler/csmi_handler_context.cc
  csmi_request_handler/csmi_stateful_db.cc
  csmi_request_handler/csmi_stateful_db/CSMIStatefulDBInit.cc
  csmi_request_handler/csmi_stateful_db/CSMIStatefulDBRecvDB.cc
  csmi_request_handler/csmi_stateful_db/CSMIStatefulDBRecvPrivate.cc

  csmi_request_handler/csmi_stateful_forward/CSMIStatefulForwardRequest.cc
  csmi_request_handler/csmi_stateful_forward/CSMIStatefulForwardResponse.cc
  
  # Allocation Mcast States:
  csmi_request_handler/CSMIAllocationCreate.cc
  csmi_request_handler/CSMIAllocationDelete.cc

  # MCast files:
  csmi_request_handler/csmi_mcast/CSMIMcastAllocation.cc
  csmi_request_handler/csmi_mcast/CSMIMcastStep.cc
  csmi_request_handler/CSMIStepUpdateAgent.cc
  csmi_request_handler/CSMIAllocationAgentUpdateState.cc

  # BB CMD States:
  csmi_request_handler/CSMIBBCMD.cc
  csmi_request_handler/CSMIBBCMDAgentState.cc
  csmi_request_handler/csmi_mcast/CSMIMcastBB.cc

  # JSRUN
  csmi_request_handler/CSMIJSRUNCMD.cc
  csmi_request_handler/CSMIJSRUNCMDAgentState.cc
  csmi_request_handler/csmi_mcast/CSMIMcastJSRUN.cc
)

add_definitions(-DUSE_SC_LOGGER)

add_definitions(-DCSM_INFRASTRUCTURE_TEST_PARALLEL)

add_library(csmd_lib ${CSM_DAEMON_LIB_SRC})
add_executable(csmd ${CSM_DAEMON_SRC})

add_dependencies(csmd_lib ${CSMD_BUILD_DEPENDENCIES})

#uncommnet below if we want to debug Boost.Asio
#add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

add_dependencies(csmd ${CSMD_BUILD_DEPENDENCIES})

include_directories(
  "${CMAKE_SOURCE_DIR}/csmnet/include"
  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
  ${CSMD_INCLUDE_DIRS}
)

#target_include_directories(csmd PRIVATE ./ ${BASEDIR}/csmdb/api/include ${CMAKE_SOURCE_DIR}/csmd/src/)
target_link_libraries(csmd csm_network_cc fsutil csmi_serialization csmutil csmras ${Boost_LIBRARIES} -lpthread -lpq)

install(TARGETS csmd COMPONENT csm-core DESTINATION csm/sbin)


# Data aggregator tester : Remove in release.
#add_executable( csm_test_data_agg csmi_request_handler/helpers/DataAggTest.cc csmi_request_handler/helpers/DataAggregators.cc )
#add_dependencies(csm_test_data_agg ${CSMD_BUILD_DEPENDENCIES})
#
#target_include_directories( csm_test_data_agg PRIVATE ./ ${BASEDIR}/csmdb/api/include ${CMAKE_SOURCE_DIR}/csmd/src/ ${BASEDIR}/)
#
#target_link_libraries(csm_test_data_agg csm_network_cc fsutil csmi_serialization csmutil ${Boost_LIBRARIES} -lpthread -lpq)
#
#install(TARGETS csm_test_data_agg COMPONENT csm-core DESTINATION csm/bin)

#message("CSM_DAEMON_SRC: ${CSM_DAEMON_SRC}")

