#================================================================================
#
#    csmi/src/ras/tests/CMakeLists.txt
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
# test case for csmi <-> master

#ToDo: move the RAS cmd lines out of here.

# COMMENTED so that not build binary for SW33
# add_executable(csm_ras_msg_type_get csm_ras_msg_type_get.c)
# install(TARGETS csm_ras_msg_type_get COMPONENT csm-api DESTINATION csm/bin)
# target_include_directories(csm_ras_msg_type_get PRIVATE ./)
# target_link_libraries(csm_ras_msg_type_get csmi csm_network_c csmutil)

# COMMENTED so that not build binary for SW33
# add_executable(csm_ras_event_subscribe csm_ras_event_subscribe.c)
# install(TARGETS csm_ras_event_subscribe COMPONENT csm-api DESTINATION csm/bin)
# target_include_directories(csm_ras_event_subscribe PRIVATE ./)
# target_link_libraries(csm_ras_event_subscribe csmi csm_network_c csmutil)

add_executable(test_csmi_ras_event_create test_csmi_ras_event_create.c)
install(TARGETS test_csmi_ras_event_create COMPONENT csm-unittest DESTINATION csm/tests/ras)
target_include_directories(test_csmi_ras_event_create PRIVATE ./)
target_link_libraries(test_csmi_ras_event_create csmi csm_network_c csmutil)

# ToDo: Temp comment out. I think eventually delete?
#add_executable(test_csmi_ras_event_query test_csmi_ras_event_query.c)
#install(TARGETS test_csmi_ras_event_query COMPONENT csm-unittest DESTINATION csm/tests/ras)
#target_include_directories(test_csmi_ras_event_query PRIVATE ./)
#target_link_libraries(test_csmi_ras_event_query csmi csm_network_c csmutil)

# ToDo: Temp comment out. I think eventually delete?
# add_executable(test_csmi_ras_msg_type test_csmi_ras_msg_type.c)
# install(TARGETS test_csmi_ras_msg_type COMPONENT csm-unittest DESTINATION csm/tests/ras)
# target_include_directories(test_csmi_ras_msg_type PRIVATE ./)
# target_link_libraries(test_csmi_ras_msg_type csmi csm_network_c csmutil)

add_executable(test_csmi_ras_subpub test_csmi_ras_subpub.c)
install(TARGETS test_csmi_ras_subpub COMPONENT csm-unittest DESTINATION csm/tests/ras)
target_include_directories(test_csmi_ras_subpub PRIVATE ./)
target_link_libraries(test_csmi_ras_subpub csmi csm_network_c csmutil)

