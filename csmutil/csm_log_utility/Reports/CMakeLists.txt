#================================================================================
#
#    csmutil/csm_log_utility/Reports/CMakeLists.txt
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

file(GLOB INSTALL_AGG_REPORTS
    "Aggregator_Reports/Aggregator_Reports.md"
)

file(GLOB INSTALL_C_REPORTS
    "Compute_Reports/Compute_Reports.md"
)

file(GLOB INSTALL_M_REPORTS
    "Master_Reports/Master_Reports.md"
)

file(GLOB INSTALL_U_REPORTS
    "Utility_Reports/Utility_Reports.md"
)

install(FILES ${INSTALL_AGG_REPORTS} COMPONENT csm-tools DESTINATION csm/tools/Reports/Aggregator_Reports)
install(FILES ${INSTALL_C_REPORTS} COMPONENT csm-tools DESTINATION csm/tools/Reports/Compute_Reports)
install(FILES ${INSTALL_M_REPORTS} COMPONENT csm-tools DESTINATION csm/tools/Reports/Master_Reports)
install(FILES ${INSTALL_U_REPORTS} COMPONENT csm-tools DESTINATION csm/tools/Reports/Utility_Reports)
