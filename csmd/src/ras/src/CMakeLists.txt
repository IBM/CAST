#================================================================================
#
#    csmd/src/ras/src/CMakeLists.txt
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
file(GLOB csmras_SRC
    "*.cc"
    "*.c"
)

# tell the logger to use the subcomponenet logger.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SC_LOGGER")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_SC_LOGGER")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_SYSTEM_NO_DEPRECATED")

add_definitions(-DUSE_SC_LOGGER -DBOOST_SYSTEM_NO_DEPRECATED)
add_library(csmras SHARED ${csmras_SRC})

install(TARGETS csmras COMPONENT csm-core DESTINATION csm/lib)
