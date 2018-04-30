#================================================================================
#
#    csmrestd/rest_scripts/spectrum_scale/CMakeLists.txt 
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

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
    "create_node_leave_event.sh"
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT csm-restd DESTINATION csm/share/rest_scripts/spectrum_scale)
