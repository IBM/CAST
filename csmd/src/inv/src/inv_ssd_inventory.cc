/*================================================================================
   
    csmd/src/inv/src/inv_ssd_inventory.cc

  © Copyright IBM Corporation 2017-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_ssd_inventory.h"
#include "inv_functions.h"
#include "logging.h"

#include <dirent.h>       // Provides scandir()
#include <glob.h>
#include <unistd.h>

#include <string>
#include <fstream>
#include <list>
#include <stdint.h>

using namespace std;

// Helper function for copying strings to the ssd_inventory structure
// field is the name of the value to be set, used for logging
// value is the actual string to be copied into the ssd_inventory structure
// dest_ptr is a pointer to the destination in the structure (like snprintf)
// DEST_MAX is the size of the dest_ptr buffer (like snprintf)
void setSsdInventoryValue(const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX);

bool GetSsdInventory(csm_ssd_inventory_t ssd_inventory[CSM_SSD_MAX_DEVICES], uint32_t& ssd_count)
{
  LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__;

  ssd_count = 0;

  // Scan for the list of SSDs
  // ls -d /sys/class/nvme/nvme*
  // /sys/class/nvme/nvme0 
  const char SSD_GLOB_PATH[] = "/sys/class/nvme/nvme*";
  std::list<std::string> ssdlist;

  int32_t globflags(0);
  glob_t ssdpaths;
  int32_t rc(0);

  rc = glob(SSD_GLOB_PATH, globflags, nullptr, &ssdpaths);
  if (rc == 0)
  {
    for (uint32_t i=0; i < ssdpaths.gl_pathc; i++)
    {
      LOG(csmd, debug) << "Found ssd: " << ssdpaths.gl_pathv[i];
      ssdlist.push_back(ssdpaths.gl_pathv[i]);
    }
  }
  else
  {
    LOG(csmd, warning) << "glob() returned rc=" << rc << " for " << SSD_GLOB_PATH;
  } 
  
  globfree(&ssdpaths);

  list<string>::iterator ssd_itr = ssdlist.begin(); 
  for (int i = 0; i < CSM_SSD_MAX_DEVICES && ssd_itr != ssdlist.end(); i++)
  {
    std::list<string> fields;
   
    // cat /sys/class/nvme/nvme0/serial 
    // S3RVNA0J702759      
    fields.push_back("serial");
    
    // cat /sys/class/nvme/nvme0/firmware_rev
    // MN11MN11
    fields.push_back("firmware_rev");

    // cat /sys/class/nvme/nvme0/model
    // PCIe3 1.6TB NVMe Flash Adapter II x8  
    fields.push_back("model");
    
    // size in 512 byte blocks:
    // cat /sys/class/nvme/nvme0/nvme0n1/size
    // 3125627568
    // Note: this is always the count of 512 byte blocks irrespective of the actual block size in use.
    //
    // size in bytes:
    // lsblk -b -P -o size --nodeps /dev/nvme0n1
    // SIZE="1600321314816"
    //
    // 3125627568 * 512 = 1600321314816
    fields.push_back("nvme0n1/size");
 
    // Attempt to collect the value associated with each field for each ssd
    for (std::list<string>::iterator field_itr = fields.begin(); field_itr != fields.end(); field_itr++) 
    {
      ifstream field_in(*ssd_itr + "/" + *field_itr);
      string value("");
  
      getline(field_in, value); 
      field_in.close();
      value = trimString(value);

      if (*field_itr == "serial")
      {
        setSsdInventoryValue(*field_itr, value, ssd_inventory[i].serial_number, CSM_SSD_SERIAL_NUMBER_MAX);
      }
      else if (*field_itr == "firmware_rev")
      {
        setSsdInventoryValue(*field_itr, value, ssd_inventory[i].fw_ver, CSM_SSD_FW_VER_MAX);
      }
      else if (*field_itr == "model")
      {
        setSsdInventoryValue(*field_itr, value, ssd_inventory[i].device_name, CSM_SSD_DEVICE_NAME_MAX);
      }
      else if (*field_itr == "nvme0n1/size")
      {
        int64_t blocks(0);
        istringstream blocks_in(value);        

        if ( !value.empty() && (blocks_in >> blocks))
        {
          const int64_t blocksize(512);
          int64_t size = (blocks * blocksize);  // Size in bytes
          LOG(csmd, info) << "size (in 512 byte blocks) = " << blocks;
          LOG(csmd, info) << "size (in bytes) = " << size;
          ssd_inventory[i].size = size;
        }
        else
        {
          LOG(csmd, error) << "Error: could not determine size";
        }
      }
      else
      {
        LOG(csmd, error) << "Error: unhandled field " << *field_itr;
      }
    } 

    // Get the pci_bus_id information
    // ls -l /sys/class/nvme/nvme0/
    //    device -> ../../../0030:01:00.0
    const int32_t LINK_NAME_MAX(1024);
    char link_name[LINK_NAME_MAX];
    ssize_t len(0); 
    string pci_path = *ssd_itr + "/device"; 

    len = readlink(pci_path.c_str(), link_name, LINK_NAME_MAX-1);
    if (len > 0)
    {     
      link_name[len] = '\0';
      string pci_bus_id = link_name;
      
      // Trim the path down to just the final directory
      size_t index = pci_bus_id.find_last_of("/");      
      if ((index != string::npos) && (pci_bus_id.size() > index+1))
      {
        pci_bus_id = pci_bus_id.substr(index+1);
        setSsdInventoryValue("pci_bus_id", pci_bus_id, ssd_inventory[i].pci_bus_id, CSM_SSD_PCI_BUS_ID_MAX);
      }   
      else
      {
        LOG(csmd, error) << "Error: could not determine pci_bus_id, unable to find / in path";
      }
    }
    else
    {
      LOG(csmd, error) << "Error: could not determine pci_bus_id, errno: " << strerror(errno) << " (" << errno << ")";
    }

    ssd_itr++;
    ssd_count++;
  } 
 
  if (ssdlist.size() > CSM_SSD_MAX_DEVICES)
  {
    LOG(csmd, warning) << "ssd_inventory truncated to " << ssd_count << " entries, " << ssdlist.size() << " ssds discovered.";
  }
  
  return true;
}

void setSsdInventoryValue(const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX)
{
  if ( !value.empty() )
  {
    LOG(csmd, info) << field << " = " << value;
    strncpy(dest_ptr, value.c_str(), DEST_MAX);
    dest_ptr[DEST_MAX - 1] = '\0'; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine " << field;
  }
}
