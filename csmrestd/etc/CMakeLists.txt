#================================================================================
#
#    csmrestd/etc/CMakeLists.txt 
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
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
  "csmrestd.service"
)

# Only install to /usr/lib/systemd/system if building an rpm for distribution
# Otherwise ship to the work directory for test purposes
if(RPMBUILD)
  install(FILES ${INSTALL_FILES} COMPONENT csm-restd DESTINATION /usr/lib/systemd/system)
else()
  install(FILES ${INSTALL_FILES} COMPONENT csm-restd DESTINATION usr/lib/systemd/system)
endif()

