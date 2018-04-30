#================================================================================
#
#    csmi/src/bb/tests/CMakeLists.txt
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

#csm_bb_cmd
add_executable(test_csmi_bb_cmd_cli ../../common/tests/test_csmi_cli.c test_csmi_bb_cmd.c)
install(TARGETS test_csmi_bb_cmd_cli COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(test_csmi_bb_cmd_cli PRIVATE ./)
target_link_libraries(test_csmi_bb_cmd_cli csmi csm_network_c csmutil)
#csm_bb_cmd (with a bad CMD name)
add_executable(test_csmi_bb_cmd_badCMDname_cli ../../common/tests/test_csmi_cli.c test_csmi_bb_cmd_badCMDname.c)
install(TARGETS test_csmi_bb_cmd_badCMDname_cli COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(test_csmi_bb_cmd_badCMDname_cli PRIVATE ./)
target_link_libraries(test_csmi_bb_cmd_badCMDname_cli csmi csm_network_c csmutil)
#csm_bb_lv_create
add_executable(test_csmi_bb_lv_create_cli ../../common/tests/test_csmi_cli.c test_csmi_bb_lv_create.c)
install(TARGETS test_csmi_bb_lv_create_cli COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(test_csmi_bb_lv_create_cli PRIVATE ./)
target_link_libraries(test_csmi_bb_lv_create_cli csmi csm_network_c csmutil)
#csm_bb_lv_delete
add_executable(test_csmi_bb_lv_delete_cli ../../common/tests/test_csmi_cli.c test_csmi_bb_lv_delete.c)
install(TARGETS test_csmi_bb_lv_delete_cli COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(test_csmi_bb_lv_delete_cli PRIVATE ./)
target_link_libraries(test_csmi_bb_lv_delete_cli csmi csm_network_c csmutil)
#csm_bb_lv_update
add_executable(test_csmi_bb_lv_update_cli ../../common/tests/test_csmi_cli.c test_csmi_bb_lv_update.c)
install(TARGETS test_csmi_bb_lv_update_cli COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(test_csmi_bb_lv_update_cli PRIVATE ./)
target_link_libraries(test_csmi_bb_lv_update_cli csmi csm_network_c csmutil)
#csm_bb_lv_update_2
add_executable(test_csmi_bb_lv_update_2_cli ../../common/tests/test_csmi_cli.c test_csmi_bb_lv_update_2.c)
install(TARGETS test_csmi_bb_lv_update_2_cli COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(test_csmi_bb_lv_update_2_cli PRIVATE ./)
target_link_libraries(test_csmi_bb_lv_update_2_cli csmi csm_network_c csmutil)
#csm_bb_vg_create
add_executable(test_csmi_bb_vg_create_cli ../../common/tests/test_csmi_cli.c test_csmi_bb_vg_create.c)
install(TARGETS test_csmi_bb_vg_create_cli COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(test_csmi_bb_vg_create_cli PRIVATE ./)
target_link_libraries(test_csmi_bb_vg_create_cli csmi csm_network_c csmutil)