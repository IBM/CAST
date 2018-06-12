#================================================================================
#
#    csmdb/sql/CMakeLists.txt
#
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

set(SUBDIR sql)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
  "csm_db_script.sh"
  "csm_db_stats.sh"
  "csm_db_connections_script.sh"
  "csm_db_ras_type_script.sh"
  "csm_db_schema_version_upgrade.sh"
  "csm_db_history_table_delete.sh"
  "csm_history_table_archive_template.sh"
  "csm_history_table_delete_template.sh"
  "csm_history_wrapper_archive_script_template.sh"
  "csm_history_wrapper_delete_script_template.sh"
  "csm_ras_event_action_table_archive.sh"
  "csm_ras_event_action_wrapper_archive_script.sh"
  "csm_ras_event_action_table_delete.sh"
  "csm_ras_event_action_wrapper_delete_script.sh"
  "csm_db_backup_script_v1.sh"
)

# Files that should not be executable when shipped
install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${DB_RPM_NAME} DESTINATION ${DB_BASE_NAME})

file(GLOB INSTALL_FILES
  "csm_db_schema_version_data.csv"
  "csm_create_tables.sql"
  "csm_create_triggers.sql"
  "csm_delete_data.sql"
  "csm_drop_functions.sql"
  "csm_drop_tables.sql"
  "csm_ras_type_data.csv"
)
install(FILES ${INSTALL_FILES} COMPONENT ${DB_RPM_NAME} DESTINATION ${DB_BASE_NAME})
