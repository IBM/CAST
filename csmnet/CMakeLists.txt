#================================================================================
#
#    csmnet/CMakeLists.txt
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
set(CSM_NETWORK_BASENAME csmnet)

#string(SUBSTRING ${GITCOMMITID} 0 12 CSM_VERSION_ID)
#add_definitions(-DCSM_VERSION_ID=\"${CSM_VERSION_ID}\")
#add_definitions(-D_GNU_SOURCE -D_USE_GNU)

#######################
# CSMNET_SECURE_COMMUNICATION enable or disable by setting
# to   yes  or  no
set(CSMNET_SECURE_COMMUNICATION yes)

if(CSMNET_SECURE_COMMUNICATION)
    message("Building with CSMNET_SECURE_COMMUNICATION enabled. Will require ssl libraries.")
	find_package(OpenSSL REQUIRED)
	add_definitions(-DSECURE_COMM)
endif(CSMNET_SECURE_COMMUNICATION)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)
