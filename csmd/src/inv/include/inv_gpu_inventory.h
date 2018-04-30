/*================================================================================
   
    csmd/src/inv/include/inv_gpu_inventory.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_GPU_INVENTORY_H
#define __INV_GPU_INVENTORY_H

#include <stdint.h>       // Provides int32_t

#include "csmi/src/inv/include/inv_types.h"

// Returns true if successful, false if an error occurred 
// If successful, gpu_count will contain the number of entries populated in the gpu_inventory array
bool GetGpuInventory(csm_gpu_inventory_t gpu_inventory[CSM_GPU_MAX_DEVICES], uint32_t& gpu_count);

// Used only for unit test, return valid data on systems that do not have GPUs
bool GetGpuInventoryUnitTestData(csm_gpu_inventory_t gpu_inventory[CSM_GPU_MAX_DEVICES], uint32_t& gpu_count);

#endif
