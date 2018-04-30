add_executable(buildversion buildversion.cc)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/bbapi_version.h
	COMMAND	${CMAKE_CURRENT_BINARY_DIR}/buildversion ${CMAKE_CURRENT_SOURCE_DIR}/bbapi_version.json ${CMAKE_CURRENT_SOURCE_DIR}/bbapi_version.h ${GITCOMMITID}
	DEPENDS buildversion ${CMAKE_CURRENT_SOURCE_DIR}/bbapi_version.json
	COMMENT "Generating bbapi_version"
	)
add_custom_target(need_bbapi_version DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bbapi_version.h)

execute_process(
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/genmanpages.pl --src=${CMAKE_CURRENT_SOURCE_DIR}/bbapi.h --bin=${CMAKE_CURRENT_BINARY_DIR} --template=${CMAKE_CURRENT_SOURCE_DIR}/apipodtemplate
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
file(GLOB BBAPIMANPAGES ${CMAKE_CURRENT_BINARY_DIR}/*.pod)
foreach(POD ${BBAPIMANPAGES})
   build_manpage(${POD} 3 "Burst Buffer" burstbuffer bb)
endforeach(POD)


file(GLOB bbapi_includes *.h *.md)
install(FILES ${bbapi_includes} COMPONENT burstbuffer DESTINATION bb/include)
install(FILES bbapi_version.h COMPONENT burstbuffer DESTINATION bb/include)
