/*================================================================================
   
    csmd/src/inv/tests/test_inv_processor_inventory.cc

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <iostream>

#include "inv_processor_inventory.h"

using std::cout;
using std::endl;

int main(void)
{
  csm_processor_inventory_t processor_inventory[CSM_PROCESSOR_MAX_DEVICES];
  uint32_t processor_count(0);
  bool rc(false);

  rc = GetProcessorInventory(processor_inventory, processor_count);

  if (rc == true)
  {
    for (uint32_t i = 0; i < processor_count; i++)
    {
      cout << "serial_number:          " << processor_inventory[i].serial_number << endl;
      cout << "physical_location:      " << processor_inventory[i].physical_location << endl;
      cout << "discovered_cores:       " << processor_inventory[i].discovered_cores << endl;
    }
  }
  else
  {
    cout << "GetProcessorInventory() returned false." << endl;
  }

  return 0;
}
