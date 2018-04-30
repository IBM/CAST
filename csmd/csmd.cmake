#================================================================================
#
#    csmd/csmd.cmake
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
set(CSMD_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/csmd/include
  ${CMAKE_SOURCE_DIR}/csmd/src/daemon
  ${CMAKE_SOURCE_DIR}/csmd/src/db
  ${CMAKE_SOURCE_DIR}/csmd/src/ras
  ${CMAKE_SOURCE_DIR}/csmd/src/inv/include
)


execute_process(
  COMMAND ./csmd/extract_csmdb_version.sh ./csmdb/sql/csm_db_schema_version_data.csv
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE DB_SCHEMA_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DDB_SCHEMA_VERSION=\"${DB_SCHEMA_VERSION}\")
#add_definitions(-DCSM_MULTI_COMPUTE_PER_NODE)
#add_definitions(-DCSM_MULTI_AGGREGATOR_PER_NODE)
#add_definitions(-DWITH_MASTER_LOAD_STATS)
add_definitions(-DIB_CONNECTOR)

set(CSMD_BUILD_DEPENDENCIES
#   mosquitto_make
	need_csm_version
)
