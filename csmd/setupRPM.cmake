#================================================================================
#
#    csmd/setupRPM.cmake
#
#  © Copyright IBM Corporation 2015-2019. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# This file is used to override rpm defaults defined in scripts/setupRPM.cmake
# when non-default behavior is needed for csm rpms 

# Note: when shipping libraries to a new directory location via cmake "install ...", 
# the new directory must be added to the CMAKE_INSTALL_RPATH in scripts/setupRPATH.cmake

# Change the CSM rpms that do not contain compiled binaries to noarch
SET(CPACK_RPM_csm-db_PACKAGE_ARCHITECTURE "noarch")
SET(CPACK_RPM_csm-hcdiag_PACKAGE_ARCHITECTURE "noarch")


set( CPACK_RPM_csm-core_POST_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csmd/rpmscripts/csmd.post.uninstall")

set( CPACK_RPM_csm-core_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csmd/rpmscripts/csmd.post.install")

set(CPACK_RPM_csm-core_PACKAGE_REQUIRES "nvme-cli >= 1.4-3")
