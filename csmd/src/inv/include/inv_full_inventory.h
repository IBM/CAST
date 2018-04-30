/*================================================================================
   
    csmd/src/inv/include/inv_full_inventory.h

  © Copyright IBM Corporation 2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_FULL_INVENTORY_H
#define __INV_FULL_INVENTORY_H

#include "csmi/src/inv/include/inv_types.h"

// Returns true if successful, false if an error occurred 
// If successful, full_inventory will contain all that CSM inventory information that is currently available
bool GetFullInventory(csm_full_inventory_t& full_inventory);

#endif
