cmake_minimum_required (VERSION 3.2.0)
project (ibm)

message("workdir: ${WORKDIR}")
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "internal")
set(CMAKE_INSTALL_PREFIX ${WORKDIR})
set(CMAKE_INSTALL_MESSAGE LAZY)

set(BUILD_DOCS_RPM 1)

#########
# as of cmake 3.2.3, it needs multiple components to build per-component RPMs.  Tricking cmake by creating a
# dummy component and specifying only the 'docs' component
set(CPACK_COMPONENTS_ALL docs)
install(FILES ${CMAKE_INSTALL_PREFIX}/docs/index.html COMPONENT docs_dummy DESTINATION docs_dummy)
#########

install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/docs/         
  	  	 COMPONENT docs 
		 DESTINATION docs
		 )
include(../setupRPM.cmake)
