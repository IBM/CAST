add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tools)
add_subdirectory(tests)
add_subdirectory(scripts)
add_subdirectory(wrappers)

#### RPM ####
set(CPACK_RPM_burstbuffer_PACKAGE_SUMMARY           "Burst Buffer system software" PARENT_SCOPE)
set(CPACK_RPM_burstbuffer_PACKAGE_DESCRIPTION       "This RPM provides burst buffer software and interfaces for transferring files between Spectrum Scale and a node-local NVMe SSD." PARENT_SCOPE)

set(CPACK_RPM_burstbuffer-tests_PACKAGE_SUMMARY           "Burst Buffer test cases" PARENT_SCOPE)
set(CPACK_RPM_burstbuffer-tests_PACKAGE_DESCRIPTION       "This RPM provides burst buffer testcases." PARENT_SCOPE)

set(CPACK_RPM_burstbuffer-tools_PACKAGE_SUMMARY           "Burst Buffer tools" PARENT_SCOPE)
set(CPACK_RPM_burstbuffer-tools_PACKAGE_DESCRIPTION       "This RPM provides various burst buffer tools used in BB development and test." PARENT_SCOPE)

set(CPACK_RPM_burstbuffer-lsf_PACKAGE_SUMMARY           "Burst Buffer LSF configuration" PARENT_SCOPE)
set(CPACK_RPM_burstbuffer-lsf_PACKAGE_DESCRIPTION       "This RPM provides script(s) that configure LSF jobs to use the burst buffer." PARENT_SCOPE)
set(CPACK_RPM_burstbuffer-lsf_PACKAGE_AUTOREQPROV "0
Prefix: ${CMAKE_INSTALL_PREFIX}/bb/scripts
" PARENT_SCOPE)


# RPM version pre-reqs
set(CPACK_RPM_burstbuffer_PACKAGE_REQUIRES       "nvme-cli,nvmetcli,fuse,fuse-libs,util-linux,lvm2,coreutils,xfsprogs,openssh-clients,grep,perl-Sys-Syslog" PARENT_SCOPE)
set(CPACK_RPM_burstbuffer-tests_PACKAGE_REQUIRES "ibm_smpi > 10.1"   PARENT_SCOPE)
set(CPACK_RPM_burstbuffer-tools_PACKAGE_REQUIRES "ibm_smpi > 10.1"   PARENT_SCOPE)

# Burstbuffer needs to run a post-install script
set(CPACK_RPM_burstbuffer_POST_INSTALL_SCRIPT_FILE  ${CMAKE_SOURCE_DIR}/bb/scripts/postinstall.sh PARENT_SCOPE)

# Burstbuffer needs to run a pre-uninstall script
set(CPACK_RPM_burstbuffer_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_SOURCE_DIR}/bb/scripts/preuninstall.sh PARENT_SCOPE)
