/*================================================================================
   
    csmd/src/inv/tests/test_inv_ssd_inventory.cc

  © Copyright IBM Corporation 2017-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <iostream>

#include "inv_ssd_inventory.h"

using std::cout;
using std::endl;

int main(void)
{
  csm_ssd_inventory_t ssd_inventory[CSM_SSD_MAX_DEVICES];
  uint32_t ssd_count(0);
  bool rc(false);

  rc = GetSsdInventory(ssd_inventory, ssd_count);

  if (rc == true)
  {
    for (uint32_t i = 0; i < ssd_count; i++)
    {
      cout << "serial_number:                 " << ssd_inventory[i].serial_number << endl;
      cout << "device_name:                   " << ssd_inventory[i].device_name << endl;
      cout << "pci_bus_id:                    " << ssd_inventory[i].pci_bus_id << endl;
      cout << "fw_ver:                        " << ssd_inventory[i].fw_ver << endl;
      cout << "size:                          " << ssd_inventory[i].size << endl;
      cout << "wear_lifespan_used:            " << ssd_inventory[i].wear_lifespan_used << endl;
      cout << "wear_total_bytes_written:      " << ssd_inventory[i].wear_total_bytes_written << endl;
      cout << "wear_total_bytes_read:         " << ssd_inventory[i].wear_total_bytes_read << endl;
      cout << "wear_percent_spares_remaining: " << ssd_inventory[i].wear_percent_spares_remaining << endl;
    }
  }
  else
  {
    cout << "GetSsdInventory() returned false." << endl;
  }

  return 0;
}
