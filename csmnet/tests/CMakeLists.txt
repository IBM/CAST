#================================================================================
#
#    csmnet/tests/CMakeLists.txt
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
include_directories("../src")

#####################################################
# define and add test sources
set(CSM_NETWORK_TEST_CPP_SOURCES
  csm_network_header_test.cc
  csm_network_msg_test.cc
  csm_multicast_msg_test.cc
  csm_version_msg_test.cc
  endpoint_ptp_test.cc
  buffer_state_test.cc
  address_test.cc
  message_and_address_test.cc
  network_ctrl_path_test.cc
  message_ack_test.cc
  #csm_network_transmission_test.cc
  #endpoint_dual_unix_test.cc
  #multi_endpoint_test.cc 
)

if(CSMNET_SECURE_COMMUNICATION)
	list(APPEND CSM_NETWORK_TEST_CPP_SOURCES
		)
endif(CSMNET_SECURE_COMMUNICATION)

foreach(_test ${CSM_NETWORK_TEST_CPP_SOURCES})
  get_filename_component(TEST_NAME ${_test} NAME_WE)
  add_executable(${TEST_NAME} ${_test})
  target_link_libraries(${TEST_NAME} csm_network_cc csmutil pthread)

if(CSMNET_SECURE_COMMUNICATION)
	target_link_libraries(${TEST_NAME} ssl crypto)
endif(CSMNET_SECURE_COMMUNICATION)

  add_test(CSM_NETWORK_${TEST_NAME} ${TEST_NAME} )
endforeach()


set(CSM_NETWORK_TEST_C_SOURCES
  csm_unix_client_test.c
)

foreach(_test ${CSM_NETWORK_TEST_C_SOURCES})
  get_filename_component(TEST_NAME_PRE ${_test} NAME_WE)
  set(TEST_NAME "${TEST_NAME_PRE}_c")
  add_executable(${TEST_NAME} ${_test})
  target_link_libraries(${TEST_NAME} csm_network_c csmutil pthread )
  add_test(CSM_NETWORK_${TEST_NAME} ${TEST_NAME} )
endforeach()


set(CSM_NETWORK_TEST_MIXED_SOURCES
  #csm_network_callback_test.cc
)

foreach(_test ${CSM_NETWORK_TEST_MIXED_SOURCES})
  get_filename_component(TEST_NAME ${_test} NAME_WE)
  add_executable(${TEST_NAME} ${_test})
  target_link_libraries(${TEST_NAME} csm_network_cc csm_network_c csmutil pthread)
  add_test(CSM_NETWORK_${TEST_NAME} ${TEST_NAME} )
endforeach()

add_definitions(-DUSE_SC_LOGGER)

############################################################
# define executables that are not included the build tests
set(CSM_NETWORK_EXCLUDE_TEST_CPP_SOURCES
#  endpoint_performance_test.cc
  versioning.cc
  epoll_wrapper_test.cc
  multi_endpoint_ptp_stress_test.cc
  csmi_timeouts_test.cc
)

foreach(_excl_test ${CSM_NETWORK_EXCLUDE_TEST_CPP_SOURCES})
  get_filename_component(TEST_NAME ${_excl_test} NAME_WE)
  add_executable(${TEST_NAME} ${_excl_test})
  target_link_libraries(${TEST_NAME} csm_network_cc csm_network_c csmutil csmi csmi_serialization ${MOSQUITTO_LIBS} pthread pq )
endforeach()


