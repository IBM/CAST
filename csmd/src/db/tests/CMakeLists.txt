#================================================================================
#
#    csmd/src/db/tests/CMakeLists.txt
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
## lower level db test using postgresql c client library
add_executable(test_postgre_api test_postgre_api.cc ../src/PostgreSql.cc)

target_include_directories(test_postgre_api PRIVATE ../include)

target_link_libraries(test_postgre_api fsutil -lpq -lpthread)

#add_test(test_postgre_api test_postgre_api )

# higher level db test using our own wrapper

add_executable(test_csmdb_api test_csmdb_api.cc ../src/PostgreSql.cc ../src/DBResult.cc)

target_link_libraries(test_csmdb_api fsutil -lpq -lpthread)

target_include_directories(test_csmdb_api PRIVATE ../include)

#add_test(test_csmdb_api test_csmdb_api )

add_definitions(-DUSE_SC_LOGGER)
