#================================================================================
#
#    csmd/src/db/src/db.cmake
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

set(CSMD_DB_SRC_DIR
  ${CMAKE_SOURCE_DIR}/csmd/src/db/src
)

# Define the list of files needed to build the csmd inventory code
# Used by csmd/src/daemon/src/CMakeLists.txt to add the db code to csmd
set(CSM_DB_SRC
  ${CSMD_DB_SRC_DIR}/DBResult.cc
  ${CSMD_DB_SRC_DIR}/PostgreSql.cc
  ${CSMD_DB_SRC_DIR}/csm_db_event_content.cc
)
