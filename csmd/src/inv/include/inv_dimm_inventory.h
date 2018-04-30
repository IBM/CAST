/*================================================================================
   
    csmd/src/inv/include/inv_dimm_inventory.h

  © Copyright IBM Corporation 2017-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_DIMM_INVENTORY_H
#define __INV_DIMM_INVENTORY_H

#include <stdint.h>       // Provides int32_t

#include "csmi/src/inv/include/inv_types.h"

// Returns true if successful, false if an error occurred 
// If successful, dimm_count will contain the number of entries populated in the dimm_inventory array
bool GetDimmInventory(csm_dimm_inventory_t dimm_inventory[CSM_DIMM_MAX_DEVICES], uint32_t& dimm_count);

#endif
