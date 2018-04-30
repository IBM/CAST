/*================================================================================

    csmd/src/inv/tests/dcgm/test_dcgm.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <dcgm_agent.h>

#include <iostream>
#include <string>

using std::cout;
using std::endl;

// This test program requires the following files provided by the datacenter-gpu-manager rpm:
// /usr/include/dcgm_agent.h
// /usr/include/dcgm_fields.h
// /usr/include/dcgm_structs.h
// /usr/lib64/libdcgm.so
// /usr/lib64/libdcgm.so.1

#define __DCGM_DBG_FILE = dcgm.log
#define __DCGM_DBG_LVL = DEBUG 

int main()
{
  // It is the user's responsibility to call dcgmInit() before calling any other methods, 
  // and dcgmShutdown() once DCGM is no longer being used. 
  // Run DCGM in embedded mode to have fine grained control over jitter
  dcgmReturn_t rc;
  dcgmHandle_t dcgm_handle;
  unsigned int gpu_ids[DCGM_MAX_NUM_DEVICES];
  dcgmDeviceAttributes_t gpu_attributes[DCGM_MAX_NUM_DEVICES];
  int gpu_count;

  rc = dcgmInit();
  if (rc != DCGM_ST_OK)
  {
    cout << "Error: dcgmInit returned \"" << errorString(rc) << "(" << rc << ")\"" << endl;
    return rc;  
  }

  rc =  dcgmStartEmbedded(DCGM_OPERATION_MODE_MANUAL, &dcgm_handle);
  if (rc != DCGM_ST_OK)
  {
    cout << "Error: dcgmStartEmbedded returned \"" << errorString(rc) << "(" << rc << ")\"" << endl;
    return rc;
  }

  rc = dcgmUpdateAllFields(dcgm_handle,1);
  if (rc != DCGM_ST_OK)
  {
    cout << "test_dcgm.cc - Error: dcgmUpdateAllFields returned \"" << errorString(rc) << "(" << rc << ")\"" << endl;
    return rc;  
  }
 
  rc = dcgmGetAllDevices(dcgm_handle, gpu_ids, &gpu_count);
  if (rc != DCGM_ST_OK)
  {
    cout << "Error: dcgmGetAllDevices returned \"" << errorString(rc) << "(" << rc << ")\"" << endl;
    return rc;  
  }

  cout << "gpu_count: " << gpu_count << endl;
  cout << "gpus:" << endl;

  for (int i = 0; i < gpu_count; i++)
  {
    gpu_attributes[i].version = dcgmDeviceAttributes_version; 
    rc = dcgmGetDeviceAttributes(dcgm_handle, gpu_ids[i], &gpu_attributes[i]);
    if (rc != DCGM_ST_OK)
    {
      cout << "Error: dcgmGetDeviceAttributes for gpu_id=" << gpu_ids[i] << " returned \"" 
           << errorString(rc) << "(" << rc << ")\"" << endl;
    }
    else
    {
      cout << "    - gpu_id: " << gpu_ids[i] << endl;
      
      // dcgmDeviceSupportedClockSets_t information
      cout << "      clocksets: " << endl;
      cout << "          count: " << gpu_attributes[i].clockSets.count << endl;
          
      for (uint32_t cs = 0; cs < gpu_attributes[i].clockSets.count; cs++)
      {
//      cout << "          - mem_clock : " << gpu_attributes[i].clockSets.clockSet[cs].memClock << endl;
//      cout << "            sm_clock  : " << gpu_attributes[i].clockSets.clockSet[cs].smClock << endl;
      }
      
      // dcgmDeviceThermals_t information
      cout << "      thermal_settings: " << endl;
      cout << "          slowdown_temp : " << gpu_attributes[i].thermalSettings.slowdownTemp << endl;
      cout << "          shutdown_temp : " << gpu_attributes[i].thermalSettings.shutdownTemp << endl;

      // dcgmDevicePowerLimits_t information
      cout << "      power_limits: " << endl;
      cout << "          cur_power_limit      : " << gpu_attributes[i].powerLimits.curPowerLimit << endl;
      cout << "          default_power_limit  : " << gpu_attributes[i].powerLimits.defaultPowerLimit << endl;
      cout << "          enforced_power_limit : " << gpu_attributes[i].powerLimits.enforcedPowerLimit << endl;
      cout << "          min_power_limit      : " << gpu_attributes[i].powerLimits.minPowerLimit << endl;
      cout << "          max_power_limit      : " << gpu_attributes[i].powerLimits.maxPowerLimit << endl;
      
      // dcgmDeviceIdentifiers_t information 
      cout << "      identifiers: " << endl;
      cout << "          brand_name            : " << gpu_attributes[i].identifiers.brandName << endl;
      cout << "          device_name           : " << gpu_attributes[i].identifiers.deviceName << endl;
      cout << "          pci_bus_id            : " << gpu_attributes[i].identifiers.pciBusId << endl;
      cout << "          serial                : " << gpu_attributes[i].identifiers.serial << endl;
      cout << "          uuid                  : " << gpu_attributes[i].identifiers.uuid << endl;
      cout << "          vbios                 : " << gpu_attributes[i].identifiers.vbios << endl;
      cout << "          inforom_image_version : " << gpu_attributes[i].identifiers.inforomImageVersion << endl;

      // dcgmDeviceMemoryUsage_t information
      cout << "GPU global memory: " << endl;
      cout << "                         version: " << gpu_attributes[i].memoryUsage.version << endl;
      cout << "                       BAR1 size: " << gpu_attributes[i].memoryUsage.bar1Total << endl;
      cout << "              total frame buffer: " << gpu_attributes[i].memoryUsage.fbTotal << endl;
      cout << "               free frame buffer: " << gpu_attributes[i].memoryUsage.fbFree << endl;
      cout << endl;
    }
  }

  rc =  dcgmStopEmbedded(dcgm_handle);
  if (rc != DCGM_ST_OK)
  {
    cout << "Error: dcgmStartEmbedded returned \"" << errorString(rc) << "(" << rc << ")\"" << endl;
    return rc;
  }

  rc = dcgmShutdown();
  if (rc != DCGM_ST_OK)
  {
    cout << "Error: dcgmShutdown returned \"" << errorString(rc) << "(" << rc << ")\"" << endl;
    return rc;  
  }

  return rc;
}

