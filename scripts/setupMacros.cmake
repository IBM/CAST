#
##  Get all the directories in the floor
#
macro(getdirs var path)
 file(GLOB filelist ${path}/*)
 set(dirs "")
 foreach(F ${filelist})
   if(IS_DIRECTORY ${F})
     list(APPEND dirs ${F})
   endif()
 endforeach(F)
 set(${var} ${dirs})
endmacro()


#
## Performs install() but also registers the targets and components to allow for quicker builds
#
function(doinstall target component path)
   set_property(GLOBAL APPEND PROPERTY ALLTARGETS ${target})
   set_property(GLOBAL APPEND PROPERTY TOPLEVEL_${ADDTOPLEVEL}_targets ${target})
   set_property(GLOBAL APPEND PROPERTY COMPONENT_${component}_targets  ${target})
   install(TARGETS ${target} COMPONENT ${component} DESTINATION ${path})
endfunction()


add_custom_target(man ALL)

function(build_manpage podfile section subject component installroot)
  get_filename_component(basename ${podfile} NAME_WE)
  set(MANNAME ${CMAKE_CURRENT_BINARY_DIR}/${basename}.${section})
  
  add_custom_command(
       OUTPUT  ${MANNAME}
       COMMAND pod2man --center ${subject} --name ${basename} --release "Version ${VERSION}-${GITCOUNT}" --section ${section} ${podfile} > ${MANNAME}
       DEPENDS ${podfile}
       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
       COMMENT "Building manpage for ${basename}, section ${section}"
       VERBATIM)
  add_custom_target(manpage_${basename} ALL DEPENDS ${MANNAME})
  add_dependencies(man manpage_${basename})

  install(FILES ${MANNAME} COMPONENT ${component} DESTINATION ${installroot}/man/man${section}/)
endfunction()


function(build_pathsetup BPS_COMPONENT BPS_PATHSUFFIX)
  configure_file(${CMAKE_SOURCE_DIR}/scripts/pathtemplate.csh.in ${CMAKE_CURRENT_BINARY_DIR}/${BPS_COMPONENT}env.csh @ONLY)
  configure_file(${CMAKE_SOURCE_DIR}/scripts/pathtemplate.sh.in  ${CMAKE_CURRENT_BINARY_DIR}/${BPS_COMPONENT}env.sh  @ONLY)
  if(RPMBUILD)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BPS_COMPONENT}env.sh  COMPONENT ${BPS_COMPONENT} DESTINATION /etc/profile.d)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BPS_COMPONENT}env.csh COMPONENT ${BPS_COMPONENT} DESTINATION /etc/profile.d)
  else()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BPS_COMPONENT}env.sh  COMPONENT ${BPS_COMPONENT} DESTINATION etc/profile.d)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BPS_COMPONENT}env.csh COMPONENT ${BPS_COMPONENT} DESTINATION etc/profile.d)
  endif()
endfunction()
