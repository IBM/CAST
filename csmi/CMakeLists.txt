#================================================================================
#
#    csmi/CMakeLists.txt
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

# todo: to be removed after cleaning up the dependencies to csmd
include( ${CMAKE_SOURCE_DIR}/csmd/dcgm.cmake )

set(CSMI_BASENAME csmi)

add_subdirectory(src)
add_subdirectory(include)

add_subdirectory(src/common/tests)
add_subdirectory(src/bb/bb_cmds)
add_subdirectory(src/bb/cmd)
add_subdirectory(src/bb/tests)
add_subdirectory(src/inv/cmd)
add_subdirectory(src/inv/tests)
add_subdirectory(src/ras/cmd)
add_subdirectory(src/ras/tests)
add_subdirectory(src/diag/cmd)
add_subdirectory(src/diag/tests)
add_subdirectory(src/wm/cmd)
add_subdirectory(src/wm/tests)
