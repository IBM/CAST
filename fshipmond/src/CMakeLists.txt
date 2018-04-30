file(GLOB file_source *.cc)
add_executable(fshipmond ${file_source})
flightgen(fshipmond fshipmond_flightlog.h)
flightlib(fshipmond fsutil)
add_dependencies(fshipmond txp)
add_dependencies(fshipmond fsutil)

#SET(CMAKE_CXX_COMPILER /path/to/cpp/compiler)
#SET(CMAKE_C_COMPILER /path/to/c/compiler)

target_compile_definitions(fshipmond PRIVATE -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=26 -DUSE_SC_LOGGER=1)
target_link_libraries(fshipmond fsutil -lpthread)
target_link_libraries(fshipmond txp)

flightlib(fshipmond txp)
install(TARGETS fshipmond COMPONENT fshipmond DESTINATION bin)

include_directories("${CMAKE_BASE_BINARY_DIR}/transport/src"
                    "${CMAKE_BASE_BINARY_DIR}/transport/include")
