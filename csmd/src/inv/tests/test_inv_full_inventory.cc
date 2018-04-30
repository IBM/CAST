/*================================================================================
   
    csmd/src/inv/tests/test_inv_full_inventory.cc

  © Copyright IBM Corporation 2017-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include <iostream>

#include "inv_full_inventory.h"

using std::cout;
using std::endl;

int main(void)
{
  csm_full_inventory_t full_inventory;
  bool rc(false);

  rc = GetFullInventory(full_inventory);

  if (rc == true)
  {
    cout << "node_name:             " << full_inventory.node.node_name << endl;
    cout << "machine_model:         " << full_inventory.node.machine_model << endl;
    cout << "serial_number:         " << full_inventory.node.serial_number << endl;
    cout << "kernel_release:        " << full_inventory.node.kernel_release << endl;
    cout << "kernel_version:        " << full_inventory.node.kernel_version << endl;
    cout << "installed_memory:      " << full_inventory.node.installed_memory << endl;
    cout << "installed_swap:        " << full_inventory.node.installed_swap << endl;
    cout << "discovered_sockets:    " << full_inventory.node.discovered_sockets << endl;
    cout << "discovered_cores:      " << full_inventory.node.discovered_cores << endl;
    cout << "discovered_gpus:       " << full_inventory.node.discovered_gpus << endl;
    cout << "discovered_hcas:       " << full_inventory.node.discovered_hcas << endl;
    cout << "discovered_dimms:      " << full_inventory.node.discovered_dimms << endl;
    cout << "discovered_ssds:       " << full_inventory.node.discovered_ssds << endl;
    cout << "os_image_name:         " << full_inventory.node.os_image_name << endl;
    cout << "os_image_uuid:         " << full_inventory.node.os_image_uuid << endl;
    
    if (full_inventory.node.discovered_dimms > 0)
    {
      cout << endl;
      cout << "dimm: " << endl;
    }
 
    for (uint32_t i = 0; i < full_inventory.node.discovered_dimms; i++)
    {
      cout << "    - serial_number:          " << full_inventory.dimm[i].serial_number << endl;
      cout << "      size:                   " << full_inventory.dimm[i].size << endl;
      cout << "      physical_location:      " << full_inventory.dimm[i].physical_location << endl;
    }
        
    if (full_inventory.node.discovered_gpus > 0)
    {
      cout << endl;
      cout << "gpu: " << endl;
    }
    
    for (uint32_t i = 0; i < full_inventory.node.discovered_gpus; i++)
    {
      cout << "    - gpu_id:                 " << full_inventory.gpu[i].gpu_id << endl;
      cout << "      device_name:            " << full_inventory.gpu[i].device_name << endl;
      cout << "      pci_bus_id:             " << full_inventory.gpu[i].pci_bus_id << endl;
      cout << "      serial_number:          " << full_inventory.gpu[i].serial_number << endl;
      cout << "      uuid:                   " << full_inventory.gpu[i].uuid << endl;
      cout << "      vbios:                  " << full_inventory.gpu[i].vbios << endl;
      cout << "      inforom_image_version:  " << full_inventory.gpu[i].inforom_image_version << endl;
      cout << "      hbm memory:             " << full_inventory.gpu[i].hbm_memory << endl;
    }

    if (full_inventory.node.discovered_hcas > 0)
    {
      cout << endl;
      cout << "hca: " << endl;
    }
 
    for (uint32_t i = 0; i < full_inventory.node.discovered_hcas; i++)
    {
      cout << "    - serial_number:          " << full_inventory.hca[i].serial_number << endl;
      cout << "      device_name:            " << full_inventory.hca[i].device_name << endl;
      cout << "      pci_bus_id:             " << full_inventory.hca[i].pci_bus_id << endl;
      cout << "      guid:                   " << full_inventory.hca[i].guid << endl;
      cout << "      part_number:            " << full_inventory.hca[i].part_number << endl;
      cout << "      fw_ver:                 " << full_inventory.hca[i].fw_ver << endl;
      cout << "      hw_rev:                 " << full_inventory.hca[i].hw_rev << endl;
      cout << "      board_id:               " << full_inventory.hca[i].board_id << endl;
      cout << "      switch_guid_port_1:     " << full_inventory.hca[i].switch_guid_port_1 << endl;
      cout << "      switch_guid_port_2:     " << full_inventory.hca[i].switch_guid_port_2 << endl;
    }

    if (full_inventory.node.discovered_ssds > 0)
    {
      cout << endl;
      cout << "ssd: " << endl;
    }

    for (uint32_t i = 0; i < full_inventory.node.discovered_ssds; i++)
    {
      cout << "    - serial_number:                 " << full_inventory.ssd[i].serial_number << endl;
      cout << "      device_name:                   " << full_inventory.ssd[i].device_name << endl;
      cout << "      pci_bus_id:                    " << full_inventory.ssd[i].pci_bus_id << endl;
      cout << "      fw_ver:                        " << full_inventory.ssd[i].fw_ver << endl;
      cout << "      size:                          " << full_inventory.ssd[i].size << endl;
      cout << "      wear_lifespan_used:            " << full_inventory.ssd[i].wear_lifespan_used << endl;
      cout << "      wear_total_bytes_written:      " << full_inventory.ssd[i].wear_total_bytes_written << endl;
      cout << "      wear_total_bytes_read:         " << full_inventory.ssd[i].wear_total_bytes_read << endl;
      cout << "      wear_percent_spares_remaining: " << full_inventory.ssd[i].wear_percent_spares_remaining << endl;
    }
  }
  else
  {
    cout << "GetFullInventory() returned false." << endl;
  }

  return 0;
}
