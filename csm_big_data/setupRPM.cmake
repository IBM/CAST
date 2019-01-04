#================================================================================
#
#    csm_big_data/castBDS.cmake
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

SET( CPACK_RPM_csm-bds_PACKAGE_ARCHITECTURE "noarch" )
set( CPACK_RPM_csm-bds_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csm_big_data/rpmscripts/cast-bds.post.install" )
set( CPACK_RPM_csm-bds_PRE_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csm_big_data/rpmscripts/cast-bds.pre.uninstall" )

# Setup Kibana RPM
SET(CPACK_RPM_csm-bds-kibana_PACKAGE_ARCHITECTURE "noarch")
set( CPACK_RPM_csm-bds-kibana_POST_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csm_big_data/rpmscripts/kibana.pre.uninstall")
set( CPACK_RPM_csm-bds-kibana_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csm_big_data/rpmscripts/kibana.post.install")
set(CPACK_RPM_csm-bds-kibana_PACKAGE_REQUIRES "kibana >= 6.2.3")


# Setup Logstash RPM
SET(CPACK_RPM_csm-bds-logstash_PACKAGE_ARCHITECTURE "noarch")
set( CPACK_RPM_csm-bds-logstash_POST_UNINSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csm_big_data/rpmscripts/logstash.pre.uninstall")
set( CPACK_RPM_csm-bds-logstash_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/csm_big_data/rpmscripts/logstash.post.install")
set(CPACK_RPM_csm-bds-logstash_PACKAGE_REQUIRES "logstash >= 6.2.3")

