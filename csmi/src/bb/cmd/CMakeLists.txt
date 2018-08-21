#================================================================================
#
#    csmi/src/bb/cmd/CMakeLists.txt
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

#csm_bb_lv_create
add_executable(csm_bb_lv_create bb_lv_create.c)
install(TARGETS csm_bb_lv_create COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_bb_lv_create PRIVATE ./)
target_link_libraries(csm_bb_lv_create csmi csm_network_c csmutil fsutil)

# cmd line interface for csm_bb_lv_delete
add_executable(csm_bb_lv_delete bb_lv_delete.c)
install(TARGETS csm_bb_lv_delete COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_bb_lv_delete PRIVATE ./)
target_link_libraries(csm_bb_lv_delete csmi csm_network_c csmutil)

# cmd line interface for csm_bb_lv_query
add_executable(csm_bb_lv_query bb_lv_query.c)
install(TARGETS csm_bb_lv_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_bb_lv_query PRIVATE ./)
target_link_libraries(csm_bb_lv_query csmi csm_network_c csmutil fsutil)

#csm_bb_vg_create
add_executable(csm_bb_vg_create bb_vg_create.c)
install(TARGETS csm_bb_vg_create COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_bb_vg_create PRIVATE ./)
target_link_libraries(csm_bb_vg_create csmi csm_network_c csmutil fsutil)

# cmd line interface for csm_bb_vg_delete
add_executable(csm_bb_vg_delete bb_vg_delete.c)
install(TARGETS csm_bb_vg_delete COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_bb_vg_delete PRIVATE ./)
target_link_libraries(csm_bb_vg_delete csmi csm_network_c csmutil fsutil)

# cmd line interface for csm_bb_vg_query
add_executable(csm_bb_vg_query bb_vg_query.c)
install(TARGETS csm_bb_vg_query COMPONENT csm-api DESTINATION csm/bin)
target_include_directories(csm_bb_vg_query PRIVATE ./)
target_link_libraries(csm_bb_vg_query csmi csm_network_c csmutil fsutil)

add_definitions(-DUSE_SC_LOGGER)

##PERFORMANCE TESTS


#csm_bb_lv_update
add_executable(fernando_csm_bb_lv_update fernando_bb_lv_update.c)
install(TARGETS fernando_csm_bb_lv_update COMPONENT csm-unittest DESTINATION csm/tests/bb)
target_include_directories(fernando_csm_bb_lv_update PRIVATE ./)
target_link_libraries(fernando_csm_bb_lv_update csmi csm_network_c csmutil fsutil)
