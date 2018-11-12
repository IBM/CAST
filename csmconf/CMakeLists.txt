#================================================================================
#
#    csmconf/CMakeLists.txt
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

# Configure all csm rpms to create /var/log/ibm/csm and /etc/ibm/csm
if(RPMBUILD)
  install(DIRECTORY COMPONENT csm-api DESTINATION /var/log/ibm/csm)
  install(DIRECTORY COMPONENT csm-api DESTINATION /etc/ibm/csm)
  install(DIRECTORY COMPONENT csm-bds DESTINATION /var/log/ibm/csm)
  install(DIRECTORY COMPONENT csm-bds DESTINATION /etc/ibm/csm)
  install(DIRECTORY COMPONENT csm-core DESTINATION /var/log/ibm/csm)
  install(DIRECTORY COMPONENT csm-core DESTINATION /var/log/ibm/csm/inv)
  install(DIRECTORY COMPONENT csm-core DESTINATION /etc/ibm/csm)
  install(DIRECTORY COMPONENT csm-core DESTINATION /opt/ibm/csm/prologs)
  install(DIRECTORY COMPONENT csm-core DESTINATION /opt/ibm/csm/recovery)
  install(DIRECTORY COMPONENT csm-db DESTINATION /var/log/ibm/csm/db)
  install(DIRECTORY COMPONENT csm-db DESTINATION /etc/ibm/csm)
  install(DIRECTORY COMPONENT csm-hcdiag DESTINATION /var/log/ibm/csm)
  install(DIRECTORY COMPONENT csm-hcdiag DESTINATION /etc/ibm/csm)
  install(DIRECTORY COMPONENT csm-restd DESTINATION /var/log/ibm/csm)
  install(DIRECTORY COMPONENT csm-restd DESTINATION /etc/ibm/csm)
endif()

install(FILES csm_api.acl        COMPONENT csm-core DESTINATION csm/share/etc)
install(FILES csm_api.cfg        COMPONENT csm-core DESTINATION csm/share/etc)
install(FILES csm_aggregator.cfg COMPONENT csm-core DESTINATION csm/share/etc)
install(FILES csm_compute.cfg    COMPONENT csm-core DESTINATION csm/share/etc)
install(FILES csm_master.cfg     COMPONENT csm-core DESTINATION csm/share/etc)
install(FILES csm_utility.cfg    COMPONENT csm-core DESTINATION csm/share/etc)

install(FILES csmrestd.cfg COMPONENT csm-restd DESTINATION csm/share/etc)

install(FILES POST_README.md PERMISSIONS OWNER_WRITE OWNER_READ COMPONENT csm-core DESTINATION csm/share/prologs)
install(FILES privileged.ini PERMISSIONS OWNER_WRITE OWNER_READ COMPONENT csm-core DESTINATION csm/share/prologs)

install(PROGRAMS privileged_epilog PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ COMPONENT csm-core DESTINATION csm/share/prologs)
install(PROGRAMS privileged_prolog PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ COMPONENT csm-core DESTINATION csm/share/prologs)

install(PROGRAMS soft_failure_recovery PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ COMPONENT csm-core DESTINATION csm/share/recovery)

