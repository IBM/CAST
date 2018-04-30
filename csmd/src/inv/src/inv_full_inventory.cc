/*================================================================================
   
    csmd/src/inv/src/inv_full_inventory.cc

  © Copyright IBM Corporation 2017-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_full_inventory.h"
#include "inv_get_node_inventory.h"
#include "inv_dimm_inventory.h"
#include "inv_gpu_inventory.h"
#include "inv_hca_inventory.h"
#include "inv_ssd_inventory.h"
#include "logging.h"

using namespace std;

bool GetFullInventory(csm_full_inventory_t& inventory)
{
  LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__;

  bool rc(false);
  rc = GetNodeInventory(inventory.node);
  if (rc == false)
  {
    LOG(csmd, error) << "GetNodeInventory() returned false.";
    return rc;
  }
  
  rc = GetDimmInventory(inventory.dimm, inventory.node.discovered_dimms);
  if (rc == false)
  {
    LOG(csmd, warning) << "GetDimmInventory() returned false.";
  }

  rc = GetGpuInventory(inventory.gpu, inventory.node.discovered_gpus);
  if (rc == false)
  {
    LOG(csmd, warning) << "GetGpuInventory() returned false.";
  }
  
  rc = GetHcaInventory(inventory.hca, inventory.node.discovered_hcas);
  if (rc == false)
  {
    LOG(csmd, warning) << "GetHcaInventory() returned false.";
  }
  
  rc = GetSsdInventory(inventory.ssd, inventory.node.discovered_ssds);
  if (rc == false)
  {
    LOG(csmd, warning) << "GetSsdInventory() returned false.";
  }

  return true;
}
