/*================================================================================

    csmi/src/inv/include/inv_types.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __INV_TYPES_H
#define __INV_TYPES_H

#include "csmi/include/csm_api_consts.h"

#include <stdint.h>
#include <string.h>

typedef struct csm_node_inventory_t
{
  char node_name[CSM_NODE_NAME_MAX];
  char machine_model[CSM_MACHINE_MODEL_MAX];
  char serial_number[CSM_SERIAL_NUMBER_MAX];
  char collection_time[CSM_COLLECTION_TIME_MAX];
  char type[CSM_TYPE_MAX];
  char kernel_release[CSM_KERNEL_RELEASE_MAX];
  char kernel_version[CSM_KERNEL_VERSION_MAX];
  uint64_t installed_memory;
  uint64_t installed_swap;
  uint32_t discovered_sockets;
  uint32_t discovered_cores;
  uint32_t discovered_gpus;
  uint32_t discovered_hcas;
  uint32_t discovered_dimms;
  uint32_t discovered_ssds;
  char os_image_name[CSM_OS_IMAGE_NAME_MAX];
  char os_image_uuid[CSM_OS_IMAGE_UUID_MAX];

  csm_node_inventory_t()
  {
    // Initialize the whole struct with 0s first; guarantees any pad bytes are also initialized
    memset(this, 0, sizeof(*this));

    // Set any non-zero defaults here
  }

} csm_node_inventory_t;

typedef struct csm_gpu_inventory_t
{
  uint32_t gpu_id;
  char device_name[CSM_GPU_DEVICE_NAME_MAX];
  char pci_bus_id[CSM_GPU_PCI_BUS_ID_MAX];
  char serial_number[CSM_GPU_SERIAL_NUMBER_MAX];
  char uuid[CSM_GPU_UUID_MAX];
  char vbios[CSM_GPU_VBIOS_MAX];
  char inforom_image_version[CSM_GPU_INFOROM_IMAGE_VERSION_MAX];
  char hbm_memory[CSM_GPU_HBM_MEMORY_SIZE_MAX];

  // Constructor 
  csm_gpu_inventory_t()
  { 
    // Initialize the whole struct with 0s first; guarantees any pad bytes are also initialized
    memset(this, 0, sizeof(*this));

    // Set any non-zero defaults here
  }

} csm_gpu_inventory_t;

typedef struct csm_dimm_inventory_t
{
  char serial_number[CSM_DIMM_SERIAL_NUMBER_MAX];
  uint32_t size;
  char physical_location[CSM_DIMM_PHYSICAL_LOCATION_MAX];
  
  // Constructor 
  csm_dimm_inventory_t()
  { 
    // Initialize the whole struct with 0s first; guarantees any pad bytes are also initialized
    memset(this, 0, sizeof(*this));

    // Set any non-zero defaults here
  }

} csm_dimm_inventory_t;

typedef struct csm_hca_inventory_t
{
  char serial_number[CSM_HCA_SERIAL_NUMBER_MAX];
  char device_name[CSM_HCA_DEVICE_NAME_MAX];
  char pci_bus_id[CSM_HCA_PCI_BUS_ID_MAX];
  char guid[CSM_HCA_GUID_MAX];
  char part_number[CSM_HCA_PART_NUMBER_MAX];
  char fw_ver[CSM_HCA_FW_VER_MAX];
  char hw_rev[CSM_HCA_HW_REV_MAX];
  char board_id[CSM_HCA_BOARD_ID_MAX];
  char switch_guid_port_1[CSM_HCA_SWITCH_GUID_MAX];
  char switch_guid_port_2[CSM_HCA_SWITCH_GUID_MAX];

  // Constructor 
  csm_hca_inventory_t()
  { 
    // Initialize the whole struct with 0s first; guarantees any pad bytes are also initialized
    memset(this, 0, sizeof(*this));

    // Set any non-zero defaults here
  }

} csm_hca_inventory_t;

typedef struct csm_ssd_inventory_t
{
  char serial_number[CSM_SSD_SERIAL_NUMBER_MAX];
  char device_name[CSM_SSD_DEVICE_NAME_MAX];
  char pci_bus_id[CSM_SSD_PCI_BUS_ID_MAX];
  char fw_ver[CSM_SSD_FW_VER_MAX];
  int64_t size;
  int32_t wear_lifespan_used; 
  int64_t wear_total_bytes_written;
  int64_t wear_total_bytes_read;
  int32_t wear_percent_spares_remaining;

  // Constructor 
  csm_ssd_inventory_t()
  { 
    // Initialize the whole struct with 0s first; guarantees any pad bytes are also initialized
    memset(this, 0, sizeof(*this));

    // Set any non-zero defaults here
    size = -1;
    wear_lifespan_used = -1; 
    wear_total_bytes_written = -1;
    wear_total_bytes_read = -1;
    wear_percent_spares_remaining = -1;
  } 

} csm_ssd_inventory_t;

typedef struct csm_full_inventory_t
{
  csm_node_inventory_t node;
  csm_dimm_inventory_t dimm[CSM_DIMM_MAX_DEVICES];
  csm_gpu_inventory_t gpu[CSM_GPU_MAX_DEVICES];
  csm_hca_inventory_t hca[CSM_HCA_MAX_DEVICES];
  csm_ssd_inventory_t ssd[CSM_SSD_MAX_DEVICES];
  
  // Constructor 
  csm_full_inventory_t() :
  node(),
  dimm(),
  gpu(),
  hca(),
  ssd()
  {}

} csm_full_inventory_t;

#endif
