#================================================================================
#
#    csm_big_data/python/CMakeLists.txt
#
#    © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================


set(SUBDIR python)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
    "findJobKeys.py"
    "findJobMetrics.py"
    "findJobsInRange.py"
    "findJobsRunning.py"
    "findJobTimeRange.py"
    "findUserJobs.py"
    "findWeightedErrors.py"
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

# Files that should not be executable when shipped
file(GLOB INSTALL_FILES
    "cast_helper.py"
    "sampleWeightedErrorMap.json"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

