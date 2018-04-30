add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(metadata)
add_subdirectory(tests)

##################################################################
#  Run script(s) for configuration
#
#  Build transport related includes/code
#

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_file(FUSE_HEADER fuse.h PATHS ${ALTINCLUDE}/linux)
ELSE()
    IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        find_file(FUSE_HEADER fuse.h PATHS /usr/include/linux)
    ELSE()
        find_file(FUSE_HEADER fuse.h PATHS /usr/include/linux)
    ENDIF()
ENDIF()

find_file(FSHIPCLD_HEADER fshipcld.h PATHS ${BASEDIR}/fshipcld/include)

add_library(txp_config txp_detect_header_change.cc)
add_custom_command(TARGET txp_config PRE_BUILD
				      COMMAND FSHIPCLD_HEADER=${FSHIPCLD_HEADER} FUSE_HEADER=${FUSE_HEADER} python txpParser.py ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}
				      WORKING_DIRECTORY ${BASEDIR}/transport/scripts)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -W -Wunused -Wcomment -Wparentheses -Wconversion -Wsign-compare -Wsign-conversion -Wundef")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -W -Wunused -Wcomment -Wparentheses -Wconversion -Wsign-compare -Wsign-conversion -Wundef")
