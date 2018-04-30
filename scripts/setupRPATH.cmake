# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

#SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/foo/lib")
foreach(LVL ${TOPLEVELS})
  SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} "${CMAKE_INSTALL_PREFIX}/${LVL}/lib")
endforeach(LVL)
foreach(LVL ${FLOORDIRS})
   message("linking libraries from floor directory: ${LVL}/lib")

  SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} "${LVL}/lib")
endforeach(LVL)

# Add csm/lib to the CMAKE_INSTALL_RPATH as csm is not a TOPLEVEL  
SET(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} "${CMAKE_INSTALL_PREFIX}/csm/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
#LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
#IF("${isSystemDir}" STREQUAL "-1")
#   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#ENDIF("${isSystemDir}" STREQUAL "-1")
