file(GLOB file_source *.cc)
add_executable(fshipcld  ${file_source})
flightgen(fshipcld fshipcld_flightlog.h)
add_dependencies(fshipcld txp)

#SET(CMAKE_CXX_COMPILER /path/to/cpp/compiler)
#SET(CMAKE_C_COMPILER /path/to/c/compiler)

target_compile_definitions(fshipcld PRIVATE -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DFUSE_USE_VERSION=26 -DUSE_SC_LOGGER=1)
#set(CMAKE_CXX_FLAGS "-g -O2 -Wall -Werror -m64 -Wl,-z,muldefs ")

target_link_libraries(fshipcld -lpthread)
target_link_libraries(fshipcld txp)

flightlib(fshipcld txp)
install(TARGETS fshipcld COMPONENT fshipcld DESTINATION bin)

include_directories("${CMAKE_BASE_BINARY_DIR}/transport/src"
                    "${CMAKE_BASE_BINARY_DIR}/transport/include")
