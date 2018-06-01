/*================================================================================
   
    csmd/src/inv/src/inv_processor_inventory.cc

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_processor_inventory.h"
#include "inv_functions.h"
#include "logging.h"

#include <dirent.h>       // Provides scandir()
#include <glob.h>

#include <string>
#include <fstream>
#include <list>
#include <stdint.h>

using namespace std;

// Helper function for finding the number of cores associated with the given processor_chip_id
// processor_chip_id defines which processor to match against
// discovered_cores is the number of matching cores found if successful
// function will return false if the number of cores cannot be determined
bool GetDiscoveredCores(const uint32_t processor_chip_id, uint32_t& discovered_cores);

// Helper function for copying strings to the processor_inventory structure
// processor is the specific processor being processed, used for logging
// field is the name of the value to be set, used for logging
// value is the actual string to be copied into the processor_inventory structure
// dest_ptr is a pointer to the destination in the structure (like snprintf)
// DEST_MAX is the size of the dest_ptr buffer (like snprintf)
void setProcessorInventoryValue(const string& processor, const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX);

bool GetProcessorInventory(csm_processor_inventory_t processor_inventory[CSM_PROCESSOR_MAX_DEVICES], uint32_t& processor_count)
{
  LOG(csmd, trace) << "Enter " << __PRETTY_FUNCTION__;

  processor_count = 0;

  // Scan the device-tree for the list of processor sockets 
  // Use globbing "*" for the @XXX portions of the directory names in case they change in the future 
  // ls -d /proc/device-tree/vpd/root-node-vpd*/enclosure*/backplane*/processor*
  // /proc/device-tree/vpd/root-node-vpd@a000/enclosure@1e00/backplane@800/processor@1000
  // /proc/device-tree/vpd/root-node-vpd@a000/enclosure@1e00/backplane@800/processor@1001
  const char PROCESSOR_GLOB_PATH[] = "/proc/device-tree/vpd/root-node-vpd*/enclosure*/backplane*/processor*";
  std::list<std::string> processorlist;

  int32_t globflags(0);
  glob_t processorpaths;
  int32_t rc(0);

  rc = glob(PROCESSOR_GLOB_PATH, globflags, nullptr, &processorpaths);
  if (rc == 0)
  {
    for (uint32_t i=0; i < processorpaths.gl_pathc && i < CSM_PROCESSOR_MAX_DEVICES; i++)
    {
      LOG(csmd, debug) << "Found processor = " << processorpaths.gl_pathv[i];
      processorlist.push_back(processorpaths.gl_pathv[i]);
    }
  }
  else
  {
    LOG(csmd, warning) << "glob() returned rc=" << rc << " for " << PROCESSOR_GLOB_PATH << ".";
  } 
  
  globfree(&processorpaths);
  
  for (list<string>::iterator processor_itr = processorlist.begin(); processor_itr != processorlist.end(); processor_itr++)
  {
    string processor = (*processor_itr).substr((*processor_itr).find("processor")) + ": ";   // Used for trace messages
    bool fields_valid(true);
 
    // Collect the serial number 
    // cat /proc/device-tree/vpd/root-node-vpd\@a000/enclosure\@1e00/backplane\@800/processor\@1000/serial-number
    // YA1934276846
    ifstream serial_in(*processor_itr + "/serial-number");
    string serial_number("");

    if (serial_in.is_open())
    {
      getline(serial_in, serial_number);
      serial_in.close();    
      serial_number = trimString(serial_number);
 
      if (serial_number.empty())
      {
        LOG(csmd, warning) << processor << "detected empty processor serial_number.";
        fields_valid = false;
      }
    }
    else
    {
      LOG(csmd, warning) << processor << "Failed to read processor serial_number. Firmware level may not support processor inventory.";
      fields_valid = false;
    }
    
    // Collect the physical location 
    // cat /proc/device-tree/vpd/root-node-vpd@a000/enclosure@1e00/backplane@800/processor@1000/ibm,loc-code 
    // UOPWR.7852C8A-Node0-Proc0
    ifstream location_in(*processor_itr + "/ibm,loc-code");
    string physical_location("");

    if (location_in.is_open())
    {
      getline(location_in, physical_location);
      location_in.close();    
      physical_location = trimString(physical_location);
 
      if (physical_location.empty())
      {
        LOG(csmd, warning) << processor << "detected empty processor physical_location";
        fields_valid = false;
      }
    }
    else
    {
      LOG(csmd, warning) << processor << "failed to read processor physical_location";
      fields_valid = false;
    }
     
    // Collect the chip-id for the current processor socket
    // cat /proc/device-tree/vpd/root-node-vpd@a000/enclosure@1e00/backplane@800/processor@1000/ibm,chip-id | hexdump -C | head -n 1
    // 00000000  00 00 00 00                                       |....|
    // cat /proc/device-tree/vpd/root-node-vpd@a000/enclosure@1e00/backplane@800/processor@1001/ibm,chip-id | hexdump -C | head -n 1
    // 00000000  00 00 00 08    
    ifstream processor_chip_id_in(*processor_itr + "/ibm,chip-id", ios::binary);
    uint32_t processor_chip_id(0);
    uint32_t discovered_cores(0);

    if (processor_chip_id_in.is_open())
    {
      processor_chip_id_in.read((char*) &processor_chip_id, sizeof(processor_chip_id));
      processor_chip_id = __builtin_bswap32(processor_chip_id); 
 
      if (processor_chip_id_in.gcount() == 0)
      {
        LOG(csmd, warning) << processor << "detected empty processor chip-id";
        fields_valid = false;
      }
      else
      {
        // Determine the number of cores for this processor socket by counting the cores with a matching chip-id
        bool rc(false);
        rc = GetDiscoveredCores(processor_chip_id, discovered_cores);
        
        if (rc == false)
        {
          LOG(csmd, warning) << processor << "Unable to determine discovered_cores for processor chip-id = " << processor_chip_id;
          fields_valid = false;
        }
      }
    }
    else
    {
      LOG(csmd, warning) << processor << "failed to read processor chip-id";
      fields_valid = false;
    }
    processor_chip_id_in.close();
   
    if ((fields_valid) && (processor_count < CSM_PROCESSOR_MAX_DEVICES))
    {
      setProcessorInventoryValue(processor, "serial_number", serial_number, processor_inventory[processor_count].serial_number, 
        CSM_PROCESSOR_SERIAL_NUMBER_MAX);

      setProcessorInventoryValue(processor, "physical_location", physical_location, processor_inventory[processor_count].physical_location, 
        CSM_PROCESSOR_PHYSICAL_LOCATION_MAX);

      LOG(csmd, info) << processor << "discovered_cores = " << discovered_cores;
      processor_inventory[processor_count].discovered_cores = discovered_cores;

      processor_count++;
    }
  }

  if (processorlist.size() > CSM_PROCESSOR_MAX_DEVICES)
  {
    LOG(csmd, warning) << "processor_inventory truncated to " << processor_count << " entries, " << processorlist.size() << " processors discovered.";
  }
 
  return true;
}

