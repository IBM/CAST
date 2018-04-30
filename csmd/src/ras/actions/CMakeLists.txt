#================================================================================
#
#    csmd/src/ras/actions/CMakeLists.txt
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

set(RAS_ACTIONS_BASE_NAME csm/ras/actions)
set(RAS_ACTIONS_RPM_NAME csm-core)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
    "node_not_ready"
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${RAS_ACTIONS_RPM_NAME} DESTINATION ${RAS_ACTIONS_BASE_NAME})

# Create /var/log/ibm/csm/ras/actions during the rpm install
if(RPMBUILD)
install(DIRECTORY COMPONENT ${RAS_ACTIONS_RPM_NAME} DESTINATION /var/log/ibm/csm/ras/actions)
endif()
