file(GLOB sysiod_includes *.h)
install(FILES ${sysiod_includes} COMPONENT fshipmond DESTINATION fshipmond/include)
include_directories("${BASEDIR}/transport/include"
                    "${CMAKE_BASE_BINARY_DIR}/transport/include")
