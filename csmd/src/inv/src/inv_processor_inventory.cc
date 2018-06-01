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
//#include <stdint.h>

using namespace std;

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
     
#ifdef TODO 
    // Collect the size
    // cat /proc/device-tree/xscom@603fc00000000/mcbist@2/mcs@8/mca@80/dimm@d000/size | hexdump -C | head -n 1
    // 00000000  00 00 80 00 
    ifstream size_in(*dimm_itr + "/size", ios::binary);
    uint32_t size(0);

    if (size_in.is_open())
    {
      size_in.read((char*) &size, sizeof(size));
      size = __builtin_bswap32(size); 
 
      if (size_in.gcount() == 0)
      {
        LOG(csmd, warning) << processor << "detected empty processor size";
        fields_valid = false;
      }
    }
    else
    {
      LOG(csmd, warning) << processor << "failed to read processor size";
      fields_valid = false;
    }
    size_in.close();
   
#endif 

    if ((fields_valid) && (processor_count < CSM_PROCESSOR_MAX_DEVICES))
    {
      setProcessorInventoryValue(processor, "serial_number", serial_number, processor_inventory[processor_count].serial_number, 
        CSM_PROCESSOR_SERIAL_NUMBER_MAX);
      setProcessorInventoryValue(processor, "physical_location", physical_location, processor_inventory[processor_count].physical_location, 
        CSM_PROCESSOR_PHYSICAL_LOCATION_MAX);
      //LOG(csmd, info) << processor << "size = " << size;
      //processor_inventory[processor_count].size = size;
      processor_count++;
    }
  }

  if (processorlist.size() > CSM_PROCESSOR_MAX_DEVICES)
  {
    LOG(csmd, warning) << "processor_inventory truncated to " << processor_count << " entries, " << processorlist.size() << " processors discovered.";
  }
 
  return true;
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
