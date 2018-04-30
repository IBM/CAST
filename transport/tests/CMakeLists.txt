#add_executable(allTests allTests.cc)
#add_executable(txpClient txpClient.cc)
#add_executable(txpClient2 txpClient2.cc)
#add_executable(txpClient3 txpClient3.cc)
#add_executable(txpClient4 txpClient4.cc)
#add_executable(txpClient5 txpClient5.cc)
#add_executable(txpServer txpServer.cc)
#add_executable(txpServer2 txpServer2.cc)

#add_dependencies(allTests txp)
#add_dependencies(txpClient txp)
#add_dependencies(txpClient2 txp)
#add_dependencies(txpClient3 txp)
#add_dependencies(txpClient4 txp)
#add_dependencies(txpClient5 txp)
#add_dependencies(txpServer txp)
#add_dependencies(txpServer2 txp)

#target_link_libraries(allTests txp)
#target_link_libraries(txpClient txp)
#target_link_libraries(txpClient2 txp)
#target_link_libraries(txpClient3 txp)
#target_link_libraries(txpClient4 txp)
#target_link_libraries(txpClient5 txp)
#target_link_libraries(txpServer txp)
#target_link_libraries(txpServer2 txp)

#target_compile_definitions(allTests PRIVATE -DUSE_SC_LOGGER=1)
#target_compile_definitions(txpClient PRIVATE -DUSE_SC_LOGGER=1)
#target_compile_definitions(txpClient2 PRIVATE -DUSE_SC_LOGGER=1)
#target_compile_definitions(txpClient3 PRIVATE -DUSE_SC_LOGGER=1)
#target_compile_definitions(txpClient4 PRIVATE -DUSE_SC_LOGGER=1)
#target_compile_definitions(txpClient5 PRIVATE -DUSE_SC_LOGGER=1)
#target_compile_definitions(txpServer PRIVATE -DUSE_SC_LOGGER=1)
#target_compile_definitions(txpServer2 PRIVATE -DUSE_SC_LOGGER=1)

include_directories("${BASEDIR}/transport/include"
                    "${CMAKE_BASE_BINARY_DIR}/transport/src"
                    "${CMAKE_BASE_BINARY_DIR}/transport/include")

#install(TARGETS allTests txpClient txpClient2 txpClient3 txpClient4 txpServer txpServer2
#install(TARGETS allTests
#        COMPONENT transport DESTINATION transport/tests/bin)
