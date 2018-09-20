/*================================================================================

    csmi/include/csm_api_consts.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_API_CONSTS_H
#define __CSM_API_CONSTS_H

#include <limits.h>       // Provides HOST_NAME_MAX

/** @file csm_api_consts.h
 * @brief A collection constants for the CSM APIs.
 * This file should not be directly included, please include @ref csm_api_common.h instead.
 */

//////////////////////////////////////////////////////////////////////////
//  csm_bool datatype and constants.
//////////////////////////////////////////////////////////////////////////
#define CSM_FALSE       0
#define CSM_TRUE        1
#define CSM_UNDEF_BOOL  2

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_allocation table
//////////////////////////////////////////////////////////////////////////

// Valid values for csm_allocation.state 
#define CSM_ALLOCATION_STATE_RUNNING "running"
#define CSM_ALLOCATION_STATE_STAGING_IN "staging-in"
#define CSM_ALLOCATION_STATE_STAGING_OUT "staging-out"

// Valid values for csm_allocation.type
#define CSM_ALLOCATION_TYPE_SHARED "shared"
#define CSM_ALLOCATION_TYPE_USER_MANAGED "user-managed"
#define CSM_ALLOCATION_TYPE_PMIX "pmix"
#define CSM_ALLOCATION_TYPE_PMIX_WITH_CGROUPS "pmix-with-cgroups"
#define CSM_ALLOCATION_TYPE_DIAGNOSTICS "diagnostics"

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_diag_run table
//////////////////////////////////////////////////////////////////////////

// Maximum lengths of strings in the csm_diag_run table:
#define CSM_STATUS_MAX 16                           // csm_diag_run.status

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_node table
//////////////////////////////////////////////////////////////////////////

// Maximum lengths of strings in the csm_node table:
#define CSM_NODE_NAME_MAX HOST_NAME_MAX             // csm_node.node_name
#define CSM_STATE_MAX 32                            // csm_node.state
#define CSM_TYPE_MAX 24                             // csm_node.type 
#define CSM_MACHINE_MODEL_MAX 64                    // csm_node.machine_model
#define CSM_SERIAL_NUMBER_MAX 64                    // csm_node.serial_number
#define CSM_COLLECTION_TIME_MAX 32                  // csm_node.collection_time when formatted as a string
#define CSM_KERNEL_RELEASE_MAX 64                   // csm_node.kernel_release
#define CSM_KERNEL_VERSION_MAX 64                   // csm_node.kernel_version
#define CSM_OS_IMAGE_NAME_MAX 64                    // csm_node.os_image_name
#define CSM_OS_IMAGE_UUID_MAX 64                    // csm_node.os_image_uuid

// Valid values for csm_node.ready 
#define CSM_NODE_READY_YES 'y'
#define CSM_NODE_READY_NO 'n'

// Valid values for csm_node.state
// CSM_NODE_STATE_DISCOVERED is the initial state set by CSM inventory when the node is discovered for the first time
// CSM_NODE_STATE_IN_SERVICE is set by the sysadmin to make the node available for general cluster use
// CSM_NODE_STATE_SOFT_FAILURE is set by CSM RAS when a RAS event occurs with set_not_ready='t'
// CSM_NODE_STATE_OUT_OF_SERVICE is set by the sysadmin, possibly to indicate a node with a pending hardware repair
// CSM_NODE_STATE_MAINTENANCE is set by the sysadmin to remove a node from the scheduler and stop processing of RAS events
// CSM_NODE_STATE_ADMIN_RESERVED is set by the sysadmin to remove a node from the scheduler and allow processing of RAS events
//
// CSM internal states that can only be set by CSM: 
// CSM_NODE_STATE_DISCOVERED, CSM_NODE_STATE_SOFT_FAILURE
//
// Valid states that can be set by system admins:
// CSM_NODE_STATE_IN_SERVICE, CSM_NODE_STATE_OUT_OF_SERVICE, CSM_NODE_STATE_MAINTENANCE, CSM_NODE_STATE_ADMIN_RESERVED
//
// RAS events will not be processed for nodes in these states: 
// CSM_NODE_STATE_OUT_OF_SERVICE, CSM_NODE_STATE_MAINTENANCE 
//
#define CSM_NODE_STATE_DISCOVERED "DISCOVERED"                
#define CSM_NODE_STATE_IN_SERVICE "IN_SERVICE"
#define CSM_NODE_STATE_SOFT_FAILURE "SOFT_FAILURE"
#define CSM_NODE_STATE_OUT_OF_SERVICE "OUT_OF_SERVICE"
#define CSM_NODE_STATE_MAINTENANCE "MAINTENANCE"
#define CSM_NODE_STATE_ADMIN_RESERVED "ADMIN_RESERVED"

