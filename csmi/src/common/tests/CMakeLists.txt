#================================================================================
#
#    csmi/src/common/tests/CMakeLists.txt
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

# test cases for serialization
add_executable(test_csmi_err_serialization test_csmi_err_serialization.c)
install(TARGETS test_csmi_err_serialization COMPONENT csm-unittest DESTINATION csm/tests/common)
target_link_libraries(test_csmi_err_serialization csmi_serialization csmutil)
add_test(test_csmi_err_serialization test_csmi_err_serialization)

# test case for csmi <-> master (has to start server manually)
add_executable(test_csmi_corner_cases_cli test_csmi_cli.c test_csmi_corner_cases.c)
install(TARGETS test_csmi_corner_cases_cli COMPONENT csm-unittest DESTINATION csm/tests/common)
target_include_directories(test_csmi_corner_cases_cli PRIVATE ./)
target_link_libraries(test_csmi_corner_cases_cli csmi csm_network_c csmutil)

# test case for csmi <-> master (has server and client)
#add_executable(test_csmi_corner_cases_srv_cli test_csmi_srv_cli.cc csmi_srv_cli_common.cc test_csmi_corner_cases.c)
#install(TARGETS test_csmi_corner_cases_srv_cli COMPONENT csm-unittest DESTINATION csm/tests/common)
#target_include_directories(test_csmi_corner_cases_srv_cli PRIVATE ./
#  "${CMAKE_SOURCE_DIR}/csmnet/include"
#  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
#  ${CSMD_INCLUDE_DIRS}
#)
#target_link_libraries(test_csmi_corner_cases_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES} -lpq -lpthread)

# Test case to call csmi_init_lib in a loop while receiving signals from another thread 
add_executable(test_signal_interrupt test_signal_interrupt.cc)
install(TARGETS test_signal_interrupt COMPONENT csm-unittest DESTINATION csm/tests/common)
target_include_directories(test_signal_interrupt PRIVATE ./)
target_link_libraries(test_signal_interrupt csmi csm_network_c csmutil -lpthread)


# Collection of test cases that do not always follow the protocol, requires a daemon to run
add_executable(test_daemon_interaction test_daemon_interaction.cc)
install(TARGETS test_daemon_interaction COMPONENT csm-unittest DESTINATION csm/tests/common)
target_link_libraries(test_daemon_interaction csmi csm_network_c csmutil -lpthread)



#add_test(test_csmi_corner_cases_srv_cli test_csmi_corner_cases_srv_cli)

add_definitions(-DUSE_SC_LOGGER)
