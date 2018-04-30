# Packaging
add_custom_target(partial_install COMMAND ${CMAKE_COMMAND} -P cmake_install.cmake)
add_custom_target(noinstall COMMAND echo skipping install)


include (InstallRequiredSystemLibraries)
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_VERSION ${VERSION})

if(${RPMBUILDSRC})
else()
  set(CPACK_PACKAGE_DIRECTORY ${WORKDIR}/rpms)
endif()

set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_VENDOR "IBM")
set(CPACK_PROPERTIES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/CPackProperties.cmake")

set(CPACK_SOURCE_GENERATOR "RPM")
set(CPACK_SOURCE_IGNORE_FILES "/.git/;/work/;/.build/;/.config;/.gitignore;/SETUP.sh")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "ibm-coral-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.src")

set(CPACK_RPM_SOURCE_PKG_BUILD_PARAMS ${CMAKEFLAGS})

set(CPACK_PACKAGE_RELOCATABLE 0)

# Spectrum MPI RPMs do not currently have provide lists.  Filter those libraries from the RPM autoreqprov tool.
set(CPACK_RPM_PACKAGE_AUTOREQPROV "1
%{?filter_setup:
%filter_from_requires /libmpi.*so/d
%filter_setup
}
")

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_LICENSE "EPL")
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_PACKAGE_RELEASE ${BUILDID_SUFFIX})
set(CPACK_RPM_PACKAGE_URL "https://github.ibm.com/CORAL/bluecoral")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION 
  /opt /opt/ibm 
  /var /var/log
  /usr /usr/lib /usr/lib/systemd /usr/lib/systemd/system
)

#### For RPATH to be correct, the packaging install prefix should match the install prefix.  
#### Official builds should set the install prefix, not the packaging install prefix
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

if(${BUILD_DOCS_RPM})
  set(CPACK_RPM_RELOCATION_PATHS "/opt/ibm")
  set(CPACK_RPM_PACKAGE_AUTOREQ 0)
  set(CPACK_PACKAGE_RELOCATABLE 1)
  set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/ibm")
endif()

include(CPack)