bool GetDiscoveredCores(const uint32_t processor_chip_id, uint32_t& discovered_cores)
{
  bool success(true); 
  uint32_t matching_cores(0);

  // Scan the device-tree for the list of cpu cores 
  // Use globbing "*" for the @XXX portions of the directory names in case they change in the future 
  // ls -d1 /proc/device-tree/cpus/PowerPC,POWER9@*/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@0/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@10/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@14/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@18/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@1c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@20/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@24/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@28/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@2c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@30/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@34/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@38/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@3c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@40/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@44/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@4/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@50/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@54/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@58/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@5c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@800/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@804/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@808/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@80c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@810/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@814/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@820/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@824/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@828/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@82c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@830/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@834/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@838/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@83c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@840/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@844/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@848/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@84c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@850/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@854/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@858/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@85c/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@8/ibm,chip-id
  // /proc/device-tree/cpus/PowerPC,POWER9@c/ibm,chip-id
  const char CPU_CORE_GLOB_PATH[] = "/proc/device-tree/cpus/PowerPC,POWER9@*/ibm,chip-id";
  int32_t globflags(0);
  glob_t corepaths;
  int32_t rc(0);

  rc = glob(CPU_CORE_GLOB_PATH, globflags, nullptr, &corepaths);
  if (rc == 0)
  {
    for (uint32_t i=0; i < corepaths.gl_pathc; i++)
    {
      //LOG(csmd, debug) << "Found core chip-id = " << corepaths.gl_pathv[i];
    
      // Collect the chip-id for the current core
      // cat /proc/device-tree/cpus/PowerPC,POWER9@0/ibm,chip-id | hexdump -C | head -n 1
      // 00000000  00 00 00 00
      // cat /proc/device-tree/cpus/PowerPC,POWER9@800/ibm,chip-id | hexdump -C | head -n 1
      // 00000000  00 00 00 08      
      ifstream core_chip_id_in(corepaths.gl_pathv[i], ios::binary);
      uint32_t core_chip_id(0);

      if (core_chip_id_in.is_open())
      {
        core_chip_id_in.read((char*) &core_chip_id, sizeof(core_chip_id));
        core_chip_id = __builtin_bswap32(core_chip_id); 
 
        if (core_chip_id_in.gcount() == 0)
        {
          LOG(csmd, warning) << "detected empty core chip-id";
          success = false;
        }
        else
        {
          //LOG(csmd, debug) << "processor chip-id = " << processor_chip_id << ", core chip-id = " << core_chip_id;
          if (core_chip_id == processor_chip_id)
          {
            matching_cores++;
          }
        }
      }
      else
      {
        LOG(csmd, warning) << "failed to read core chip-id";
        success = false;
      }
      core_chip_id_in.close();
    }
  }
  else
  {
    LOG(csmd, warning) << "glob() returned rc=" << rc << " for " << CPU_CORE_GLOB_PATH << ".";
    success = false;
  } 
  
  globfree(&corepaths);
 
  if (success == true)
  {
    discovered_cores = matching_cores;
  }
 
  return success;
}

void setProcessorInventoryValue(const string& processor, const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX)
{
  if ( !value.empty() )
  {
    LOG(csmd, info) << processor << field << " = " << value;
    
    strncpy(dest_ptr, value.c_str(), DEST_MAX);
    dest_ptr[DEST_MAX - 1] = '\0'; 
    
    if (value.size() > (DEST_MAX-1))
    {      
      LOG(csmd, warning) << processor << field << " truncated to " << (DEST_MAX-1) << " bytes";
    }
  }
  else
  {
    LOG(csmd, error) << processor << "Error: could not determine " << field;
  }
}
