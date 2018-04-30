/*================================================================================
   
    csmd/src/inv/src/inv_gpu_inventory.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_gpu_inventory.h"
#include "logging.h"

/////////////////////////////////////////////////////////////////////////////////
// DCGM support is enabled
/////////////////////////////////////////////////////////////////////////////////
#ifdef DCGM

#include <dcgm_agent.h>
#include <dlfcn.h>
#include <string>
#include "csmi/src/inv/include/inv_types.h"
#include "csmd/src/inv/include/inv_dcgm_access.h"

bool GetGpuInventory(csm_gpu_inventory_t gpu_inventory[CSM_GPU_MAX_DEVICES], uint32_t& gpu_count)
{
  LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__;

  // This is for unit test only and should be commented out for normal operation
  //return GetGpuInventoryUnitTestData(gpu_inventory, gpu_count);

  // Initialize gpu_inventory 
  for ( uint32_t i = 0; i < CSM_GPU_MAX_DEVICES; i++ )
  {
    gpu_inventory[i].gpu_id = i;
        
    memset(gpu_inventory[i].device_name, 0, CSM_GPU_DEVICE_NAME_MAX);
    memset(gpu_inventory[i].pci_bus_id, 0, CSM_GPU_PCI_BUS_ID_MAX);
    memset(gpu_inventory[i].serial_number, 0, CSM_GPU_SERIAL_NUMBER_MAX);
    memset(gpu_inventory[i].uuid, 0, CSM_GPU_UUID_MAX);
    memset(gpu_inventory[i].vbios, 0, CSM_GPU_VBIOS_MAX);
    memset(gpu_inventory[i].inforom_image_version, 0, CSM_GPU_INFOROM_IMAGE_VERSION_MAX);
    memset(gpu_inventory[i].hbm_memory, 0, CSM_GPU_HBM_MEMORY_SIZE_MAX);
  }

  // local variables
  bool dlopen_flag;
  bool dcgm_init_flag;
  bool get_attributes_flag;
  int dcgm_gpu_count;
  unsigned int * gpu_ids;
  dcgmDeviceAttributes_t * gpu_attributes;

  // setting variables
  gpu_count = 0;
  dlopen_flag = false;
  dcgm_init_flag = true;
  get_attributes_flag = true;;
  dcgm_gpu_count = -1;

  LOG(csmd, debug) << "GPU inventory collection calling INV_DCGM_ACCESS::GetInstance()";

  // cheking if the class that collects the data was not able to use dlopen
  dlopen_flag = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetDlopenFlag();
  if ( dlopen_flag == true )
  {
   return true;
  }

  // checking if the class that collects the data was not able to initialize DCGM env
  dcgm_init_flag = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetDCGMInitFlag();
  if ( dcgm_init_flag == false )
  {
   return false;
  }

  // checking if the class that collects the data got error in collecting the attributes
  get_attributes_flag = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetGPUsAttributes();
  if ( get_attributes_flag == false )
  {
   return false;
  }

  // checking if the class that collects the data got additional problems during construction
  dcgm_gpu_count = csm::daemon::INV_DCGM_ACCESS::GetInstance()->DCGMGPUCount();
  if ( dcgm_gpu_count == -1 ){
   LOG(csmd, error) << "The numebr of GPUS is equal to -1, this is wrong, no GPU data";
   return false;
  }

  // getting the data
  dcgm_gpu_count = csm::daemon::INV_DCGM_ACCESS::GetInstance()->DCGMGPUCount();
  gpu_ids = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetGPUsIdsPointer();
  gpu_attributes = csm::daemon::INV_DCGM_ACCESS::GetInstance()->GetGPUsAttributesPointer();

  // logging the data
  for (int32_t i = 0; i < dcgm_gpu_count && i < DCGM_MAX_NUM_DEVICES; i++)
  {

      // loggint the identfier
      LOG(csmd, debug) << "- gpu_id: " << gpu_ids[i];
      
      // dcgmDeviceSupportedClockSets_t information
      LOG(csmd, debug) << "  clocksets: ";
      LOG(csmd, debug) << "    count: " << gpu_attributes[i].clockSets.count;

      /*
      for (uint32_t cs = 0; cs < gpu_attributes[i].clockSets.count; cs++)
      {
       LOG(csmd, debug) << "    - mem_clock : " << gpu_attributes[i].clockSets.clockSet[cs].memClock;
       LOG(csmd, debug) << "      sm_clock  : " << gpu_attributes[i].clockSets.clockSet[cs].smClock;
      }
      */
      
      // dcgmDeviceThermals_t information
      LOG(csmd, debug) << "  thermal_settings: ";
      LOG(csmd, debug) << "    slowdown_temp : " << gpu_attributes[i].thermalSettings.slowdownTemp;
      LOG(csmd, debug) << "    shutdown_temp : " << gpu_attributes[i].thermalSettings.shutdownTemp;

      // dcgmDevicePowerLimits_t information
      LOG(csmd, debug) << "  power_limits: ";
      LOG(csmd, debug) << "    cur_power_limit      : " << gpu_attributes[i].powerLimits.curPowerLimit;
      LOG(csmd, debug) << "    default_power_limit  : " << gpu_attributes[i].powerLimits.defaultPowerLimit;
      LOG(csmd, debug) << "    enforced_power_limit : " << gpu_attributes[i].powerLimits.enforcedPowerLimit;
      LOG(csmd, debug) << "    min_power_limit      : " << gpu_attributes[i].powerLimits.minPowerLimit;
      LOG(csmd, debug) << "    max_power_limit      : " << gpu_attributes[i].powerLimits.maxPowerLimit;
      
      // dcgmDeviceIdentifiers_t information 
      LOG(csmd, debug) << "  identifiers: ";
      LOG(csmd, debug) << "    brand_name            : " << gpu_attributes[i].identifiers.brandName;
      LOG(csmd, debug) << "    device_name           : " << gpu_attributes[i].identifiers.deviceName;
      LOG(csmd, debug) << "    pci_bus_id            : " << gpu_attributes[i].identifiers.pciBusId;
      LOG(csmd, debug) << "    serial                : " << gpu_attributes[i].identifiers.serial;
      LOG(csmd, debug) << "    uuid                  : " << gpu_attributes[i].identifiers.uuid;
      LOG(csmd, debug) << "    vbios                 : " << gpu_attributes[i].identifiers.vbios;
      LOG(csmd, debug) << "    inforom_image_version : " << gpu_attributes[i].identifiers.inforomImageVersion;

      // dcgmDeviceMemoryUsage_t information
      LOG(csmd, debug) << "GPU global memory: ";
      LOG(csmd, debug) << "                   version: " << gpu_attributes[i].memoryUsage.version;
      LOG(csmd, debug) << "                 BAR1 size: " << gpu_attributes[i].memoryUsage.bar1Total;
      LOG(csmd, debug) << "        total frame buffer: " << gpu_attributes[i].memoryUsage.fbTotal;
      LOG(csmd, debug) << "         free frame buffer: " << gpu_attributes[i].memoryUsage.fbFree;
      
      LOG(csmd, debug);

      // copy the information into the gpu_inventory array 
      if ( gpu_count < CSM_GPU_MAX_DEVICES )
      {
        //LOG(csmd, debug) << "Copying data for gpu_id " << gpu_ids[i] << " to gpu_inventory.";
        gpu_inventory[gpu_count].gpu_id = gpu_ids[i];
        
        strncpy(gpu_inventory[gpu_count].device_name, gpu_attributes[i].identifiers.deviceName, CSM_GPU_DEVICE_NAME_MAX);
        gpu_inventory[gpu_count].device_name[CSM_GPU_DEVICE_NAME_MAX-1] = '\0';

        strncpy(gpu_inventory[gpu_count].pci_bus_id, gpu_attributes[i].identifiers.pciBusId, CSM_GPU_PCI_BUS_ID_MAX);
        gpu_inventory[gpu_count].pci_bus_id[CSM_GPU_PCI_BUS_ID_MAX-1] = '\0';
        
        strncpy(gpu_inventory[gpu_count].serial_number, gpu_attributes[i].identifiers.serial, CSM_GPU_SERIAL_NUMBER_MAX);
        gpu_inventory[gpu_count].serial_number[CSM_GPU_SERIAL_NUMBER_MAX-1] = '\0';
        
        strncpy(gpu_inventory[gpu_count].uuid, gpu_attributes[i].identifiers.uuid, CSM_GPU_UUID_MAX);
        gpu_inventory[gpu_count].uuid[CSM_GPU_UUID_MAX-1] = '\0';
        
        strncpy(gpu_inventory[gpu_count].vbios, gpu_attributes[i].identifiers.vbios, CSM_GPU_VBIOS_MAX);
        gpu_inventory[gpu_count].vbios[CSM_GPU_VBIOS_MAX-1] = '\0';
        
        strncpy(gpu_inventory[gpu_count].inforom_image_version, gpu_attributes[i].identifiers.inforomImageVersion, CSM_GPU_INFOROM_IMAGE_VERSION_MAX);
        gpu_inventory[gpu_count].inforom_image_version[CSM_GPU_INFOROM_IMAGE_VERSION_MAX-1] = '\0';

        strncpy(gpu_inventory[gpu_count].hbm_memory, (std::to_string(gpu_attributes[i].memoryUsage.fbTotal)).c_str(), CSM_GPU_HBM_MEMORY_SIZE_MAX);
        gpu_inventory[gpu_count].hbm_memory[CSM_GPU_HBM_MEMORY_SIZE_MAX-1] = '\0';

      }

      // Increment gpu_count
      gpu_count++;

  }
  
  return true;

}

