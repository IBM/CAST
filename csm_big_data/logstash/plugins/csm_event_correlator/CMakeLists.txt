#================================================================================
#
#    csm_big_data/Logstash/plugins/grok_dynamic/CMakeLists.txt
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

add_subdirectory(spec/logstash/filters)
add_subdirectory(lib/logstash/filters)

execute_process(
    COMMAND bash  "-c" "pwd; rm -rf *.gem; gem build logstash-filter-csm_event_correlator.gemspec"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(SUBDIR logstash/plugins/csm_event_correlator)

# Files that should not be executable when shipped
file(GLOB INSTALL_FILES
    "logstash-filter-csm-event-correlator-*.gem"
)

install(FILES ${INSTALL_FILES} COMPONENT ${BDS_RPM_NAME} DESTINATION ${BDS_BASE_NAME}/${SUBDIR})
