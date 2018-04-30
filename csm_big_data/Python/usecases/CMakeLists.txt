#================================================================================
#
#    csm_big_data/Python/usecases/CMakeLists.txt
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

add_subdirectory(config.inis)

set(SUBDIR Python/usecases)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
  "find_job_keys.py"
  "find_job_metrics.py"
  "find_jobs_running.py"
  "find_job_time_range.py"
  "find_temp_info.py" 
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

# Files that should not be executable when shipped
file(GLOB INSTALL_FILES
  "csm_bds_context.py"
  "default.ini"
  "__init__.py"
  "README.rst"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

