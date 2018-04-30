#================================================================================
#   
#    csmd/src/inv/src/CMakeLists.txt
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
include (${CMAKE_SOURCE_DIR}/csmd/csmd.cmake)

set(CSMD_INV_SRC_DIR
  ${CMAKE_SOURCE_DIR}/csmd/src/inv/src
)

# Define the list of files needed to build the csmd inventory code
# Used by csmd/src/daemon/src/CMakeLists.txt to add the csmd inventory code to csmd
set(CSM_INVENTORY_SRC
  ${CSMD_INV_SRC_DIR}/inv_full_inventory.cc
  ${CSMD_INV_SRC_DIR}/inv_get_node_inventory.cc
  ${CSMD_INV_SRC_DIR}/inv_get_node_inventory_serialization.cc
  ${CSMD_INV_SRC_DIR}/inv_dimm_inventory.cc
  ${CSMD_INV_SRC_DIR}/inv_gpu_inventory.cc
  ${CSMD_INV_SRC_DIR}/inv_hca_inventory.cc
  ${CSMD_INV_SRC_DIR}/inv_ssd_inventory.cc
  ${CSMD_INV_SRC_DIR}/inv_agent_handler.cc
  ${CSMD_INV_SRC_DIR}/inv_aggregator_handler.cc
  ${CSMD_INV_SRC_DIR}/inv_master_handler.cc
  ${CSMD_INV_SRC_DIR}/inv_dcgm_access.cc
)
