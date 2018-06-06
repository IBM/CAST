#================================================================================
#
#    csm_big_data/Logstash/plugins/grok_dynamic/lib/logstash/filters/CMakeLists.txt
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

set(SUBDIR Logstash/plugins/grok_dynamic/lib/logstash/filters)

file(GLOB INSTALL_FILES
  "grok_dynamic_pure.rb"
  "grok_dynamic.rb"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})
