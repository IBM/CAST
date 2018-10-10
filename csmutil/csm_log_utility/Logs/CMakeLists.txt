#================================================================================
#
#    csmutil/csm_log_utility/Logs/CMakeLists.txt
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

file(GLOB INSTALL_A_Logs
    "Aggregator/ReadMe.md"
)

file(GLOB INSTALL_C_Logs
    "Compute/ReadMe.md"
)

file(GLOB INSTALL_M_Logs
    "Master/ReadMe.md"
)

file(GLOB INSTALL_U_Logs
    "Utility/ReadMe.md"
)

install(FILES ${INSTALL_A_Logs} COMPONENT csm-core DESTINATION csm/tools/Logs/Aggregator)
install(FILES ${INSTALL_C_Logs} COMPONENT csm-core DESTINATION csm/tools/Logs/Compute)
install(FILES ${INSTALL_M_Logs} COMPONENT csm-core DESTINATION csm/tools/Logs/Master)
install(FILES ${INSTALL_U_Logs} COMPONENT csm-core DESTINATION csm/tools/Logs/Utility)
