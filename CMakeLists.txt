cmake_minimum_required (VERSION 3.2.0)
project (ibm)

set(BUILDID_RELEASE    1)
set(BUILDID_CUMULFIXID 1)
set(BUILDID_EFIXID     0)

execute_process(
  COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GITCOUNT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(BUILDID_SUFFIX     "${GITCOUNT}")
set(VERSION "${BUILDID_RELEASE}.${BUILDID_CUMULFIXID}.${BUILDID_EFIXID}")

enable_testing()

include (scripts/setupMacros.cmake)
include (scripts/setupBoost.cmake)
include (CTest)

execute_process(
  COMMAND git log -1 --format=%H
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE tmp_GITCOMMITID
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND date
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE tmp_BUILDTIME
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GITCOMMITIDSHORT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)


message("GIT commit: ${tmp_GITCOMMITID}")
message("Build time: ${tmp_BUILDTIME}")

if(tmp_GITCOMMITID STREQUAL "")
   message("GIT commit could not be determined.  Using current build time as commit ID")
   set(GITCOMMITID ${tmp_BUILDTIME})
else()
   set(GITCOMMITID ${tmp_GITCOMMITID})
endif()


################################
# Available toplevel directories
# The DISABLE_TOPLEVELS variable will trim from this list.
#
set(ALL_TOPLEVELS extras flightlog utilities transport bb fshipmond fshipd fshipcld csmutil csmnet csmconf scripts csmi csmdb csmd csmrestd csm_big_data export_layout hcdiag bscfs)
#set(ALL_TOPLEVELS flightlog utilities transport bb fshipmond fshipd fshipcld export_layout bscfs)
set(DISABLE_TOPLEVELS "none" CACHE STRING "Disabled toplevels")
message("Disabled toplevels: ${DISABLE_TOPLEVELS}")

set(BASEDIR ${CMAKE_SOURCE_DIR} CACHE FILEPATH "Base directory of source repository")
set(SETUPFILE "." CACHE FILEPATH "Path to the SETUP.sh environment setup script")
set(GENXCODE OFF CACHE BOOL "Generate an Xcode project")
set(GENECLIPSEPROJECT OFF CACHE BOOL "Generate an Eclipse project")
set(ALTINCLUDE "" CACHE FILEPATH "Path to alternate include directory")
set(VERBOSE_COMPILE OFF CACHE BOOL "Compile using compiler's verbose setting")
set(REMOTEIP "" CACHE STRING "IP address for remote ssh builds")
set(BUILD_PARALLEL OFF CACHE BOOL "Perform parallel build")
set(MINCMAKE "" CACHE STRING "Minimum cmake version permitted")
set(OPTLEVEL "-O0" CACHE STRING "Optimization level")
set(DCGM OFF CACHE BOOL "Build CSM with support for Nvidia Data Center GPU Manager (DCGM)")

message("Base source directory: ${CMAKE_SOURCE_DIR}")

set(CMAKE_BASE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
message("Base build directory:  ${CMAKE_BASE_BINARY_DIR}")

string(REGEX REPLACE "[ ,]" ";" DOUT ${DISABLE_TOPLEVELS})
set(TOPLEVELS ${ALL_TOPLEVELS})
list(REMOVE_ITEM TOPLEVELS ${DOUT})

###################################
#  Setup default compile options
#
set(CMAKE_C_FLAGS   "-g -Wall -Werror -m64 -fPIC")
set(CMAKE_CXX_FLAGS "-g -Wall -Werror -m64 -fPIC -std=c++11")

set(CMAKE_C_FLAGS_DEBUG   "")
set(CMAKE_CXX_FLAGS_DEBUG "")

set(CMAKE_C_FLAGS_RELWITHDEBINFO   "${OPTLEVEL}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${OPTLEVEL}")

set(CMAKE_C_FLAGS_RELEASE   "${OPTLEVEL}")
set(CMAKE_CXX_FLAGS_RELEASE "${OPTLEVEL}")

# keep symbols in rpmbuild step
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
   set(CPACK_RPM_SPEC_INSTALL_POST "/bin/true")
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
   set(CPACK_RPM_SPEC_INSTALL_POST "/bin/true")
endif()


# Define __SOURCEROOT__
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SOURCEROOT__='\"${CMAKE_SOURCE_DIR}\"'")

# Define __FILENAME__ which gets evaluated to the short file name ("main.cc") for each source file
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(<F)\"'")

if(${VERBOSE_COMPILE})
  add_definitions(-v)
endif()

set(STRICT_WARN OFF CACHE BOOL "Enable strict warnings")
message("Option STRICT_WARN is ${STRICT_WARN}")
if(${STRICT_WARN})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -W -Wunused -Wcomment -Wparentheses -Wconversion -Wsign-compare -Wsign-conversion -Wundef")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -W -Wunused -Wcomment -Wparentheses -Wconversion -Wsign-compare -Wsign-conversion -Wundef")
endif(${STRICT_WARN})

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "internal")

set(RPMBUILD OFF CACHE BOOL "Perform an RPM build")
message("Option RPMBUILD is ${RPMBUILD}")

if(${RPMBUILD})
  set(CMAKE_INSTALL_PREFIX "/opt/ibm")
else(!${RPMBUILD})
  set(CMAKE_INSTALL_PREFIX ${WORKDIR})
endif(${RPMBUILD})

set(CMAKE_INSTALL_MESSAGE LAZY)
include (scripts/setupRPATH.cmake)
include (scripts/setupClang.cmake)

##########################################################
#  Add include paths relative to repository root/ directory
#
include_directories(${CMAKE_SOURCE_DIR})

#############################################################################
#  Add include paths relative to repositoryroot/toplevel/include/ directories
#
foreach (LVL ${ALL_TOPLEVELS})
	include_directories("${CMAKE_SOURCE_DIR}/${LVL}/include")
endforeach(LVL)

include_directories(${ALTINCLUDE})

##################################################################
#  Add include paths relative to floor/subdir/include/ directories
#
if(${FLOOR})
getdirs(FLOORDIRS ${FLOOR})
foreach (LVL ${FLOORDIRS})
     message("including files from floor directory: ${LVL}/include")
     include_directories(${LVL}/include)
endforeach(LVL)
endif()

###################################
#  Build all configured source code
foreach (LVL ${TOPLEVELS})
	set(ADDTOPLEVEL ${LVL})
	add_subdirectory(${LVL})
endforeach(LVL)

# csmd RPM packaging customization
include (csmd/setupRPM.cmake)

include (scripts/setupDoxygen.cmake)
include (scripts/setupTargets.cmake)
include (scripts/setupRPM.cmake)

file(GLOB config *.cfg csmconf/*.cfg)
install(FILES ${config} COMPONENT coral DESTINATION bin)
