/*================================================================================
   
    csmd/src/inv/tests/test_inv_hca_inventory.cc

  © Copyright IBM Corporation 2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <iostream>

#include "inv_hca_inventory.h"

using std::cout;
using std::endl;

int main(void)
{
  csm_hca_inventory_t hca_inventory[CSM_HCA_MAX_DEVICES];
  uint32_t hca_count(0);
  bool rc(false);

  rc = GetHcaInventory(hca_inventory, hca_count);

  if (rc == true)
  {
    for (uint32_t i = 0; i < hca_count; i++)
    {
      cout << "serial_number:          " << hca_inventory[i].serial_number << endl;
      cout << "device_name:            " << hca_inventory[i].device_name << endl;
      cout << "pci_bus_id:             " << hca_inventory[i].pci_bus_id << endl;
      cout << "guid:                   " << hca_inventory[i].guid << endl;
      cout << "part_number:            " << hca_inventory[i].part_number << endl;
      cout << "fw_ver:                 " << hca_inventory[i].fw_ver << endl;
      cout << "hw_rev:                 " << hca_inventory[i].hw_rev << endl;
      cout << "board_id:               " << hca_inventory[i].board_id << endl;
      cout << "switch_guid_port_1:     " << hca_inventory[i].switch_guid_port_1 << endl;
      cout << "switch_guid_port_2:     " << hca_inventory[i].switch_guid_port_2 << endl;
    }
  }
  else
  {
    cout << "GetHcaInventory() returned false." << endl;
  }

  return 0;
}
