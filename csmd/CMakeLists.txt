#================================================================================
#
#    csmd/CMakeLists.txt
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

include( ${CMAKE_SOURCE_DIR}/csmd/dcgm.cmake )
include (${CMAKE_SOURCE_DIR}/csmd/csmd.cmake)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(tests)

add_subdirectory(bench)
