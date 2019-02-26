/*================================================================================
   
    csmd/src/inv/src/inv_ssd_inventory.cc

  © Copyright IBM Corporation 2017-2019. All Rights Reserved

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

#include <fstream>
#include <list>
#include <stdint.h>
#include <algorithm>

using namespace std;

// Helper function for copying strings to the ssd_inventory structure
// field is the name of the value to be set, used for logging
// value is the actual string to be copied into the ssd_inventory structure
// dest_ptr is a pointer to the destination in the structure (like snprintf)
// DEST_MAX is the size of the dest_ptr buffer (like snprintf)
void setSsdInventoryValue(const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX, const bool& verbose=true);

// Helper function for extracting field values from the nvme command output lines
// Returns -1 if an error occurs, otherwise returns the fields value
// Logs a message that includes the fieldname for both successful and unsuccessful operation 
int64_t getNvmeFieldInt64Value(string outputline, const string &fieldname);

bool GetSsdInventory(csm_ssd_inventory_t ssd_inventory[CSM_SSD_MAX_DEVICES], uint32_t& ssd_count, const bool& verbose)
{
  if (verbose)
  { 
    LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__; 
  }
  else
  {
    LOG(csmd, debug) << "Enter " << __PRETTY_FUNCTION__;
  }

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
        setSsdInventoryValue(*field_itr, value, ssd_inventory[i].serial_number, CSM_SSD_SERIAL_NUMBER_MAX, verbose);
      }
      else if (*field_itr == "firmware_rev")
      {
        setSsdInventoryValue(*field_itr, value, ssd_inventory[i].fw_ver, CSM_SSD_FW_VER_MAX, verbose);
      }
      else if (*field_itr == "model")
      {
        setSsdInventoryValue(*field_itr, value, ssd_inventory[i].device_name, CSM_SSD_DEVICE_NAME_MAX, verbose);
      }
      else if (*field_itr == "nvme0n1/size")
      {
        int64_t blocks(0);
        istringstream blocks_in(value);        

        if ( !value.empty() && (blocks_in >> blocks))
        {
          const int64_t blocksize(512);
          int64_t size = (blocks * blocksize);  // Size in bytes
          if (verbose)
          {
            LOG(csmd, info) << "size (in 512 byte blocks) = " << blocks;
            LOG(csmd, info) << "size (in bytes) = " << size;
          }
          else
          {
            LOG(csmd, debug) << "size (in 512 byte blocks) = " << blocks;
            LOG(csmd, debug) << "size (in bytes) = " << size;
          }
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
        setSsdInventoryValue("pci_bus_id", pci_bus_id, ssd_inventory[i].pci_bus_id, CSM_SSD_PCI_BUS_ID_MAX, verbose);
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

    // Get the SSD wear information
    // TODO: Generate devicename from the sysfs file system tokens
    std::string devicename("/dev/nvme0n1");
    GetSsdWear(devicename, ssd_inventory[i].wear_lifespan_used, ssd_inventory[i].wear_percent_spares_remaining, 
               ssd_inventory[i].wear_total_bytes_written, ssd_inventory[i].wear_total_bytes_read);
    
    if (verbose)
    {
      LOG(csmd, info) << "wear_lifespan_used: " << ssd_inventory[i].wear_lifespan_used
                      << " wear_percent_spares_remaining: " << ssd_inventory[i].wear_percent_spares_remaining;
      LOG(csmd, info) << "wear_total_bytes_written: " << ssd_inventory[i].wear_total_bytes_written
                      << " wear_total_bytes_read: " << ssd_inventory[i].wear_total_bytes_read;
    }
    else
    {
      LOG(csmd, debug) << "wear_lifespan_used: " << ssd_inventory[i].wear_lifespan_used
                       << " wear_percent_spares_remaining: " << ssd_inventory[i].wear_percent_spares_remaining;
      LOG(csmd, debug) << "wear_total_bytes_written: " << ssd_inventory[i].wear_total_bytes_written
                       << " wear_total_bytes_read: " << ssd_inventory[i].wear_total_bytes_read;
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

void setSsdInventoryValue(const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX, const bool& verbose)
{
  if ( !value.empty() )
  {
    if ( verbose )
    { 
      LOG(csmd, info) << field << " = " << value; 
    }
    else 
    { 
      LOG(csmd, debug) << field << " = " << value; 
    }

    strncpy(dest_ptr, value.c_str(), DEST_MAX);
    dest_ptr[DEST_MAX - 1] = '\0'; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine " << field;
  }
}

bool GetSsdWear(const std::string &devicename, int32_t &wear_lifespan_used, int32_t &wear_percent_spares_remaining, 
                int64_t &wear_total_bytes_written, int64_t &wear_total_bytes_read)
{
   // Success of the execution
   bool success = true;
   wear_lifespan_used = -1;
   wear_percent_spares_remaining = -1;
   wear_total_bytes_written = -1;
   wear_total_bytes_read = -1;

   // Open a pipe to execute the nvme command.
   std::array<char, 128> buffer;
   std::stringstream outputstream;
   string cmd = "/usr/sbin/nvme";
   cmd = cmd + " smart-log " + devicename + " 2>&1";

   FILE* pipe = popen( cmd.c_str(), "r" );

   // If the pipe was successfully opened execute.
   if ( pipe )
   {
      // Build a string stream from the buffer.
      while(!feof(pipe))
      {
         if ( fgets( buffer.data(), 128, pipe) != nullptr)
         {
            outputstream << buffer.data();
         }
      }

      // Iterate over the stream to extract the usable values.
      std::string outputline;
      while (std::getline(outputstream, outputline, '\n'))
      {
         LOG(csmd, trace) << "GetSsdWear() read: " << outputline;
      
         // nvme smart-log /dev/nvme0n1 | egrep "available_spare |percentage_used |data_units_read |data_units_written "
         // available_spare                     : 100%
         // percentage_used                     : 0%
         // data_units_read                     : 9,412,417
         // data_units_written                  : 7,324,282

         // Errors:
         // command not found
         // No such file or directory
         // Usage:

         if ( outputline.find("command not found") != std::string::npos )
         {
            LOG(csmd, error) << "GetSsdWear(): " << outputline; 
         }
         else if ( outputline.find("No such file or directory") != std::string::npos )
         {
            LOG(csmd, error) << "GetSsdWear(): " << outputline; 
         }
         else if ( outputline.find("Usage:") != std::string::npos )
         {
            LOG(csmd, error) << "GetSsdWear(): " << outputline;
         }
         else if ( outputline.find("available_spare ") != std::string::npos )
         {
            wear_percent_spares_remaining = getNvmeFieldInt64Value(outputline, "available_spare");
         }
         else if ( outputline.find("percentage_used ") != std::string::npos )
         {
            wear_lifespan_used = getNvmeFieldInt64Value(outputline, "percentage_used");
         }
         else if ( outputline.find("data_units_read ") != std::string::npos )
         {
            int64_t data_units_read(-1);
            data_units_read = getNvmeFieldInt64Value(outputline, "data_units_read");

            if (data_units_read != -1)
            {
               wear_total_bytes_read = data_units_read*512000;
               LOG(csmd, debug) << "GetSsdWear(): wear_total_bytes_read: " << wear_total_bytes_read;
            }
         }
         else if ( outputline.find("data_units_written ") != std::string::npos )
         {
            int64_t data_units_written(-1);
            data_units_written = getNvmeFieldInt64Value(outputline, "data_units_written");

            if (data_units_written != -1)
            {
               wear_total_bytes_written = data_units_written*512000;
               LOG(csmd, debug) << "GetSsdWear(): wear_total_bytes_written: " << wear_total_bytes_written;
            }
         }
      }

      if ( pclose(pipe) < 0 )
      {
         LOG(csmd, warning) << "GetSsdWear(): pclose() error errno: " << strerror(errno) << " (" << errno << ")";
      }
   }
   else
   {
      LOG(csmd, warning) << "GetSsdWear(): popen() error errno: " << strerror(errno) << " (" << errno << ")";
      success = false;
   }
  
   return success;
}

int64_t getNvmeFieldInt64Value(string outputline, const string &fieldname)
{
   int64_t value(-1);
            
   // Example expected line formats:
   // available_spare                     : 100%
   // percentage_used                     : 0%
   // data_units_read                     : 9,412,417
   // data_units_written                  : 7,324,282
            
   // Remove commas
   outputline.erase(std::remove(outputline.begin(), outputline.end(), ','), outputline.end()); 
   
   istringstream ss_in(outputline);
   string token("");

   ss_in >> token;     // Read field name
   ss_in >> token;     // Read :
   ss_in >> value;     // Read field value

   if ( value != -1 )
   {
      LOG(csmd, debug) << "getNvmeFieldInt64Value(): Read " << fieldname << " value: " << value;
   }
   else
   {
      LOG(csmd, warning) << "getNvmeFieldInt64Value(): Unable to read " << fieldname << " value from: [" << outputline << "]";
   } 

   return value;
}
