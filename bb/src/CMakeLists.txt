# If CSMI toplevel is enabled, add CSM libraries and nodeController
foreach (LVL ${TOPLEVELS})
    if(${LVL} STREQUAL "csmi")
        message("Found csmi.  Adding BB NodeController for CSM")
        set(SRCCSM "nodecontroller_csm.cc")
        set(LIBCSM "csmi")
        set(DEFCSM "-DHAVE_CSMI")
    endif()
endforeach(LVL)

if(NOT DEFINED SRCCSM)
    if(EXISTS "/opt/ibm/csm/include")
        set(SRCCSM "nodecontroller_csm.cc")
        set(LIBCSM "/opt/ibm/csm/lib/libcsmi.so")
	set(DEFCSM "-DHAVE_CSMI")
    endif()
endif()

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bbdefaults.h
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gendefaults.pl ${CMAKE_CURRENT_SOURCE_DIR}/bbdefaults.h.in ${CMAKE_CURRENT_BINARY_DIR}/bbdefaults.h ${CMAKE_INSTALL_PREFIX} ${RPMBUILD}
        COMMENT "Generating bbdefaults.h"
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bbdefaults.h.in ${CMAKE_CURRENT_SOURCE_DIR}/gendefaults.pl
        )
add_custom_target(need_bbdefaults ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bbdefaults.h)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bbras.h
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/genras.pl ${CMAKE_CURRENT_SOURCE_DIR}/ras.json ${CMAKE_CURRENT_BINARY_DIR}/bbras.h ${CMAKE_CURRENT_BINARY_DIR}/bbras.csv
        COMMENT "Generating bbras.h"
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ras.json ${CMAKE_CURRENT_SOURCE_DIR}/genras.pl
        )
add_custom_target(need_bbras ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bbras.h)

add_executable(bbServer tracksyscall.cc bbconndata.cc main.cc connections.cc bberror.cc bbinternal.cc bbserver.cc xfer.cc usage.cc fh.cc serial.cc bbio.cc bbio_regular.cc bbio_BSCFS.cc BBTransferDef.cc BBJob.cc BBTagID.cc BBTagParts.cc BBTagInfo.cc BBTagInfoMap.cc BBLVKey_ExtentInfo.cc BBTagInfo2.cc BBTagInfoMap2.cc nodecontroller.cc bbwrkqmgr.cc bbwrkqe.cc ContribFile.cc ContribIdFile.cc HandleFile.cc LVUuidFile.cc weak.cc)

add_executable(bbProxy tracksyscall.cc bbconndata.cc main.cc connections.cc bbproxyConn2bbserver.cc bberror.cc bbinternal.cc bbproxy.cc lvlookup.cc fh.cc serial.cc usage.cc BBTransferDef.cc BBJob.cc nodecontroller.cc LVUtils.cc weak.cc ${SRCCSM})

add_library(bbAPI SHARED  bbconndata.cc connections.cc bberror.cc bbinternal.cc bbapi.cc bbapi2.cc fh.cc BBTransferDef.cc BBJob.cc nodecontroller.cc   ${SRCCSM})
add_executable(bbcmd bbcmd.cc)

flightgen(bbServer bbserver_flightlog.h)
target_link_libraries(bbServer fsutil txp -lpthread)
target_compile_definitions(bbServer PRIVATE -DBBSERVER=1 -DUSE_SC_LOGGER=1)
add_dependencies(bbServer need_bbapi_version need_bbdefaults need_bbras)
flightlib(bbServer fsutil)
flightlib(bbServer txp)

flightgen(bbProxy bbproxy_flightlog.h)
target_link_libraries(bbProxy fsutil txp -lpthread ${LIBCSM})
target_compile_definitions(bbProxy PRIVATE -DBBPROXY=1 -DUSE_SC_LOGGER=1 ${DEFCSM})
add_dependencies(bbProxy need_bbapi_version need_bbdefaults need_bbras)
flightlib(bbProxy fsutil)
flightlib(bbProxy txp)

flightgen(bbAPI bbapi_flightlog.h)
target_link_libraries(bbAPI fsutil txp -lpthread ${LIBCSM})
target_compile_definitions(bbAPI PRIVATE -DBBAPI=1 -DUSE_SC_LOGGER=1 ${DEFCSM})
add_dependencies(bbAPI need_bbapi_version need_bbdefaults need_bbras)
flightlib(bbAPI fsutil)
flightlib(bbAPI txp)

target_link_libraries(bbcmd bbAPI)
target_compile_definitions(bbcmd PRIVATE -DBBCMD=1 -DUSE_SC_LOGGER=1)
target_include_directories(bbcmd PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(bbcmd need_bbapi_version need_bbdefaults need_bbras)

include_directories("${CMAKE_BASE_BINARY_DIR}/bscfs/include"
                    "${CMAKE_BASE_BINARY_DIR}/transport/src"
                    "${CMAKE_BASE_BINARY_DIR}/transport/include"
                    "/opt/ibm/csm/include")

doinstall(bbServer burstbuffer bb/sbin)
doinstall(bbProxy  burstbuffer bb/sbin)
doinstall(bbAPI    burstbuffer bb/lib)
doinstall(bbcmd    burstbuffer bb/bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bbras.csv COMPONENT burstbuffer-mn DESTINATION bb/scripts)

build_manpage(${CMAKE_CURRENT_SOURCE_DIR}/bbcmd.pod    1 "Burst Buffer" burstbuffer bb)
build_manpage(${CMAKE_CURRENT_SOURCE_DIR}/bbProxy.pod  8 "Burst Buffer" burstbuffer bb)
build_manpage(${CMAKE_CURRENT_SOURCE_DIR}/bbServer.pod 8 "Burst Buffer" burstbuffer bb)
