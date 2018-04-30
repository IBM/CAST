file(GLOB file_source *.cc)

ADD_LIBRARY(txp ${file_source})
#ADD_LIBRARY(txp SHARED ${file_source})

add_dependencies(txp txp_config)

target_compile_definitions(txp PRIVATE -DUSE_SC_LOGGER=1)

flightgen(txp txp_flightlog.h)
target_link_libraries(txp fsutil -lpthread -lrdmacm -libverbs -lssl -lcrypto)

flightlib(txp fsutil)

include_directories("${CMAKE_BASE_BINARY_DIR}/transport/src"
                    "${CMAKE_BASE_BINARY_DIR}/transport/include")

install(TARGETS txp COMPONENT transport-devel DESTINATION transport/sbin)
