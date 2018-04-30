execute_process(COMMAND uname -m 
		COMMAND tr -d '\n' 
		OUTPUT_VARIABLE ARCHITECTURE)
set(CPACK_FILENAME_TEMPLATE "ibm-COMPONENT-CPACK_PACKAGE_VERSION-ARCHITECTURE")

if("${CPACK_INSTALL_CMAKE_PROJECTS}" STREQUAL "")
   set(CPACK_RPM_RELOCATION_PATHS "/opt/ibm/src")
   set(CPACK_RPM_PACKAGE_AUTOREQ 0)
   set(CPACK_PACKAGE_RELOCATABLE 1)
endif()

# Workaround for cmake 3.7.1 bug when using user-specified spec files (fixed in cmake commit 574c81e28cef6737adc1619ce3b44b43bdcf308b).  
set(RPMBUILD_FLAGS "-bb")
