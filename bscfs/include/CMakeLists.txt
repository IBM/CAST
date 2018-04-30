execute_process(
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/genmanpages.pl --src=${CMAKE_CURRENT_SOURCE_DIR}/bscfsAPI.h --bin=${CMAKE_CURRENT_BINARY_DIR} --template=${CMAKE_CURRENT_SOURCE_DIR}/podtemplate
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
file(GLOB BSCFSMANPAGES ${CMAKE_CURRENT_BINARY_DIR}/*.pod)
foreach(POD ${BSCFSMANPAGES})
   build_manpage(${POD} 3 "Burst Buffer" burstbuffer bb)
endforeach(POD)


install(FILES bscfsAPI.h bscfsAPI_internal.h
	    bscfs_ioctl.h bscfs_mapfile.h bscfs_index.h
	COMPONENT burstbuffer
	DESTINATION bscfs/include)
