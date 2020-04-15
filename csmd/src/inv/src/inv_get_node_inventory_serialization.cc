/*================================================================================
    
    csmd/src/inv/src/inv_get_node_inventory_serialization.cc 

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_get_node_inventory_serialization.h"
#include "logging.h"

#include <string>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using std::string;

//static csmi_cmd_t ExpectedCmd = CSM_CMD_INV_get_node_inventory;

uint32_t get_node_inventory_pack(const csm_full_inventory_t& in_inventory, string& out_payload_str)
{
  // Pack the node portion of the inventory
  out_payload_str.append( (char*) &in_inventory.node, sizeof(in_inventory.node) );
  //LOG(csmd, info) << "sizeof(in_inventory.node) = " << sizeof(in_inventory.node);
  
  // Pack the dimm portion of the inventory 
  if ( in_inventory.node.discovered_dimms > CSM_DIMM_MAX_DEVICES )
  {
    LOG(csmd, error) << "in_inventory.node.discovered_dimms (" << in_inventory.node.discovered_dimms << ") " 
                     << "> CSM_DIMM_MAX_DEVICES (" << CSM_DIMM_MAX_DEVICES << ")";

    out_payload_str.clear();
    LOG(csmd, info) << "out_payload_str.length() = " << out_payload_str.length();
    return out_payload_str.length();
  }

  // Only pack populated DIMMs
  for ( uint32_t i = 0; i < in_inventory.node.discovered_dimms; i++ )
  {
    out_payload_str.append( (char*) &(in_inventory.dimm[i]), sizeof(in_inventory.dimm[i]) );
    //LOG(csmd, info) << "sizeof(in_inventory.dimm[i]) = " << sizeof(in_inventory.dimm[i]);
  } 

  // Pack the gpu portion of the inventory 
  if ( in_inventory.node.discovered_gpus > CSM_GPU_MAX_DEVICES )
  {
    LOG(csmd, error) << "in_inventory.node.discovered_gpus (" << in_inventory.node.discovered_gpus << ") " 
                     << "> CSM_GPU_MAX_DEVICES (" << CSM_GPU_MAX_DEVICES << ")";

    out_payload_str.clear();
    LOG(csmd, info) << "out_payload_str.length() = " << out_payload_str.length();
    return out_payload_str.length();
  }

  // Only pack populated GPUs
  for ( uint32_t i = 0; i < in_inventory.node.discovered_gpus; i++ )
  {
    out_payload_str.append( (char*) &(in_inventory.gpu[i]), sizeof(in_inventory.gpu[i]) );
    //LOG(csmd, info) << "sizeof(in_inventory.gpu[i]) = " << sizeof(in_inventory.gpu[i]);
  }
 
  // Pack the hca portion of the inventory 
  if ( in_inventory.node.discovered_hcas > CSM_HCA_MAX_DEVICES )
  {
    LOG(csmd, error) << "in_inventory.node.discovered_hcas (" << in_inventory.node.discovered_hcas << ") " 
                     << "> CSM_HCA_MAX_DEVICES (" << CSM_HCA_MAX_DEVICES << ")";

    out_payload_str.clear();
    LOG(csmd, info) << "out_payload_str.length() = " << out_payload_str.length();
    return out_payload_str.length();
  }

  // Only pack populated HCAs
  for ( uint32_t i = 0; i < in_inventory.node.discovered_hcas; i++ )
  {
    out_payload_str.append( (char*) &(in_inventory.hca[i]), sizeof(in_inventory.hca[i]) );
    //LOG(csmd, info) << "sizeof(in_inventory.hca[i]) = " << sizeof(in_inventory.hca[i]);
  }
  
  // Pack the ssd portion of the inventory 
  if ( in_inventory.node.discovered_ssds > CSM_SSD_MAX_DEVICES )
  {
    LOG(csmd, error) << "in_inventory.node.discovered_ssds (" << in_inventory.node.discovered_ssds << ") " 
                     << "> CSM_SSD_MAX_DEVICES (" << CSM_SSD_MAX_DEVICES << ")";

    out_payload_str.clear();
    LOG(csmd, info) << "out_payload_str.length() = " << out_payload_str.length();
    return out_payload_str.length();
  }

  // Only pack populated SSDs
  for ( uint32_t i = 0; i < in_inventory.node.discovered_ssds; i++ )
  {
    out_payload_str.append( (char*) &(in_inventory.ssd[i]), sizeof(in_inventory.ssd[i]) );
    //LOG(csmd, info) << "sizeof(in_inventory.ssd[i]) = " << sizeof(in_inventory.ssd[i]);
  }
  
  // Pack the processor_count
  out_payload_str.append( (char*) &(in_inventory.processor_count), sizeof(in_inventory.processor_count) );
  
  // Pack the processor portion of the inventory 
  if ( in_inventory.processor_count > CSM_PROCESSOR_MAX_DEVICES )
  {
    LOG(csmd, error) << "in_inventory.processor_count (" << in_inventory.processor_count << ") " 
                     << "> CSM_PROCESSOR_MAX_DEVICES (" << CSM_PROCESSOR_MAX_DEVICES << ")";

    out_payload_str.clear();
    LOG(csmd, info) << "out_payload_str.length() = " << out_payload_str.length();
    return out_payload_str.length();
  }

  // Only pack populated processors
  for ( uint32_t i = 0; i < in_inventory.processor_count; i++ )
  {
    out_payload_str.append( (char*) &(in_inventory.processor[i]), sizeof(in_inventory.processor[i]) );
    //LOG(csmd, info) << "sizeof(in_inventory.processor[i]) = " << sizeof(in_inventory.processor[i]);
  }

  LOG(csmd, info) << "out_payload_str.length() = " << out_payload_str.length();

  return out_payload_str.length();
}

uint32_t get_node_inventory_unpack(const string& in_payload_str, csm_full_inventory_t& out_inventory)
{
  uint32_t offset(0);  

  // Clear the current structure
  memset( static_cast<void*>(&out_inventory), 0, sizeof(out_inventory) );
  //LOG(csmd, info) << "sizeof(out_inventory) = " << sizeof(out_inventory);

  // Unpack the node portion of the inventory
  if ( in_payload_str.size() >= sizeof(out_inventory.node) )
  {
    memcpy( &out_inventory.node, &(in_payload_str.c_str()[offset]), sizeof(out_inventory.node) );
    offset += sizeof(out_inventory.node);
    //LOG(csmd, info) << "sizeof(out_inventory.node) = " << sizeof(out_inventory.node) << ", offset = " << offset;
  }
  else
  {
    LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                     << "< sizeof(out_inventory.node) (" << sizeof(out_inventory.node) << ")";
  }
  
  // Unpack the dimm portion of the inventory
  if ( out_inventory.node.discovered_dimms > CSM_DIMM_MAX_DEVICES )
  {
    LOG(csmd, error) << "out_inventory.node.discovered_dimms (" << out_inventory.node.discovered_dimms << ") " 
                     << "> CSM_DIMM_MAX_DEVICES (" << CSM_DIMM_MAX_DEVICES << "), truncating array to fit";

    out_inventory.node.discovered_dimms = CSM_DIMM_MAX_DEVICES;
  }

  for ( uint32_t i = 0; i < out_inventory.node.discovered_dimms; i++ )
  {
    if ( in_payload_str.size() >= (offset + sizeof(out_inventory.dimm[i])) )
    {
      memcpy( &out_inventory.dimm[i], &(in_payload_str.c_str()[offset]), sizeof(out_inventory.dimm[i]) );
      offset += sizeof(out_inventory.dimm[i]);
      //LOG(csmd, info) << "sizeof(out_inventory.dimm[i]) = " << sizeof(out_inventory.dimm[i])
      //                << ", offset = " << offset;
    }
    else
    {
      LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                       << "< offset (" << offset << ") + sizeof(out_inventory.dimm[i]) (" 
                       << sizeof(out_inventory.dimm[i]) << ")";
   
    }
  }
  
  // Unpack the gpu portion of the inventory
  if ( out_inventory.node.discovered_gpus > CSM_GPU_MAX_DEVICES )
  {
    LOG(csmd, error) << "out_inventory.node.discovered_gpus (" << out_inventory.node.discovered_gpus << ") " 
                     << "> CSM_GPU_MAX_DEVICES (" << CSM_GPU_MAX_DEVICES << "), truncating array to fit";

    out_inventory.node.discovered_gpus = CSM_GPU_MAX_DEVICES;
  }

  for ( uint32_t i = 0; i < out_inventory.node.discovered_gpus; i++ )
  {
    if ( in_payload_str.size() >= (offset + sizeof(out_inventory.gpu[i])) )
    {
      memcpy( &out_inventory.gpu[i], &(in_payload_str.c_str()[offset]), sizeof(out_inventory.gpu[i]) );
      offset += sizeof(out_inventory.gpu[i]);
      //LOG(csmd, info) << "sizeof(out_inventory.gpu[i]) = " << sizeof(out_inventory.gpu[i])
      //                << ", offset = " << offset;
    }
    else
    {
      LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                       << "< offset (" << offset << ") + sizeof(out_inventory.gpu[i]) (" 
                       << sizeof(out_inventory.gpu[i]) << ")";
   
    }
  }
  
  // Unpack the hca portion of the inventory
  if ( out_inventory.node.discovered_hcas > CSM_HCA_MAX_DEVICES )
  {
    LOG(csmd, error) << "out_inventory.node.discovered_hcas (" << out_inventory.node.discovered_hcas << ") " 
                     << "> CSM_HCA_MAX_DEVICES (" << CSM_HCA_MAX_DEVICES << "), truncating array to fit";

    out_inventory.node.discovered_hcas = CSM_HCA_MAX_DEVICES;
  }

  for ( uint32_t i = 0; i < out_inventory.node.discovered_hcas; i++ )
  {
    if ( in_payload_str.size() >= (offset + sizeof(out_inventory.hca[i])) )
    {
      memcpy( &out_inventory.hca[i], &(in_payload_str.c_str()[offset]), sizeof(out_inventory.hca[i]) );
      offset += sizeof(out_inventory.hca[i]);
      //LOG(csmd, info) << "sizeof(out_inventory.hca[i]) = " << sizeof(out_inventory.hca[i])
      //                << ", offset = " << offset;
    }
    else
    {
      LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                       << "< offset (" << offset << ") + sizeof(out_inventory.hca[i]) (" 
                       << sizeof(out_inventory.hca[i]) << ")";
    }
  }
  
  // Unpack the ssd portion of the inventory
  if ( out_inventory.node.discovered_ssds > CSM_SSD_MAX_DEVICES )
  {
    LOG(csmd, error) << "out_inventory.node.discovered_ssds (" << out_inventory.node.discovered_ssds << ") " 
                     << "> CSM_SSD_MAX_DEVICES (" << CSM_SSD_MAX_DEVICES << "), truncating array to fit";

    out_inventory.node.discovered_ssds = CSM_SSD_MAX_DEVICES;
  }

  for ( uint32_t i = 0; i < out_inventory.node.discovered_ssds; i++ )
  {
    if ( in_payload_str.size() >= (offset + sizeof(out_inventory.ssd[i])) )
    {
      memcpy( &out_inventory.ssd[i], &(in_payload_str.c_str()[offset]), sizeof(out_inventory.ssd[i]) );
      offset += sizeof(out_inventory.ssd[i]);
      //LOG(csmd, info) << "sizeof(out_inventory.ssd[i]) = " << sizeof(out_inventory.ssd[i])
      //                << ", offset = " << offset;
    }
    else
    {
      LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                       << "< offset (" << offset << ") + sizeof(out_inventory.ssd[i]) (" 
                       << sizeof(out_inventory.ssd[i]) << ")";
    }
  }

  // Unpack the processor count
  if ( in_payload_str.size() >= (offset + sizeof(out_inventory.processor_count)) )
  {
    memcpy( &out_inventory.processor_count, &(in_payload_str.c_str()[offset]), sizeof(out_inventory.processor_count) );
    offset += sizeof(out_inventory.processor_count);
    //LOG(csmd, info) << "sizeof(out_inventory.processor_count) = " << sizeof(out_inventory.processor_count)
    //                << ", offset = " << offset;
  }
  else
  {
    LOG(csmd, warning) << "No processor_count received, probable version mismatch: in_payload_str.size() (" << in_payload_str.size() << ") " 
                       << "< offset (" << offset << ") + sizeof(out_inventory.processor_count) (" 
                       << sizeof(out_inventory.processor_count) << ")";
    out_inventory.processor_count = 0;
  }
  
  // Unpack the processor portion of the inventory
  if ( out_inventory.processor_count > CSM_PROCESSOR_MAX_DEVICES )
  {
    LOG(csmd, error) << "out_inventory.processor_count (" << out_inventory.processor_count << ") " 
                     << "> CSM_PROCESSOR_MAX_DEVICES (" << CSM_PROCESSOR_MAX_DEVICES << "), truncating array to fit";

    out_inventory.processor_count = CSM_PROCESSOR_MAX_DEVICES;
  }

  for ( uint32_t i = 0; i < out_inventory.processor_count; i++ )
  {
    if ( in_payload_str.size() >= (offset + sizeof(out_inventory.processor[i])) )
    {
      memcpy( &out_inventory.processor[i], &(in_payload_str.c_str()[offset]), sizeof(out_inventory.processor[i]) );
      offset += sizeof(out_inventory.processor[i]);
      //LOG(csmd, info) << "sizeof(out_inventory.processor[i]) = " << sizeof(out_inventory.processor[i])
      //                << ", offset = " << offset;
    }
    else
    {
      LOG(csmd, error) << "in_payload_str.size() (" << in_payload_str.size() << ") " 
                       << "< offset (" << offset << ") + sizeof(out_inventory.processor[i]) (" 
                       << sizeof(out_inventory.processor[i]) << ")";
    }
  }

  return offset;
}

