/*================================================================================
   
    csmd/src/inv/src/inv_dimm_inventory.cc

  © Copyright IBM Corporation 2017-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_dimm_inventory.h"
#include "inv_functions.h"
#include "logging.h"

#include <dirent.h>       // Provides scandir()
#include <glob.h>

#include <string>
#include <fstream>
#include <list>
#include <stdint.h>

using namespace std;

// Helper function for copying strings to the dimm_inventory structure
// dimm is the specific dimm being processed, used for logging
// field is the name of the value to be set, used for logging
// value is the actual string to be copied into the dimm_inventory structure
// dest_ptr is a pointer to the destination in the structure (like snprintf)
// DEST_MAX is the size of the dest_ptr buffer (like snprintf)
void setDimmInventoryValue(const string& dimm, const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX);

bool GetDimmInventory(csm_dimm_inventory_t dimm_inventory[CSM_DIMM_MAX_DEVICES], uint32_t& dimm_count)
{
  LOG(csmd, trace) << "Enter " << __PRETTY_FUNCTION__;

  dimm_count = 0;

  // Scan the device-tree for the list of memory dimms
  // Use globbing "*" for the @XXX portions of the directory names in case they change in the future 
  // ls -d /proc/device-tree/xscom*/mcbist*/mcs*/mca*/dimm* | sort -t '/' -k8
  // /proc/device-tree/xscom@603fc00000000/mcbist@2/mcs@8/mca@80/dimm@d000
  // /proc/device-tree/xscom@603fc00000000/mcbist@2/mcs@8/mca@40/dimm@d001
  // /proc/device-tree/xscom@603fc00000000/mcbist@2/mcs@4/mca@20/dimm@d002
  // /proc/device-tree/xscom@603fc00000000/mcbist@2/mcs@4/mca@10/dimm@d003
  // /proc/device-tree/xscom@603fc00000000/mcbist@1/mcs@2/mca@8/dimm@d004
  // /proc/device-tree/xscom@603fc00000000/mcbist@1/mcs@2/mca@4/dimm@d005
  // /proc/device-tree/xscom@603fc00000000/mcbist@1/mcs@1/mca@2/dimm@d006
  // /proc/device-tree/xscom@603fc00000000/mcbist@1/mcs@1/mca@1/dimm@d007
  // /proc/device-tree/xscom@623fc00000000/mcbist@2/mcs@8/mca@80/dimm@d008
  // /proc/device-tree/xscom@623fc00000000/mcbist@2/mcs@8/mca@40/dimm@d009
  // /proc/device-tree/xscom@623fc00000000/mcbist@2/mcs@4/mca@20/dimm@d00a
  // /proc/device-tree/xscom@623fc00000000/mcbist@2/mcs@4/mca@10/dimm@d00b
  // /proc/device-tree/xscom@623fc00000000/mcbist@1/mcs@2/mca@8/dimm@d00c
  // /proc/device-tree/xscom@623fc00000000/mcbist@1/mcs@2/mca@4/dimm@d00d
  // /proc/device-tree/xscom@623fc00000000/mcbist@1/mcs@1/mca@2/dimm@d00e
  // /proc/device-tree/xscom@623fc00000000/mcbist@1/mcs@1/mca@1/dimm@d00f
  const char DIMM_GLOB_PATH[] = "/proc/device-tree/xscom*/mcbist*/mcs*/mca*/dimm*";
  std::list<std::string> dimmlist;

  int32_t globflags(0);
  glob_t dimmpaths;
  int32_t rc(0);

  rc = glob(DIMM_GLOB_PATH, globflags, nullptr, &dimmpaths);
  if (rc == 0)
  {
    for (uint32_t i=0; i < dimmpaths.gl_pathc && i < CSM_DIMM_MAX_DEVICES; i++)
    {
      LOG(csmd, debug) << "Found dimm = " << dimmpaths.gl_pathv[i];
      dimmlist.push_back(dimmpaths.gl_pathv[i]);
    }
  }
  else
  {
    LOG(csmd, warning) << "glob() returned rc=" << rc << " for " << DIMM_GLOB_PATH << ". Firmware level may not support dimm inventory.";
  } 
  
  globfree(&dimmpaths);
  
  for (list<string>::iterator dimm_itr = dimmlist.begin(); dimm_itr != dimmlist.end(); dimm_itr++)
  {
    string dimm = (*dimm_itr).substr((*dimm_itr).find("dimm")) + ": ";   // Used for trace messages
    bool fields_valid(true);
 
    // Collect the serial number 
    // cat /proc/device-tree/xscom@603fc00000000/mcbist@2/mcs@8/mca@80/dimm@d000/serial-number | hexdump -C | head -n 1
    // 00000000  36 e3 d5 b5
    ifstream serial_in(*dimm_itr + "/serial-number", ios::binary);
    uint32_t serial_integer(0); 
    string serial_number("");
    ostringstream serial_out;

    if (serial_in.is_open())
    {
      serial_in.read((char*) &serial_integer, sizeof(serial_integer));
      serial_integer = __builtin_bswap32(serial_integer); 
      
      serial_out << hex << serial_integer; 
      serial_number = serial_out.str();
     
      if (serial_in.gcount() == 0)
      {
        LOG(csmd, warning) << dimm << "detected empty dimm serial_number";
        fields_valid = false;
      }
    }
    else
    {
      LOG(csmd, warning) << dimm << "failed to read dimm serial_number";
      fields_valid = false;
    }
    serial_in.close();
      
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
        LOG(csmd, warning) << dimm << "detected empty dimm size";
        fields_valid = false;
      }
    }
    else
    {
      LOG(csmd, warning) << dimm << "failed to read dimm size";
      fields_valid = false;
    }
    size_in.close();
   
    // Collect the physical location 
    // cat /proc/device-tree/xscom@603fc00000000/mcbist@2/mcs@8/mca@80/dimm@d000/ibm,loc-code
    // UOPWR.7852C0A-Node0-DIMM0
    ifstream location_in(*dimm_itr + "/ibm,loc-code");
    string physical_location("");

    if (location_in.is_open())
    {
      getline(location_in, physical_location);
      location_in.close();    
      physical_location = trimString(physical_location);
 
      if (physical_location.empty())
      {
        LOG(csmd, warning) << dimm << "detected empty dimm physical_location";
        fields_valid = false;
      }
    }
    else
    {
      LOG(csmd, warning) << dimm << "failed to read dimm physical_location";
      fields_valid = false;
    }
 
    if ((fields_valid) && (dimm_count < CSM_DIMM_MAX_DEVICES))
    {
      setDimmInventoryValue(dimm, "serial_number", serial_number, dimm_inventory[dimm_count].serial_number, CSM_DIMM_SERIAL_NUMBER_MAX);
      LOG(csmd, info) << dimm << "size = " << size;
      dimm_inventory[dimm_count].size = size;
      setDimmInventoryValue(dimm, "physical_location", physical_location, dimm_inventory[dimm_count].physical_location, CSM_DIMM_PHYSICAL_LOCATION_MAX);
      dimm_count++;
    }
  }

  if (dimmlist.size() > CSM_DIMM_MAX_DEVICES)
  {
    LOG(csmd, warning) << "dimm_inventory truncated to " << dimm_count << " entries, " << dimmlist.size() << " dimms discovered.";
  }
 
  return true;
}

void setDimmInventoryValue(const string& dimm, const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX)
{
  if ( !value.empty() )
  {
    LOG(csmd, info) << dimm << field << " = " << value;
    
    strncpy(dest_ptr, value.c_str(), DEST_MAX);
    dest_ptr[DEST_MAX - 1] = '\0'; 
    
    if (value.size() > (DEST_MAX-1))
    {      
      LOG(csmd, warning) << dimm << field << " truncated to " << (DEST_MAX-1) << " bytes";
    }
  }
  else
  {
    LOG(csmd, error) << dimm << "Error: could not determine " << field;
  }
}
