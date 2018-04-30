#================================================================================
#
#    csmd/src/daemon/etc/CMakeLists.txt
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

# Files that should not be executable when shipped
file(GLOB INSTALL_FILES
  "csmd-aggregator.service"
  "csmd-compute.service"
  "csmd-master.service"
  "csmd-utility.service"
)

# Files that should be executable when shipped
file(GLOB INSTALL_PROGRAMS
    "rotate-log-file.sh"
)

install(PROGRAMS ${INSTALL_PROGRAMS} COMPONENT csm-core DESTINATION csm/sbin)

# Only install to /usr/lib/systemd/system if building an rpm for distribution
# Otherwise ship to the work directory for test purposes
if(RPMBUILD)
  install(FILES ${INSTALL_FILES} COMPONENT csm-core DESTINATION /usr/lib/systemd/system)
else()
  install(FILES ${INSTALL_FILES} COMPONENT csm-core DESTINATION usr/lib/systemd/system)
endif()

