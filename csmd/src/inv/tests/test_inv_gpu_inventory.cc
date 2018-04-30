/*================================================================================
   
    csmd/src/inv/tests/test_inv_gpu_inventory.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <iostream>

#include "inv_gpu_inventory.h"

using std::cout;
using std::endl;

int main(void)
{
  csm_gpu_inventory_t gpu_inventory[CSM_GPU_MAX_DEVICES];
  uint32_t gpu_count(0);
  bool rc(false);

  // Call GetGpuInventory multiple times to make sure the library load/unload works repeatedly
  for ( uint32_t j = 0; j < 1; j++ )
  {
    rc = GetGpuInventory(gpu_inventory, gpu_count);

    if ( rc == true )
    {
      for( uint32_t i = 0; i < gpu_count; i++ )
      {
        cout << "gpu_id:                 " << gpu_inventory[i].gpu_id << endl;
        cout << "device_name:            " << gpu_inventory[i].device_name << endl;
        cout << "pci_bus_id:             " << gpu_inventory[i].pci_bus_id << endl;
        cout << "serial_number:          " << gpu_inventory[i].serial_number << endl;
        cout << "uuid:                   " << gpu_inventory[i].uuid << endl;
        cout << "vbios:                  " << gpu_inventory[i].vbios << endl;
        cout << "inforom_image_version:  " << gpu_inventory[i].inforom_image_version << endl;
        cout << "hbm memory:             " << gpu_inventory[i].hbm_memory << endl;
      }
    }
    else
    {
      cout << "GetGpuInventory() returned false." << endl;
    }
  }

  return 0;
}
