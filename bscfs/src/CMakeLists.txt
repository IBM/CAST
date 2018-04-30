set(FUSE_FOUND TRUE)
find_path(FUSE_HEADER_PATH  NAMES fuse_opt.h fuse.h PATHS /usr/include/linux /usr/include/linux/fuse)
IF(NOT FUSE_HEADER_PATH)
message(WARNING "Can't find fuse.h")
set(FUSE_FOUND FALSE)
ENDIF(NOT FUSE_HEADER_PATH)

find_library(FUSE_LIBRARY NAMES fuse PATHS /lib /lib64 /usr/lib /usr/lib64 /lib/powerpc64le-linux-gnu /usr/lib/powerpc64le-linux-gnu )
IF(NOT FUSE_LIBRARY)
       message(WARNING "Can't find the fuse library" )
       set(FUSE_FOUND FALSE)
ENDIF(NOT FUSE_LIBRARY)


IF(FUSE_FOUND)
        message(STATUS "Found Fuse: ${FUSE_LIBRARY}" )
        set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}" "${FUSE_HEADER_PATH}")
        set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}" "${FUSE_LIBRARY}")
        add_executable(bscfsAgent bscfsAgent.cc bscfsOperations.cc)
        flightgen(bscfsAgent bscfsAgent_flightlog.h)
        target_link_libraries(bscfsAgent bbAPI -lpthread -lfuse -lrt)
        add_dependencies(bscfsAgent need_bbapi_version)
        target_compile_definitions(bscfsAgent PRIVATE -D_FILE_OFFSET_BITS=64 -DREENTRANT -DUSE_SC_LOGGER=1)

        flightlib(bscfsAgent fsutil)
        flightlib(bscfsAgent txp)

        install(TARGETS bscfsAgent COMPONENT burstbuffer DESTINATION bscfs/agent)

        add_executable(fuse_getattr fuse_getattr.cc)
        target_link_libraries(fuse_getattr -lfuse)
        target_compile_definitions(fuse_getattr PRIVATE -D_FILE_OFFSET_BITS=64)
        install(TARGETS fuse_getattr COMPONENT burstbuffer DESTINATION bscfs/tests)

        include_directories("${CMAKE_SOURCE_DIR}/bb/src")

ENDIF(FUSE_FOUND)
