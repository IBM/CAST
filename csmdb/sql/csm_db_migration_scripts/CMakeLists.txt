#================================================================================
#
#    csmdb/sql/csm_db_migration_scripts/CMakeLists.txt
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

set(SUBDIR csm_db_migration_scripts)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
)

# Files that should not be executable when shipped
install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${DB_RPM_NAME} DESTINATION ${DB_BASE_NAME}/${SUBDIR})

file(GLOB INSTALL_FILES
  "csm_drop_triggers_functions.sql"
  "csm_db_schema_updates_16_0_ms.sql"
  "csm_db_schema_updates_16_2_ms.sql"
  "csm_db_schema_updates_17_0_ms.sql"
)
install(FILES ${INSTALL_FILES} COMPONENT ${DB_RPM_NAME} DESTINATION ${DB_BASE_NAME}/${SUBDIR})
