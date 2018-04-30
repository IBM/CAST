## Build flightlog library (.so)
add_library(flightlog SHARED flightlog.c dynplugin.cc)

## Build decoder executable using libflightlog.so shared object
add_executable(decoder decoder.cc)
target_link_libraries(decoder flightlog -ldl ${Boost_LIBRARIES})

## Install targets
install(TARGETS  flightlog   COMPONENT flightlog DESTINATION flightlog/lib)
install(TARGETS  decoder     COMPONENT flightlog DESTINATION flightlog/bin)
install(PROGRAMS buildFlightRegistry.pl COMPONENT flightlog-devel DESTINATION flightlog/bin)


## Function call so other executables can easily auto-generate their flightlog registry and link libflightlog.so
file(GLOB BuildFlightRegistry "${CMAKE_CURRENT_SOURCE_DIR}/buildFlightRegistry.pl")
set(ENV{BuildFlightRegistry} ${BuildFlightRegistry})

function(flightgen target filename)
   message("Running flightgen ${target}")
   target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
   string(REPLACE ".h" ".c" SOURCENAME ${filename})
   add_custom_target(${target}_flightgen 
   		BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${filename} ${CMAKE_CURRENT_BINARY_DIR}/${SOURCENAME}
		COMMAND $ENV{BuildFlightRegistry} --archive --path ${CMAKE_CURRENT_SOURCE_DIR} --output ${CMAKE_CURRENT_BINARY_DIR}/${filename} --append "${CMAKE_CURRENT_BINARY_DIR}/${filename}_append.txt"
		COMMENT "Synchronizing flightlog registry for target ${target} at ${filename}."
		VERBATIM
		)
   add_dependencies(${target} ${target}_flightgen)
   target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${filename} ${CMAKE_CURRENT_BINARY_DIR}/${SOURCENAME})
   target_link_libraries(${target} flightlog -ldl)
   set_target_properties(${target} PROPERTIES FLIGHTLOG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/${filename})
   set_target_properties(${target} PROPERTIES FLAPPENDHEADER "${CMAKE_CURRENT_BINARY_DIR}/${filename}_append.txt")
   file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${filename}_append.txt "")
endfunction()

function(flightlib target libtarget)
   get_target_property(fllibinclude ${libtarget} FLIGHTLOG_HEADER)
   get_target_property(flappendpath ${target} FLAPPENDHEADER)
   file(APPEND ${flappendpath} "${fllibinclude}\n")
   add_dependencies(${target}_flightgen ${libtarget}_flightgen)   
endfunction()

function(flightdecoder target decodertarget)
   message("Running flightdecoder ${target} ${decodertarget}")
   target_include_directories(${decodertarget} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
   add_dependencies(${decodertarget} ${target})
endfunction()

build_manpage(${CMAKE_CURRENT_SOURCE_DIR}/decoder.pod  1 "Flightlog" flightlog flightlog)
build_pathsetup(flightlog flightlog/bin)
