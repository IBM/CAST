/*================================================================================
   
    csmd/src/inv/include/inv_ssd_inventory.h

  © Copyright IBM Corporation 2017-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_SSD_INVENTORY_H
#define __INV_SSD_INVENTORY_H

#include <stdint.h>       // Provides int32_t

#include <string>

#include "csmi/src/inv/include/inv_types.h"

// Returns true if successful, false if an error occurred 
// If successful, ssd_count will contain the number of entries populated in the ssd_inventory array
bool GetSsdInventory(csm_ssd_inventory_t ssd_inventory[CSM_SSD_MAX_DEVICES], uint32_t& ssd_count);

// Returns true if successful, false if an error occurred 
// If successful: wear_lifespan_used, wear_percent_spares_remaining, wear_total_bytes_written, and wear_total_bytes_read
// will contain valid values.
// If unsuccessful: all fields will be set to -1
bool GetSsdWear(const std::string &devicename, int32_t &wear_lifespan_used, int32_t &wear_percent_spares_remaining,
                int64_t &wear_total_bytes_written, int64_t &wear_total_bytes_read);

#endif
