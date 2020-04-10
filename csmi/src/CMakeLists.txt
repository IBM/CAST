#================================================================================
#
#    csmi/src/CMakeLists.txt
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
file(GLOB csmi_serialization_SRC
    "common/src/csmi_common_serial.c"
    "common/src/csmi*_serialization.c"
    "inv/src/csmi*_serialization.c"
    "ras/src/csmi_*_serialization.c"
    "wm/src/csmi_*_serialization.c"
	"diag/src/csmi_*_serialization.c"
	"bb/src/csmi_*_serialization.c"
    
    "common/src/csmi*_internal.c"
    "inv/src/csmi*_internal.c"
    "ras/src/csmi_*_internal.c"
    "wm/src/csmi_*_internal.c"
	"diag/src/csmi_*_internal.c"
	"bb/src/csmi_*_internal.c"

    "common/src/csmi_cmds.c"
    "common/src/csm_api_helper_macros.c"
    "common/src/csmi_struct_hash.c"
    "common/src/csmi_json.cc"

)

file(GLOB csmi_SRC
    "common/src/*.c"
    "ras/src/*.c"
    "inv/src/*.c"
	"diag/src/*.c"
	"bb/src/*.c"
    "wm/src/*.c"
)



add_definitions(-D_XOPEN_SOURCE=700)

add_library(csmi SHARED 
            ${csmi_SRC} )
target_link_libraries(csmi csmutil csm_network_c)

include_directories(${BASEDIR}/ "${BASEDIR}/csmnet/include")
install(TARGETS csmi COMPONENT csm-core DESTINATION csm/lib)

add_library(csmi_serialization SHARED ${csmi_serialization_SRC} )
target_link_libraries(csmi_serialization csmutil)
install(TARGETS csmi_serialization COMPONENT csm-core DESTINATION csm/lib)

add_subdirectory(launch/src)

file(GLOB csmi_python
    "common/src/*python.cc"
    "inv/src/*python.cc"
    "wm/src/*python.cc"
    "diag/src/*python.cc"
    "bb/src/*python.cc"
    "ras/src/*python.cc"
)

file(GLOB csmi_python_boot
    "common/src/lib_csm_py.py"
    "inv/src/lib_csm_inv_py.py"
    "wm/src/lib_csm_wm_py.py"
    "diag/src/lib_csm_diag_py.py"
    "bb/src/lib_csm_bb_py.py"
    "ras/src/lib_csm_ras_py.py"
)

# python
# The Python Libraries:
find_package(PythonLibs 2.7 )
message("Python Include: ${PYTHON_INCLUDE_DIRS}")
if (EXISTS ${PYTHON_INCLUDE_DIRS} )
    message("BUILDING PYTHON LIBRARIES")

    include_directories( SYSTEM ${PYTHON_INCLUDE_DIRS} )
    
    # The Boost Python Libraries:
    find_package( Boost COMPONENTS python REQUIRED )
    include_directories( SYSTEM ${Boost_INCLUDE_DIR} )
    
    # Link the library
    #add_library(_csm_py SHARED ${csmi_python})
    #target_link_libraries( _csm_py ${Boost_LIBRARIES} csmi)
    #install(TARGETS _csm_py COMPONENT csm-core DESTINATION csm/lib)
    #install(FILES ${csmi_python_boot} PERMISSIONS OWNER_READ COMPONENT csm-core DESTINATION csm/lib)
    
    add_library(_csm_py SHARED ${csmi_python})
    target_link_libraries( _csm_py ${Boost_LIBRARIES} csmi)
    install(TARGETS _csm_py COMPONENT csm-api DESTINATION csm/lib)
    install(FILES common/src/lib_csm_py.py PERMISSIONS OWNER_READ COMPONENT csm-api DESTINATION csm/lib)
    
    add_library(_csm_wm_py SHARED ${csmi_python})
    target_link_libraries( _csm_wm_py ${Boost_LIBRARIES} csmi)
    install(TARGETS _csm_wm_py COMPONENT csm-api DESTINATION csm/lib)
    install(FILES wm/src/lib_csm_wm_py.py PERMISSIONS OWNER_READ COMPONENT csm-api DESTINATION csm/lib)
    
    add_library(_csm_bb_py SHARED ${csmi_python})
    target_link_libraries( _csm_bb_py ${Boost_LIBRARIES} csmi)
    install(TARGETS _csm_bb_py COMPONENT csm-api DESTINATION csm/lib)
    install(FILES bb/src/lib_csm_bb_py.py PERMISSIONS OWNER_READ COMPONENT csm-api DESTINATION csm/lib)
    
    add_library(_csm_diag_py SHARED ${csmi_python})
    target_link_libraries( _csm_diag_py ${Boost_LIBRARIES} csmi)
    install(TARGETS _csm_diag_py COMPONENT csm-api DESTINATION csm/lib)
    install(FILES diag/src/lib_csm_diag_py.py PERMISSIONS OWNER_READ COMPONENT csm-api DESTINATION csm/lib)
    
    add_library(_csm_inv_py SHARED ${csmi_python})
    target_link_libraries( _csm_inv_py ${Boost_LIBRARIES} csmi)
    install(TARGETS _csm_inv_py COMPONENT csm-api DESTINATION csm/lib)
    install(FILES inv/src/lib_csm_inv_py.py PERMISSIONS OWNER_READ COMPONENT csm-api DESTINATION csm/lib)
    
    add_library(_csm_ras_py SHARED ${csmi_python})
    target_link_libraries( _csm_ras_py ${Boost_LIBRARIES} csmi)
    install(TARGETS _csm_ras_py COMPONENT csm-api DESTINATION csm/lib)
    install(FILES ras/src/lib_csm_ras_py.py PERMISSIONS OWNER_READ COMPONENT csm-api DESTINATION csm/lib)

endif()

