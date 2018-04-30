file(GLOB file_source *.cc)
add_executable(fshipd  ${file_source})
#add_executable(fshipd fshipd.cc MessageHandler.cc )
flightgen(fshipd fshipd_flightlog.h)
flightlib(fshipd fsutil)
add_dependencies(fshipd txp)
add_dependencies(fshipd fsutil)

#SET(CMAKE_CXX_COMPILER /path/to/cpp/compiler)
#SET(CMAKE_C_COMPILER /path/to/c/compiler)

target_compile_definitions(fshipd PRIVATE -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=26 -DUSE_SC_LOGGER=1)
target_link_libraries(fshipd fsutil -lpthread)
target_link_libraries(fshipd txp)

flightlib(fshipd txp)
install(TARGETS fshipd COMPONENT fshipd DESTINATION bin)

include_directories("${CMAKE_BASE_BINARY_DIR}/transport/src"
                    "${CMAKE_BASE_BINARY_DIR}/transport/include")
