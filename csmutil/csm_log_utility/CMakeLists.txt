#================================================================================
#
#    csmutil/csm_log_utility/CMakeLists.txt
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

add_subdirectory(Helper_Files)
add_subdirectory(Logs)
add_subdirectory(Reports)

# non executable files
file(GLOB INSTALL_SCRIPTS
    "ReadMe.md"
)

install(FILES ${INSTALL_SCRIPTS} COMPONENT csm-tools DESTINATION csm/tools)

# Files that should be executable when shipped

file(GLOB INSTALL_PROGRAMS
  "API_Statistics.py"
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT csm-tools DESTINATION csm/tools)

# CPACK_RPM settings for the csm-tools rpm are managed in csmd/setupRPM.cmake
