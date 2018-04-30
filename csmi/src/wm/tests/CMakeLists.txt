#================================================================================
#
#    csmi/src/wm/tests/CMakeLists.txt
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


# test case for csmi <-> master (has to start server manually)
add_executable(test_csmi_allocation_create_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_create.c)
install(TARGETS test_csmi_allocation_create_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_create_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_create_cli csmi csm_network_c csmutil)
add_executable(test_csmi_allocation_delete_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_delete.c)
install(TARGETS test_csmi_allocation_delete_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_delete_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_delete_cli csmi csm_network_c csmutil)
add_executable(test_csmi_allocation_query_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_query.c)
install(TARGETS test_csmi_allocation_query_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_query_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_query_cli csmi csm_network_c csmutil)
add_executable(test_csmi_allocation_query_details_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_query_details.c)
install(TARGETS test_csmi_allocation_query_details_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_query_details_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_query_details_cli csmi csm_network_c csmutil)

#csm_allocation_step_begin
add_executable(test_csmi_allocation_step_begin_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_step_begin.c)
install(TARGETS test_csmi_allocation_step_begin_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_step_begin_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_step_begin_cli csmi csm_network_c csmutil)
#csm_allocation_step_end
add_executable(test_csmi_allocation_step_end_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_step_end.c)
install(TARGETS test_csmi_allocation_step_end_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_step_end_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_step_end_cli csmi csm_network_c csmutil)
#csm_allocation_step_query
add_executable(test_csmi_allocation_step_query_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_step_query.c)
install(TARGETS test_csmi_allocation_step_query_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_step_query_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_step_query_cli csmi csm_network_c csmutil)
#csm_allocation_step_query_active_all
add_executable(test_csmi_allocation_step_query_active_all_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_step_query_active_all.c)
install(TARGETS test_csmi_allocation_step_query_active_all_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_step_query_active_all_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_step_query_active_all_cli csmi csm_network_c csmutil)
#csm_allocation_step_query_details
add_executable(test_csmi_allocation_step_query_details_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_step_query_details.c)
install(TARGETS test_csmi_allocation_step_query_details_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_step_query_details_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_step_query_details_cli csmi csm_network_c csmutil)

#csm_allocation_step_query_details
add_executable(test_csmi_sample_cli ../../common/tests/test_csmi_cli.c test_csmi_sample.c)
install(TARGETS test_csmi_sample_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_sample_cli PRIVATE ./)
target_link_libraries(test_csmi_sample_cli csmi csm_network_c csmutil)

# test case for csmi <-> master (has server and client)
#add_executable(test_csmi_allocation_create_srv_cli ../../common/tests/test_csmi_srv_cli.cc ../../common/tests/csmi_srv_cli_common.cc test_csmi_allocation_create.c)
#install(TARGETS test_csmi_allocation_create_srv_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
#target_include_directories(test_csmi_allocation_create_srv_cli PRIVATE ./
#  "${CMAKE_SOURCE_DIR}/csmnet/include"
#  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
#  ${CSMD_INCLUDE_DIRS}
#)
#add_executable(test_csmi_allocation_delete_srv_cli ../../common/tests/test_csmi_srv_cli.cc ../../common/tests/csmi_srv_cli_common.cc test_csmi_allocation_delete.c)
#install(TARGETS test_csmi_allocation_delete_srv_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
#target_include_directories(test_csmi_allocation_delete_srv_cli PRIVATE ./
#  "${CMAKE_SOURCE_DIR}/csmnet/include"
#  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
#  ${CSMD_INCLUDE_DIRS}
#)
#add_executable(test_csmi_allocation_query_srv_cli ../../common/tests/test_csmi_srv_cli.cc ../../common/tests/csmi_srv_cli_common.cc test_csmi_allocation_query.c)
#install(TARGETS test_csmi_allocation_query_srv_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
#target_include_directories(test_csmi_allocation_query_srv_cli PRIVATE ./
#  "${CMAKE_SOURCE_DIR}/csmnet/include"
#  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
#  ${CSMD_INCLUDE_DIRS}
#)
#target_link_libraries(test_csmi_allocation_create_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES}  -lpq -lpthread)
#target_link_libraries(test_csmi_allocation_delete_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES} -lpq -lpthread)
#target_link_libraries(test_csmi_allocation_query_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES} -lpq -lpthread)

# add_test(test_csmi_allocation_create_srv_cli test_csmi_allocation_create_srv_cli -f ${CMAKE_INSTALL_PREFIX}/bin/coral.cfg)
# add_test(test_csmi_allocation_delete_srv_cli test_csmi_allocation_delete_srv_cli -f ${CMAKE_INSTALL_PREFIX}/bin/coral.cfg)
# add_test(test_csmi_allocation_query_srv_cli test_csmi_allocation_query_srv_cli -f ${CMAKE_INSTALL_PREFIX}/bin/coral.cfg)

#add_executable(test_csmi_allocation_query_details_srv_cli ../../common/tests/test_csmi_srv_cli.cc ../../common/tests/csmi_srv_cli_common.cc test_csmi_allocation_query_details.c)
#install(TARGETS test_csmi_allocation_query_details_srv_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
#target_include_directories(test_csmi_allocation_query_details_srv_cli PRIVATE ./
#  "${CMAKE_SOURCE_DIR}/csmnet/include"
#  "${CMAKE_SOURCE_DIR}/csmdb/api/include"
#  ${CSMD_INCLUDE_DIRS}
#)
#target_link_libraries(test_csmi_allocation_create_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES}  -lpq -lpthread)
#target_link_libraries(test_csmi_allocation_delete_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES} -lpq -lpthread)
#target_link_libraries(test_csmi_allocation_query_details_srv_cli csmi csmd_lib csm_network_cc csm_network_c fsutil csmutil csmras ${Boost_LIBRARIES} -lpq -lpthread)

# add_test(test_csmi_allocation_create_srv_cli test_csmi_allocation_create_srv_cli -f ${CMAKE_INSTALL_PREFIX}/bin/coral.cfg)
# add_test(test_csmi_allocation_delete_srv_cli test_csmi_allocation_delete_srv_cli -f ${CMAKE_INSTALL_PREFIX}/bin/coral.cfg)
# add_test(test_csmi_allocation_query_details_srv_cli test_csmi_allocation_query_details_srv_cli -f ${CMAKE_INSTALL_PREFIX}/bin/coral.cfg)

#csm_allocation_query_active_all
add_executable(test_csmi_allocation_query_active_all_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_query_active_all.c)
install(TARGETS test_csmi_allocation_query_active_all_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_query_active_all_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_query_active_all_cli csmi csm_network_c csmutil)

add_executable(test_csmi_allocation_step_cgroup_create_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_step_cgroup_create.c)
install(TARGETS test_csmi_allocation_step_cgroup_create_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_step_cgroup_create_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_step_cgroup_create_cli csmi csm_network_c csmutil)

#csmi_allocation_step_cgroup_delete
add_executable(test_csmi_allocation_step_cgroup_delete_cli ../../common/tests/test_csmi_cli.c test_csmi_allocation_step_cgroup_delete.c)
install(TARGETS test_csmi_allocation_step_cgroup_delete_cli COMPONENT csm-unittest DESTINATION csm/tests/wm)
target_include_directories(test_csmi_allocation_step_cgroup_delete_cli PRIVATE ./)
target_link_libraries(test_csmi_allocation_step_cgroup_delete_cli csmi csm_network_c csmutil)

add_definitions(-DUSE_SC_LOGGER)
