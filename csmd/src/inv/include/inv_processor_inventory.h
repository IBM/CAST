/*================================================================================
   
    csmd/src/inv/include/inv_processor_inventory.h

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_PROCESSOR_INVENTORY_H
#define __INV_PROCESSOR_INVENTORY_H

#include <stdint.h>       // Provides int32_t

#include "csmi/src/inv/include/inv_types.h"

// Returns true if successful, false if an error occurred 
// If successful, processor_count will contain the number of entries populated in the processor_inventory array
bool GetProcessorInventory(csm_processor_inventory_t processor_inventory[CSM_PROCESSOR_MAX_DEVICES], uint32_t& processor_count);

#endif
