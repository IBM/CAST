#
add_executable(polltest polltest.cc)

#SET(CMAKE_CXX_COMPILER /path/to/cpp/compiler)
#SET(CMAKE_C_COMPILER /path/to/c/compiler)

target_compile_definitions(polltest PRIVATE -D_FILE_OFFSET_BITS=64 -D_REENTRANT)
#set(CMAKE_CXX_FLAGS "-g -O2 -Wall -Werror -m64 -Wl,-z,muldefs ")

install(TARGETS polltest COMPONENT fshipcld DESTINATION test)

