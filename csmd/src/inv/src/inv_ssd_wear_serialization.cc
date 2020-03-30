/*================================================================================
    
    csmd/src/inv/src/inv_ssd_wear_serialization.cc 

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_ssd_wear_serialization.h"
#include "logging.h"

#include <string>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using std::string;

uint32_t ssd_wear_pack(const csm_ssd_wear_t& in_ssd_wear, string& out_payload_str)
{
  // Pack the node name 
  out_payload_str.append( (char*) &in_ssd_wear.node_name, sizeof(in_ssd_wear.node_name) );
  
  // Pack the ssd portion of the inventory 
  if ( in_ssd_wear.discovered_ssds > CSM_SSD_MAX_DEVICES )
  {
    LOG(csmd, error) << "in_ssd_wear.discovered_ssds (" << in_ssd_wear.discovered_ssds << ") " 
                     << "> CSM_SSD_MAX_DEVICES (" << CSM_SSD_MAX_DEVICES << ")";

    out_payload_str.clear();
    LOG(csmd, debug) << "out_payload_str.length() = " << out_payload_str.length();
    return out_payload_str.length();
  }
 
  // Pack the count of discovered_ssds 
  out_payload_str.append( (char*) &in_ssd_wear.discovered_ssds, sizeof(in_ssd_wear.discovered_ssds) );

  // Only pack populated SSDs
  for ( uint32_t i = 0; i < in_ssd_wear.discovered_ssds; i++ )
  {
    out_payload_str.append( (char*) &(in_ssd_wear.ssd[i]), sizeof(in_ssd_wear.ssd[i]) );
  }
  
  LOG(csmd, debug) << "out_payload_str.length() = " << out_payload_str.length();

  return out_payload_str.length();
}

uint32_t ssd_wear_unpack(const string& in_payload_str, csm_ssd_wear_t& out_ssd_wear)
{
  uint32_t offset(0);  

  // Clear the current structure
  memset( static_cast<void*>(&out_ssd_wear), 0, sizeof(out_ssd_wear) );

  // Unpack the node name
  if ( in_payload_str.size() >= sizeof(out_ssd_wear.node_name) )
  {
    memcpy( &out_ssd_wear.node_name, &(in_payload_str.c_str()[offset]), sizeof(out_ssd_wear.node_name) );
    offset += sizeof(out_ssd_wear.node_name);
  }
  else
  {
    LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                     << "< sizeof(out_ssd_wear.node_name) (" << sizeof(out_ssd_wear.node_name) << ")";
  }
  
  // Unpack the count of discovered_ssds 
  if ( in_payload_str.size() >= sizeof(out_ssd_wear.discovered_ssds) )
  {
    memcpy( &out_ssd_wear.discovered_ssds, &(in_payload_str.c_str()[offset]), sizeof(out_ssd_wear.discovered_ssds) );
    offset += sizeof(out_ssd_wear.discovered_ssds);
  }
  else
  {
    LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                     << "< sizeof(out_ssd_wear.node_name) (" << sizeof(out_ssd_wear.node_name) << ")";
  }
  
  // Unpack the ssd portion of the message 
  if ( out_ssd_wear.discovered_ssds > CSM_SSD_MAX_DEVICES )
  {
    LOG(csmd, error) << "out_ssd_wear.discovered_ssds (" << out_ssd_wear.discovered_ssds << ") " 
                     << "> CSM_SSD_MAX_DEVICES (" << CSM_SSD_MAX_DEVICES << "), truncating array to fit";

    out_ssd_wear.discovered_ssds = CSM_SSD_MAX_DEVICES;
  }

  for ( uint32_t i = 0; i < out_ssd_wear.discovered_ssds; i++ )
  {
    if ( in_payload_str.size() >= (offset + sizeof(out_ssd_wear.ssd[i])) )
    {
      memcpy( &out_ssd_wear.ssd[i], &(in_payload_str.c_str()[offset]), sizeof(out_ssd_wear.ssd[i]) );
      offset += sizeof(out_ssd_wear.ssd[i]);
    }
    else
    {
      LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                       << "< offset (" << offset << ") + sizeof(out_ssd_wear.ssd[i]) (" 
                       << sizeof(out_ssd_wear.ssd[i]) << ")";
    }
  }

  return offset;
}

