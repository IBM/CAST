/*================================================================================
    
    csmd/src/inv/include/inv_ssd_wear_serialization.h    

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef _INV_SSD_WEAR_SERIALIZATION_H
#define _INV_SSD_WEAR_SERIALIZATION_H

#include "csmi/src/inv/include/inv_types.h"

#include <string>

using std::string;

uint32_t ssd_wear_pack(const csm_ssd_wear_t& in_ssd_wear, string& out_payload_str);

uint32_t ssd_wear_unpack(const string& in_payload_str, csm_ssd_wear_t& out_ssd_wear);

#endif
