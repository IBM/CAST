#================================================================================
#
#    csmd/src/ras/tests/CMakeLists.txt
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================
#add_subdirectory(scratch)
#add_subdirectory(rastestEnvFilter)
#add_subdirectory(testboostthread)

add_executable(test_RasEventThreshold test_RasEventThreshold.cc )
target_include_directories(test_RasEventThreshold PRIVATE ./ )
target_link_libraries(test_RasEventThreshold csmras fsutil csmutil  ${Boost_LIBRARIES} -lpthread)

