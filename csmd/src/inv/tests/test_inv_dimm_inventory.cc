/*================================================================================
   
    csmd/src/inv/tests/test_inv_dimm_inventory.cc

  © Copyright IBM Corporation 2017-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <iostream>

#include "inv_dimm_inventory.h"

using std::cout;
using std::endl;

int main(void)
{
  csm_dimm_inventory_t dimm_inventory[CSM_DIMM_MAX_DEVICES];
  uint32_t dimm_count(0);
  bool rc(false);

  rc = GetDimmInventory(dimm_inventory, dimm_count);

  if (rc == true)
  {
    for (uint32_t i = 0; i < dimm_count; i++)
    {
      cout << "serial_number:          " << dimm_inventory[i].serial_number << endl;
      cout << "size:                   " << dimm_inventory[i].size << endl;
      cout << "physical_location:      " << dimm_inventory[i].physical_location << endl;
    }
  }
  else
  {
    cout << "GetDimmInventory() returned false." << endl;
  }

  return 0;
}
