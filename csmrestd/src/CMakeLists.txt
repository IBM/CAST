#
# test bench for prototype restapi framework.
# using boost asio...
#
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SC_LOGGER")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_SC_LOGGER")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_SYSTEM_NO_DEPRECATED")

set(RESTAPI_SRC
     csmrestd.cc
     RestApiConnection.cc
     RestApiServer.cc
     RestApiReply.cc
     )



add_executable(csmrestd ${RESTAPI_SRC} )
install(TARGETS csmrestd COMPONENT csm-restd DESTINATION csm/sbin)
target_link_libraries(csmrestd ${Boost_LIBRARIES} csmi csm_network_c csmutil fsutil -lpthread -lssl -lcrypto)
target_include_directories(csmrestd PRIVATE ${PROJECT_SOURCE_DIR}/csmd/src/daemon/include )



