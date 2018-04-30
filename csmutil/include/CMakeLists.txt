#================================================================================
#
#    csmutil/include/CMakeLists.txt
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
add_executable(csm_buildversion csm_buildversion.cc)

add_custom_command(TARGET csm_buildversion
	POST_BUILD
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/csm_buildversion ${CMAKE_CURRENT_SOURCE_DIR}/csm_version.h ${GITCOMMITID}
    COMMENT "Generating csm_version"
)

add_custom_target(need_csm_version DEPENDS csm_buildversion
	COMMENT "CSM_VERSION dependency build"
)
