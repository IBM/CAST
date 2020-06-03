#================================================================================
#
#    csmd/dcgm.cmake
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

# DCGM does not have a stable API interface for Alpha and Beta deliveries
# This causes CSM to require coreq changes when a new version of DCGM becomes available
# In order to stage these changes into git, use REQUIRED_DCGM_VERSION to control compilation
#
# Process when new DCGM becomes available:
# Set REQUIRED_DCGM_VERSION to new version in git to disable compilation everywhere
# Test new version of DCGM with CSM in sandbox and commit changes when ready
# Upgrade level of DCGM everywhere to enable the changes to resume compilation
#
# CSM Alpha 2 DCGM version:
# datacenter-gpu-manager-363.11-1.ppc64le
# set(REQUIRED_DCGM_VERSION "datacenter-gpu-manager-363.11-1.ppc64le")
# CSM Beta 1 DCGM version:
# datacenter-gpu-manager-375.46-1.ppc64le
# set(REQUIRED_DCGM_VERSION "datacenter-gpu-manager-375.46-1.ppc64le")
# CSM Beta 2 and PRPQ DCGM version:
# datacenter-gpu-manager-384-1.ppc64le 
# set(REQUIRED_DCGM_VERSION "datacenter-gpu-manager-384-1.ppc64le")
# CSM 0.4.0 prega version:
# set(REQUIRED_DCGM_VERSION "datacenter-gpu-manager-1.3.3-1.ppc64le")
# CSM GA version:
# datacenter-gpu-manager-1.4.0-1.ppc64le 
# datacenter-gpu-manager-1.4.1-1.ppc64le
# set(REQUIRED_DCGM_VERSION "datacenter-gpu-manager-1.6")
# CSM 1.8 version:
# datacenter-gpu-manager-2.0.4-1.ppc64le
set(REQUIRED_DCGM_VERSION "datacenter-gpu-manager")

# Check if DCGM is installed
execute_process(COMMAND "/usr/bin/rpm" "-q" "datacenter-gpu-manager"
                RESULT_VARIABLE DCGM_INSTALLED
                OUTPUT_VARIABLE DCGM_RPM_NAME
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# At the toplevel, DCGM is defined by configure.pl --dcgm/--nodcgm
# If DCGM is enabled at the top level, and the rpm is installed, and the rpm is the required version, build with DCGM support
# Otherwise, disable DCGM support
if( (DCGM) AND (DCGM_INSTALLED EQUAL 0) AND ("${DCGM_RPM_NAME}" MATCHES "${REQUIRED_DCGM_VERSION}") )
  message("Building CSM with DCGM support, installed version is ${DCGM_RPM_NAME}")
  
  # Note: add_definitions is required to get the symbol DCGM defined in the cxx source files
  add_definitions(-DDCGM)
  
elseif(NOT DCGM)
  message("Building CSM without DCGM support, DCGM flag is not set.")
elseif(NOT DCGM_INSTALLED EQUAL 0)
  message("Building CSM without DCGM support, datacenter-gpu-manager is not installed")
else()
  message("Building CSM without DCGM support, installed version ${DCGM_RPM_NAME} != required version ${REQUIRED_DCGM_VERSION}")
endif()
