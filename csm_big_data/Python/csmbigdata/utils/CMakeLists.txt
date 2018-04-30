#================================================================================
#
#    csm_big_data/python/LogAnalysis/csmbigdata/utils/CMakeLists.txt
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

set(SUBDIR Python/csmbigdata/utils)

file(GLOB INSTALL_FILES
  "csmi_tool.py"
  "__init__.py"
  "output_helpers.py"
  "hive_payload.py"
  "unity_helpers.py"
  "unity_payload.py"
  "unity.py"
  "README.rst"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})

