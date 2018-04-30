/*================================================================================
   
    csmd/src/inv/tests/test_inv_get_node_inventory.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "inv_get_node_inventory.h"

#include <assert.h>

int main(void)
{
  csm_node_inventory_t node_inventory;
  bool success(false);

  // return the c structure of the node inventory
  // false if error occurs
  success = GetNodeInventory(node_inventory);

  if (success == true)
  {
    std::cout << "node_name:             " << node_inventory.node_name << std::endl;
    std::cout << "machine_model:         " << node_inventory.machine_model << std::endl;
    std::cout << "serial_number:         " << node_inventory.serial_number << std::endl;
    std::cout << "kernel_release:        " << node_inventory.kernel_release << std::endl;
    std::cout << "kernel_version:        " << node_inventory.kernel_version << std::endl;
    std::cout << "installed_memory:      " << node_inventory.installed_memory << std::endl;
    std::cout << "installed_swap:        " << node_inventory.installed_swap << std::endl;
    std::cout << "discovered_sockets:    " << node_inventory.discovered_sockets << std::endl;
    std::cout << "discovered_cores:      " << node_inventory.discovered_cores << std::endl;
    std::cout << "os_image_name:         " << node_inventory.os_image_name << std::endl;
    std::cout << "os_image_uuid:         " << node_inventory.os_image_uuid << std::endl;
  }
  else
  {
    printf("GetNodeInventory() returned false\n");
  }

  return 0;
}
