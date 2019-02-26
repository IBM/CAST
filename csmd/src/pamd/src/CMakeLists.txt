#================================================================================
#
#    csmd/src/pamd/src/CMakeLists.txt
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

set(CSM_PAM_LIB_SRC
    csm_pam.cc
)
add_definitions(-DUSE_SC_LOGGER)
    

add_library(csmpam SHARED ${CSM_PAM_LIB_SRC})

target_link_libraries(csmpam csmi)
include_directories(csmpam PRIVATE ./ ${BASEDIR}/ )

if( ${RPMBUILD} )
    install(TARGETS csmpam COMPONENT csm-core DESTINATION /usr/lib64/security/)
else( !{RPMBUILD} )
    install(TARGETS csmpam COMPONENT csm-core DESTINATION csm/lib)
endif( ${RPMBUILD} )

