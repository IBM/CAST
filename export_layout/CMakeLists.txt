add_subdirectory(src)
add_subdirectory(include)

#### RPM ####
set(CPACK_RPM_export_layout_PACKAGE_SUMMARY           "Kernel module to provide export layout device" PARENT_SCOPE)
set(CPACK_RPM_export_layout_PACKAGE_DESCRIPTION       "This RPM provides a kernel module that creates a /dev/export_layout device that can be used to obtain file layout locks on supporting file systems from user space." PARENT_SCOPE)

# RPM version pre-reqs
set(CPACK_RPM_export_layout_PACKAGE_REQUIRES "kernel-devel" PARENT_SCOPE)

# Export layout is GPL licensed (due to kernel build)
set(CPACK_RPM_export_layout_USER_BINARY_SPECFILE      ${CMAKE_SOURCE_DIR}/scripts/gpl_template.specrpm PARENT_SCOPE)

# Export layout needs to run a post-install script
set(CPACK_RPM_export_layout_POST_INSTALL_SCRIPT_FILE  ${CMAKE_SOURCE_DIR}/export_layout/src/postinstall.sh PARENT_SCOPE)

# Export layout needs to run a pre-uninstall script
set(CPACK_RPM_export_layout_PRE_UNINSTALL_SCRIPT_FILE ${CMAKE_SOURCE_DIR}/export_layout/src/preuninstall.sh PARENT_SCOPE)
