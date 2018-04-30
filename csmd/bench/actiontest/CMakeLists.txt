#================================================================================
#
#    csmd/bench/actiontest/CMakeLists.txt
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
# obsolete tests..


file(GLOB actiontest_SRC
    "*.cc"
    "*.c"
)
### tell the logger to use the subcomponenet logger.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SC_LOGGER")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_SC_LOGGER")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_SYSTEM_NO_DEPRECATED")
##
###add_executable(rastest01 rastest01.cc csm_api_ras.c RasEvent.cc string_tokenizer.cc)
add_executable(actiontest ${actiontest_SRC})
##
##target_link_libraries(rastest01 csmras fsutil csmi csm_network_c csmutil pq -lpthread )
##
##
##


