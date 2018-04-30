/*================================================================================
   
    csmd/src/inv/include/inv_get_node_inventory.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __INV_GET_NODE_INVENTORY_H
#define __INV_GET_NODE_INVENTORY_H

#include <stdint.h>       // Provides int32_t

#include "csmi/src/common/include/csmi_cmds.h"
#include "csmi/src/inv/include/inv_types.h"

// Fills in the node_inventory structure 
// Returns true on success, false on error
bool GetNodeInventory(csm_node_inventory_t& node_inventory);

csmi_cmd_t GetCommandNodeInventory();

// return the length of the payload
// 0 if error occurs
int GetPayloadNodeInventory(csm_node_inventory_t *node, char **payload);
#endif
