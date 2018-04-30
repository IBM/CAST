#================================================================================
#
#    csmd/src/CMakeLists.txt
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

# See csmd/dcgm.cmake for more info on DCGM settings!
# At the toplevel, DCGM is defined by configure.pl --dcgm/--nodcgm
# If DCGM is enabled at the top level, and the rpm is installed, and the rpm is the required version, build with DCGM support
# Otherwise, disable DCGM support
if( (DCGM) AND (DCGM_INSTALLED EQUAL 0) AND (DCGM_RPM_NAME STREQUAL REQUIRED_DCGM_VERSION) )
  # Only build the DCGM test cases when an appropriate level of DCGM is available 
  add_subdirectory(inv/tests/dcgm)
endif()

add_subdirectory(daemon/etc)
add_subdirectory(daemon/src)
add_subdirectory(daemon/tests)

add_subdirectory(db/tests)

add_subdirectory(ras/src)
add_subdirectory(ras/tests)
add_subdirectory(ras/actions)

add_subdirectory(inv/src)
add_subdirectory(inv/tests)
add_subdirectory(inv/tests/ufm)
add_subdirectory(inv/ib_and_switch)

add_subdirectory(pamd/src)

add_subdirectory(utils/src)

