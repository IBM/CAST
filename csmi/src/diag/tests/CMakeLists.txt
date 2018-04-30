#================================================================================
#
#    csmi/src/diag/tests/CMakeLists.txt
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

#csm_diag_result_create
add_executable(test_csmi_diag_result_create_cli ../../common/tests/test_csmi_cli.c test_csmi_diag_result_create.c)
install(TARGETS test_csmi_diag_result_create_cli COMPONENT csm-unittest DESTINATION csm/tests/diag)
target_include_directories(test_csmi_diag_result_create_cli PRIVATE ./)
target_link_libraries(test_csmi_diag_result_create_cli csmi csm_network_c csmutil)
#csm_diag_run_begin
add_executable(test_csmi_diag_run_begin_cli ../../common/tests/test_csmi_cli.c test_csmi_diag_run_begin.c)
install(TARGETS test_csmi_diag_run_begin_cli COMPONENT csm-unittest DESTINATION csm/tests/diag)
target_include_directories(test_csmi_diag_run_begin_cli PRIVATE ./)
target_link_libraries(test_csmi_diag_run_begin_cli csmi csm_network_c csmutil)
#csm_diag_run_end
add_executable(test_csmi_diag_run_end_cli ../../common/tests/test_csmi_cli.c test_csmi_diag_run_end.c)
install(TARGETS test_csmi_diag_run_end_cli COMPONENT csm-unittest DESTINATION csm/tests/diag)
target_include_directories(test_csmi_diag_run_end_cli PRIVATE ./)
target_link_libraries(test_csmi_diag_run_end_cli csmi csm_network_c csmutil)
#csm_diag_run_query
add_executable(test_csmi_diag_run_query_cli ../../common/tests/test_csmi_cli.c test_csmi_diag_run_query.c)
install(TARGETS test_csmi_diag_run_query_cli COMPONENT csm-unittest DESTINATION csm/tests/diag)
target_include_directories(test_csmi_diag_run_query_cli PRIVATE ./)
target_link_libraries(test_csmi_diag_run_query_cli csmi csm_network_c csmutil)
#csm_diag_run_query_details
add_executable(test_csmi_diag_run_query_details_cli ../../common/tests/test_csmi_cli.c test_csmi_diag_run_query_details.c)
install(TARGETS test_csmi_diag_run_query_details_cli COMPONENT csm-unittest DESTINATION csm/tests/diag)
target_include_directories(test_csmi_diag_run_query_details_cli PRIVATE ./)
target_link_libraries(test_csmi_diag_run_query_details_cli csmi csm_network_c csmutil)
