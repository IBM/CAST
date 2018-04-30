/*================================================================================
    
    csmd/src/inv/include/inv_get_node_inventory_serialization.h    

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef _INV_GET_NODE_INVENTORY_SERIALIZATION_H
#define _INV_GET_NODE_INVENTORY_SERIALIZATION_H

#include "csmi/src/inv/include/inv_types.h"

#include <string>

using std::string;

uint32_t get_node_inventory_pack(const csm_full_inventory_t& in_inventory, string& out_payload_str);

uint32_t get_node_inventory_unpack(const string& in_payload_str, csm_full_inventory_t& out_inventory);

#endif