// Valid values for csm_node.type
#define CSM_NODE_TYPE_MANAGEMENT "management"
#define CSM_NODE_TYPE_SERVICE "service"
#define CSM_NODE_TYPE_LOGIN "login"
#define CSM_NODE_TYPE_WORKLOAD_MANAGER "workload-manager"
#define CSM_NODE_TYPE_LAUNCH "launch"
#define CSM_NODE_TYPE_COMPUTE "compute"
#define CSM_NODE_TYPE_UTILITY "utility"
#define CSM_NODE_TYPE_AGGREGATOR "aggregator"

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_dimm table
//////////////////////////////////////////////////////////////////////////

// Maximum lengths of strings in the csm_dimm table:
#define CSM_DIMM_MAX_DEVICES 20                     // Maximum number of DIMMs per server
#define CSM_DIMM_SERIAL_NUMBER_MAX 16               // csm_dimm.serial_number
#define CSM_DIMM_PHYSICAL_LOCATION_MAX 40           // csm_dimm.physical_location

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_gpu table
//////////////////////////////////////////////////////////////////////////

// Maximum lengths of strings in the csm_gpu table:
#define CSM_GPU_MAX_DEVICES 8                       // Maximum number of GPUs per server
#define CSM_GPU_DEVICE_NAME_MAX 32                  // csm_gpu.device_name
#define CSM_GPU_PCI_BUS_ID_MAX 32                   // csm_gpu.pci_bus_id
#define CSM_GPU_SERIAL_NUMBER_MAX 32                // csm_gpu.serial_number
#define CSM_GPU_UUID_MAX 64                         // csm_gpu.uuid
#define CSM_GPU_VBIOS_MAX 32                        // csm_gpu.vbios
#define CSM_GPU_INFOROM_IMAGE_VERSION_MAX 32        // csm_gpu.inforom_image_version
#define CSM_GPU_HBM_MEMORY_SIZE_MAX 6               // csm_gpu.hbm_memory

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_hca table
//////////////////////////////////////////////////////////////////////////

// Maximum lengths of strings in the csm_hca table:
#define CSM_HCA_MAX_DEVICES 2                     // Maximum number of HCAs per server
#define CSM_HCA_SERIAL_NUMBER_MAX 20              // csm_hca.serial_number
#define CSM_HCA_DEVICE_NAME_MAX 40                // csm_hca.device_name
#define CSM_HCA_PCI_BUS_ID_MAX 16                 // csm_hca.pci_bus_id
#define CSM_HCA_GUID_MAX 20                       // csm_hcm.guid
#define CSM_HCA_PART_NUMBER_MAX 12                // csm_hca.part_number
#define CSM_HCA_FW_VER_MAX 16                     // csm_hca.fw_ver
#define CSM_HCA_HW_REV_MAX 8                      // csm_hca.hw_rev
#define CSM_HCA_BOARD_ID_MAX 16                   // csm_hcm.board_id
#define CSM_HCA_SWITCH_GUID_MAX 20                // csm_hca.switch_guid_port_X

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_processor_socket table
//////////////////////////////////////////////////////////////////////////

// Maximum lengths of strings in the csm_processor_socket table:
#define CSM_PROCESSOR_MAX_DEVICES 4                 // Maximum number of processor sockets per server
#define CSM_PROCESSOR_SERIAL_NUMBER_MAX 16          // csm_processor_socket.serial_number
#define CSM_PROCESSOR_PHYSICAL_LOCATION_MAX 40      // csm_processor_socket.physical_location

//////////////////////////////////////////////////////////////////////////
// Constants related to the csm_ssd table
//////////////////////////////////////////////////////////////////////////

// Maximum lengths of strings in the csm_ssd table:
#define CSM_SSD_MAX_DEVICES 2                     // Maximum number of SSDs per server
#define CSM_SSD_SERIAL_NUMBER_MAX 20              // csm_ssd.serial_number
#define CSM_SSD_DEVICE_NAME_MAX 40                // csm_ssd.device_name
#define CSM_SSD_PCI_BUS_ID_MAX 16                 // csm_ssd.pci_bus_id
#define CSM_SSD_FW_VER_MAX 16                     // csm_ssd.fw_ver


#endif
