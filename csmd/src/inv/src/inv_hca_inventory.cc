/*================================================================================
   
    csmd/src/inv/src/inv_hca_inventory.cc

  © Copyright IBM Corporation 2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "inv_hca_inventory.h"
#include "inv_ib_guid.h"
#include "logging.h"

#include <dirent.h>       // Provides scandir()
#include <glob.h>
#include <unistd.h>

#include <string>
#include <fstream>
#include <list>
#include <stdint.h>
#include <map>
#include <algorithm>

using namespace std;

// Helper function for decoding the binary VPD file
void decodeVpd(const string i_vpdpath, string& o_product_name, string& o_part_number, string& o_serial_number);

// Helper function for copying strings to the hca_inventory structure
// field is the name of the value to be set, used for logging
// value is the actual string to be copied into the hca_inventory structure
// dest_ptr is a pointer to the destination in the structure (like snprintf)
// DEST_MAX is the size of the dest_ptr buffer (like snprintf)
void setHcaInventoryValue(const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX);

bool GetHcaInventory(csm_hca_inventory_t hca_inventory[CSM_HCA_MAX_DEVICES], uint32_t& hca_count)
{
  LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__;

  hca_count = 0;

  // Scan for the list of Mellanox HCAs 
  // ls -d /sys/class/infiniband/mlx5_* 
  // /sys/class/infiniband/mlx5_0
  // /sys/class/infiniband/mlx5_1
  // /sys/class/infiniband/mlx5_2
  // /sys/class/infiniband/mlx5_3
  const char HCA_GLOB_PATH[] = "/sys/class/infiniband/mlx5_*";
  std::list<std::string> hcalist;
  std::list<std::string> sys_image_guids;

  int32_t globflags(0);
  glob_t hcapaths;
  int32_t rc(0);

  rc = glob(HCA_GLOB_PATH, globflags, nullptr, &hcapaths);
  if (rc == 0)
  {
    // Check each path to determine if this is a new unique physical adapter based on sys_image_guid
    for (uint32_t i=0; i < hcapaths.gl_pathc; i++)
    {
      // Check the sys_image_guid first to see if this mlx5_ device is associated with a new physical adapter.
      // cat /sys/class/infiniband/mlx5_0/sys_image_guid 
      // ec0d:9a03:006d:9804
      ifstream field_in(string(hcapaths.gl_pathv[i]) + "/" + "sys_image_guid");
      string value("");
  
      field_in >> value;
      field_in.close();
      value = standardizeGuid(value);
  
      if (value.empty())
      {
        LOG(csmd, error) << "Error: could not determine value for sys_image_guid";
      }
      else
      {
        // Do we already have information about this physical adapter?
        if (find(sys_image_guids.begin(), sys_image_guids.end(), value) == sys_image_guids.end())
        {
          LOG(csmd, debug) << "Found new hca: sys_image_guid = " << value;
          hcalist.push_back(hcapaths.gl_pathv[i]);
          sys_image_guids.push_back(value);
        }
      }
    }
  }
  else
  {
    LOG(csmd, warning) << "glob() returned rc=" << rc << " for " << HCA_GLOB_PATH;
  } 
  
  globfree(&hcapaths);

  list<string>::iterator hca_itr = hcalist.begin(); 
  for (int i = 0; i < CSM_HCA_MAX_DEVICES && hca_itr != hcalist.end(); i++)
  {
    std::list<string> fields;
    
    // cat /sys/class/infiniband/mlx5_0/sys_image_guid 
    // ec0d:9a03:006d:9804
    fields.push_back("sys_image_guid");
      
    // cat /sys/class/infiniband/mlx5_0/board_id 
    // IBM0000000002
    fields.push_back("board_id");
      
    // cat /sys/class/infiniband/mlx5_0/fw_ver
    // 16.21.0106
    fields.push_back("fw_ver");
      
    // cat /sys/class/infiniband/mlx5_0/hw_rev
    // 0
    fields.push_back("hw_rev");

    // Attempt to collect the value associated with each field for each hca
    for (std::list<string>::iterator field_itr = fields.begin(); field_itr != fields.end(); field_itr++) 
    {
      ifstream field_in(*hca_itr + "/" + *field_itr);
      string value("");
  
      field_in >> value;
      field_in.close();

      if (*field_itr == "sys_image_guid")
      {
        value = standardizeGuid(value);
        setHcaInventoryValue(*field_itr, value, hca_inventory[i].guid, CSM_HCA_GUID_MAX);
      }
      else if (*field_itr == "board_id")
      {
        setHcaInventoryValue(*field_itr, value, hca_inventory[i].board_id, CSM_HCA_BOARD_ID_MAX);
      }
      else if (*field_itr == "fw_ver")
      {
        setHcaInventoryValue(*field_itr, value, hca_inventory[i].fw_ver, CSM_HCA_FW_VER_MAX);
      }
      else if (*field_itr == "hw_rev")
      {
        setHcaInventoryValue(*field_itr, value, hca_inventory[i].hw_rev, CSM_HCA_HW_REV_MAX);
      }
      else
      {
        LOG(csmd, error) << "Error: unhandled field " << *field_itr;
      }
    } 

    // Get the pci_bus_id information
    // ls -l /sys/class/infiniband/mlx5_0/
    //    device -> ../../../0003:01:00.0   
    const int32_t LINK_NAME_MAX(1024);
    char link_name[LINK_NAME_MAX];
    ssize_t len(0); 
    string pci_path = *hca_itr + "/device"; 

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
        setHcaInventoryValue("pci_bus_id", pci_bus_id, hca_inventory[i].pci_bus_id, CSM_HCA_PCI_BUS_ID_MAX);
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

    // Get the interesting fields from the VPD
    string device_name("");
    string part_number("");
    string serial_number("");
    decodeVpd(*hca_itr + "/device/vpd", device_name, part_number, serial_number);

    setHcaInventoryValue("device_name", device_name, hca_inventory[i].device_name, CSM_HCA_DEVICE_NAME_MAX);
    setHcaInventoryValue("part_number", part_number, hca_inventory[i].part_number, CSM_HCA_PART_NUMBER_MAX);
    setHcaInventoryValue("serial_number", serial_number, hca_inventory[i].serial_number, CSM_HCA_SERIAL_NUMBER_MAX);

    hca_itr++;
    hca_count++;
  } 
 
  if (hcalist.size() > CSM_HCA_MAX_DEVICES)
  {
    LOG(csmd, warning) << "hca_inventory truncated to " << hca_count << " entries, " << hcalist.size() << " hcas discovered.";
  }

  return true;
}

// Helper function for decoding the binary VPD file
// i_vpdpath is the full path to the file to be parsed
// After the function returns:
// o_product_name, o_part_number, o_serial_number will be set to valid values 
// or empty("") if the value cannot be determined
void decodeVpd(const string i_vpdpath, string& o_product_name, string& o_part_number, string& o_serial_number)
{
  // Attempt to open the file for parsing
  std::ifstream vpd_in(i_vpdpath);

  // Clear the output parameters
  o_product_name = "";
  o_part_number = "";
  o_serial_number = "";

  // VPD format
  // File contains multiple blocks, each block starts with a block header.
  // Block Header:
  // 1 byte | 1 byte | 1 byte
  // hdr1   | len    | hdr2
  // 
  // First block type:
  // Starts with block header
  // Product Name Block (hdr1=0x82)
  // Contains one entry after the block header:
  // product name string (len bytes)
  //
  // Second block type:
  // Starts with block header
  // Part Number Block (hdr1=0x90)
  // Contains multiple entries, each with this format:
  // 2 bytes | 1 byte | sublen bytes 
  // vpdcode | sublen | string value for this vpd item 
  char nextchar(0);              // The next byte read from the stream
  char hdr1(0);                  // The first header byte for this block
  uint16_t len(0);               // The total length of the block beyond the header
  uint16_t bytesread(0);;        // The total number of bytes that have been read in the current block
  char hdr2(0);                  // The second header byte for this block
  string value("");              // The value we are currently constructing, up to sublen bytes
  string vpdcode("");            // The unique VPD code associated with the current value
  uint16_t sublen(0);            // The length of one value within a block 

  // State machine for reading the next expected value from the file
  enum vpd_state_t { READHDR1, READLEN, READHDR2, READVALUE, READVPDCODE1, READVPDCODE2, READSUBLEN };
  vpd_state_t state(READHDR1);

  // Known VPD blocks
  const char PRODUCT_NAME_HDR(0x82); 
  const char PART_NUMBER_HDR(0x90); 

  // Parse the file, one byte at a time
  while (vpd_in.get(nextchar))
  {
    if (state == READHDR1)
    {
      hdr1 = nextchar;
      state = READLEN;
    }
    else if (state == READLEN)
    {
      len = (uint16_t) nextchar;
      state = READHDR2;
    }
    else if (state == READHDR2)
    {
      hdr2 = nextchar;
      value = "";
      bytesread = 0;
    
      if (hdr1 == PRODUCT_NAME_HDR)
      { 
         state=READVALUE;
         sublen=len;
      }
      else if (hdr1 == PART_NUMBER_HDR)
      {
        state=READVPDCODE1;
      }
      else
      {
         LOG(csmd, warning) << "Unknown VPD block type, hdr1=" << hex << (int) hdr1 << " hdr2=" << (int) hdr2 << dec;
         state=READVPDCODE1;
      }
    }
    else if (state == READVPDCODE1)
    {
      bytesread++;
      vpdcode = nextchar;
      state = READVPDCODE2;
    }
    else if (state == READVPDCODE2)
    {
      bytesread++;
      vpdcode += nextchar;
      state = READSUBLEN;
    }
    else if (state == READSUBLEN)
    {
      bytesread++;
      sublen = (uint16_t) nextchar;
      state = READVALUE;
    }
    else if (state == READVALUE)
    {
      bytesread++;
      value += nextchar;
      
      // We have finished reading a whole value
      if (value.size() == sublen)
      {
         LOG(csmd, debug) << hex << "hdr1=" << (int) hdr1 << " hdr2=" << (int) hdr2 
                          << dec << " len=" << len << " read=" << bytesread << " len2=" << sublen
                          << " code=" << vpdcode << " value=[" << value << "]";
      
        // For relevant fields, set the value of the appropriate output parameter 
        if (hdr1 == PRODUCT_NAME_HDR)
        {
          o_product_name = value;
        }
        else if (vpdcode == "PN")
        {
          o_part_number = value;
        }
        else if (vpdcode == "SN")
        {
          o_serial_number = value;
        }
          
        value.clear();

        if (bytesread == len)
        {
          state = READHDR1;
        }
        else
        {
          state = READVPDCODE1;
        }
      }
    }
    
//    LOG(csmd, debug) << "state=" << state 
//                     << hex << " hdr1=" << (int) hdr1 << " hdr2=" << (int) hdr2 << " len=" << (int) len << " sublen=" << sublen 
//                     << dec << " value.size()=" << value.size() << " nextchar=" << nextchar << " " 
//                     << hex << (int) nextchar << dec;
  }
  vpd_in.close();
}

void setHcaInventoryValue(const string& field, const string& value, char* dest_ptr, const uint32_t& DEST_MAX)
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
