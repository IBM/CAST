#================================================================================
#
#    csmd/src/utils/src/CMakeLists.txt
#
#    © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

set(CSM_SMT_SRC
    CSMSMT.cc    
    ../../daemon/src/csmi_request_handler/helpers/cgroup.cc
    ../../daemon/src/csmi_request_handler/helpers/Agent.cc
)
add_definitions(-DUSE_SC_LOGGER)
    
add_executable(csm_smt ${CSM_SMT_SRC})
target_link_libraries(csm_smt fsutil csmi_serialization csmutil ${Boost_LIBRARIES} -lpthread -lpq)
install(TARGETS csm_smt COMPONENT csm-core DESTINATION csm/bin)




