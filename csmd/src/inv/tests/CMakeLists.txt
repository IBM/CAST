#================================================================================
#   
#    csmd/src/inv/tests/CMakeLists.txt
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
include (${CMAKE_SOURCE_DIR}/csmd/csmd.cmake)

file(GLOB TEST_INV_GET_NODE_INVENTORY
  ../src/inv_get_node_inventory.cc
  test_inv_get_node_inventory.cc 
)

add_executable(test_inv_get_node_inventory ${TEST_INV_GET_NODE_INVENTORY})
target_link_libraries(test_inv_get_node_inventory csmd_lib csm_network_cc csmi_serialization csmutil fsutil -lpthread -lpq)
install(TARGETS test_inv_get_node_inventory COMPONENT csm-unittest DESTINATION csm/tests/inv)

add_definitions(-DUSE_SC_LOGGER -DBOOST_SYSTEM_NO_DEPRECATED)

file(GLOB TEST_INV_GPU_INVENTORY
  ../src/inv_gpu_inventory.cc
  ../src/inv_dcgm_access.cc
  test_inv_gpu_inventory.cc 
)

add_executable(test_inv_gpu_inventory ${TEST_INV_GPU_INVENTORY})
target_link_libraries(test_inv_gpu_inventory csmd_lib csmi_serialization csmutil fsutil -lpthread -ldl)
install(TARGETS test_inv_gpu_inventory COMPONENT csm-unittest DESTINATION csm/tests/inv)

file(GLOB TEST_INV_DIMM_INVENTORY
  ../src/inv_dimm_inventory.cc
  test_inv_dimm_inventory.cc 
)

add_executable(test_inv_dimm_inventory ${TEST_INV_DIMM_INVENTORY})
target_link_libraries(test_inv_dimm_inventory csmutil fsutil)
install(TARGETS test_inv_dimm_inventory COMPONENT csm-unittest DESTINATION csm/tests/inv)

file(GLOB TEST_INV_HCA_INVENTORY
  ../src/inv_hca_inventory.cc
  test_inv_hca_inventory.cc 
)

add_executable(test_inv_hca_inventory ${TEST_INV_HCA_INVENTORY})
target_link_libraries(test_inv_hca_inventory csmutil fsutil)
install(TARGETS test_inv_hca_inventory COMPONENT csm-unittest DESTINATION csm/tests/inv)

file(GLOB TEST_INV_SSD_INVENTORY
  ../src/inv_ssd_inventory.cc
  test_inv_ssd_inventory.cc 
)

add_executable(test_inv_ssd_inventory ${TEST_INV_SSD_INVENTORY})
target_link_libraries(test_inv_ssd_inventory csmutil fsutil)
install(TARGETS test_inv_ssd_inventory COMPONENT csm-unittest DESTINATION csm/tests/inv)

file(GLOB TEST_INV_IB_GUID
  test_inv_ib_guid.cc 
)

add_executable(test_inv_ib_guid ${TEST_INV_IB_GUID})
target_link_libraries(test_inv_ib_guid csmutil fsutil)
install(TARGETS test_inv_ib_guid COMPONENT csm-unittest DESTINATION csm/tests/inv)

file(GLOB TEST_INV_FULL_INVENTORY
  ../src/inv_full_inventory.cc
  ../src/inv_get_node_inventory.cc
  ../src/inv_dimm_inventory.cc
  ../src/inv_gpu_inventory.cc
  ../src/inv_dcgm_access.cc
  test_inv_full_inventory.cc 
)

add_executable(test_inv_full_inventory ${TEST_INV_FULL_INVENTORY})
target_link_libraries(test_inv_full_inventory csmd_lib csmi_serialization csmutil fsutil)
install(TARGETS test_inv_full_inventory COMPONENT csm-unittest DESTINATION csm/tests/inv)

include_directories(
  ${CSMD_INCLUDE_DIRS}
)
