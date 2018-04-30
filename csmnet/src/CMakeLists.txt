#================================================================================
#
#    csmnet/src/CMakeLists.txt
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
enable_testing()

set(CSM_NETWORK_CCLIB_SOURCES
  CPP/csm_version_msg.cc
  CPP/endpoint_unix.cc
  CPP/endpoint_dual_unix.cc
  CPP/endpoint_ptp.cc
  CPP/endpoint_ptp_plain.cc
  CPP/endpoint_ptp_sec.cc
  CPP/endpoint_utility.cc
  CPP/endpoint_aggregator.cc
  CPP/endpoint_multi_unix.cc
  CPP/csm_network_msg_cpp.cc
  CPP/csm_multicast_message.cc
  CPP/network_ctrl_path.cc
  CPP/multi_endpoint.cc
  CPP/reliable_msg.cc
)

set(CSM_NETWORK_CLIB_SOURCES
  C/csm_network_local.c
)


add_definitions(-DUSE_SC_LOGGER)
add_definitions(-D_GNU_SOURCE)

# define object lists to avoid double compile for shared and static libs
add_library(csm_network_cc_objs OBJECT ${CSM_NETWORK_CCLIB_SOURCES})
add_library(csm_network_c_objs OBJECT ${CSM_NETWORK_CLIB_SOURCES})

add_dependencies(csm_network_cc_objs need_csm_version )
add_dependencies(csm_network_c_objs need_csm_version )

# set PIC for shared libraries
set_property(TARGET ${csm_network_cc_objs} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${csm_network_c_objs} PROPERTY POSITION_INDEPENDENT_CODE 1)

# define static libs
add_library(csm_network_cc STATIC $<TARGET_OBJECTS:csm_network_cc_objs>)
add_library(csm_network_c STATIC $<TARGET_OBJECTS:csm_network_c_objs>)

add_dependencies(csm_network_cc need_csm_version )
add_dependencies(csm_network_c need_csm_version )

# define shared libs
add_library(csm_network_cc_shared SHARED $<TARGET_OBJECTS:csm_network_cc_objs>)
add_library(csm_network_c_shared SHARED $<TARGET_OBJECTS:csm_network_c_objs>)

add_dependencies(csm_network_cc_shared need_csm_version )
add_dependencies(csm_network_c_shared need_csm_version )

# rename the output names of shared libs
set_target_properties(csm_network_cc_shared PROPERTIES OUTPUT_NAME csm_network_cc)
set_target_properties(csm_network_c_shared PROPERTIES OUTPUT_NAME csm_network_c)


#add_dependencies(csm_network_cc mosquitto_make )
target_link_libraries(csm_network_cc csmutil csmi_serialization fsutil)
target_link_libraries(csm_network_c csmutil csmi_serialization pthread)

if(CSMNET_SECURE_COMMUNICATION)
	target_link_libraries(csm_network_cc ssl crypto)
	target_link_libraries(csm_network_c ssl crypto)
endif(CSMNET_SECURE_COMMUNICATION)


#install(TARGETS csm_network_cc COMPONENT csmnet DESTINATION csmnet/lib)
#install(TARGETS csm_network_c COMPONENT csmnet DESTINATION csmnet/lib)
#install(TARGETS csm_network_cc_shared COMPONENT csmnet DESTINATION csmnet/lib)
#install(TARGETS csm_network_c_shared COMPONENT csmnet DESTINATION csmnet/lib)