#endif  // DCGM support is enabled

/////////////////////////////////////////////////////////////////////////////////
// DCGM support is disabled - stubs only
/////////////////////////////////////////////////////////////////////////////////
#ifndef DCGM
bool GetGpuInventory(csm_gpu_inventory_t gpu_inventory[CSM_GPU_MAX_DEVICES], uint32_t& gpu_count)
{
  LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__;
  LOG(csmd, info) << "Built without libdcgm.so support, no GPU inventory will be returned.";

  gpu_count = 0;  
  return true;
}

#endif  // DCGM support is disabled 
  
// Function to return realistic GPU inventory for unit testing only
bool GetGpuInventoryUnitTestData(csm_gpu_inventory_t gpu_inventory[CSM_GPU_MAX_DEVICES], uint32_t& gpu_count)
{
  //////////////////////////////////////////////////////////
  // Scaffolded data - for testing only
  //////////////////////////////////////////////////////////
  gpu_count = 4;
  for ( uint32_t i = 0; i < gpu_count; i++ )
  {
    gpu_inventory[i].gpu_id = i;
        
    strncpy(gpu_inventory[i].device_name, "Tesla P100-SXM2-16GB", CSM_GPU_DEVICE_NAME_MAX);
    gpu_inventory[i].device_name[CSM_GPU_DEVICE_NAME_MAX-1] = '\0';

    if ( i == 0 )
    {
      strncpy(gpu_inventory[i].pci_bus_id, "0002:01:00.0", CSM_GPU_PCI_BUS_ID_MAX);
      strncpy(gpu_inventory[i].serial_number, "0322716103294", CSM_GPU_SERIAL_NUMBER_MAX);
      strncpy(gpu_inventory[i].uuid, "GPU-ceb8b324-8f21-39c9-7933-2167862f42ea", CSM_GPU_UUID_MAX);
    }
    else if ( i == 1 )
    {
      strncpy(gpu_inventory[i].pci_bus_id, "0003:01:00.0", CSM_GPU_PCI_BUS_ID_MAX);
      strncpy(gpu_inventory[i].serial_number, "0322716102611", CSM_GPU_SERIAL_NUMBER_MAX);
      strncpy(gpu_inventory[i].uuid, "GPU-c99c9d1e-a47c-e00e-c7d5-5f98dcc21145", CSM_GPU_UUID_MAX);
    }
    else if ( i == 2 )
    {
      strncpy(gpu_inventory[i].pci_bus_id, "0006:01:00.0", CSM_GPU_PCI_BUS_ID_MAX);
      strncpy(gpu_inventory[i].serial_number, "0322716103089", CSM_GPU_SERIAL_NUMBER_MAX);
      strncpy(gpu_inventory[i].uuid, "GPU-d6444506-1946-56e1-5e30-bc9a5904d9bb", CSM_GPU_UUID_MAX);
    }
    else
    {
      strncpy(gpu_inventory[i].pci_bus_id, "0007:01:00.0", CSM_GPU_PCI_BUS_ID_MAX);
      strncpy(gpu_inventory[i].serial_number, "0322716071361", CSM_GPU_SERIAL_NUMBER_MAX);
      strncpy(gpu_inventory[i].uuid, "GPU-f604e283-f51f-8831-8462-e1e31f54979e", CSM_GPU_UUID_MAX);
    }

    gpu_inventory[i].pci_bus_id[CSM_GPU_PCI_BUS_ID_MAX-1] = '\0';
    gpu_inventory[i].serial_number[CSM_GPU_SERIAL_NUMBER_MAX-1] = '\0';
    gpu_inventory[i].uuid[CSM_GPU_UUID_MAX-1] = '\0';
        
    strncpy(gpu_inventory[i].vbios, "86.00.26.00.02", CSM_GPU_VBIOS_MAX);
    gpu_inventory[i].vbios[CSM_GPU_VBIOS_MAX-1] = '\0';
        
    strncpy(gpu_inventory[i].inforom_image_version, "H403.0201.00.04", CSM_GPU_INFOROM_IMAGE_VERSION_MAX);
    gpu_inventory[i].inforom_image_version[CSM_GPU_INFOROM_IMAGE_VERSION_MAX-1] = '\0';

    strncpy(gpu_inventory[i].hbm_memory, "16276", CSM_GPU_HBM_MEMORY_SIZE_MAX);
    gpu_inventory[i].hbm_memory[CSM_GPU_HBM_MEMORY_SIZE_MAX-1] = '\0';

  }

  return true;
}
