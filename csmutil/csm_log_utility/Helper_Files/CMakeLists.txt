#================================================================================
#
#    csmutil/csm_log_utility/Helper_Files/CMakeLists.txt
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

file(GLOB INSTALL_HELPER_SCRIPTS
    "Master_Stat.py"
    "Aggregator_Stat.py"
    "Utility_Stat.py"
    "Computer_Stat.py"
    "Log_Object.py"
)

install(FILES ${INSTALL_HELPER_SCRIPTS} COMPONENT csm-core DESTINATION csm/tools/Helper_Files)