find_package(Doxygen)

if(DOXYGEN_FOUND)
  configure_file(${SCRIPTDIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(docs_pre1 COMMAND ${CMAKE_COMMAND} --build . --target install
  			      COMMAND mkdir -p docs
  			      WORKING_DIRECTORY ${BUILDDIR}
  			      )
  add_custom_target(docs_pre2 ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile 
		       WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
		       COMMENT "Generating Doxygen" 
		       VERBATIM)
  add_custom_target(docs COMMAND ${CMAKE_COMMAND} -DWORKDIR=${WORKDIR} "${SCRIPTDIR}/docs"
  			 COMMAND ${CMAKE_COMMAND} --build . --target package
  			 WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}/docs")

  add_dependencies(docs docs_pre2)
  add_dependencies(docs_pre2 docs_pre1)
else(!DOXYGEN_FOUND)
  add_custom_target(docs COMMAND echo "Error: Doxygen not installed on this system!!!")
endif(DOXYGEN_FOUND)
