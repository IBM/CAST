/*================================================================================
   
    csmd/src/inv/src/inv_get_node_inventory.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "csm_daemon_config.h"
#include "inv_get_node_inventory.h"
#include "inv_functions.h"

#include "csmi/src/common/include/csmi_serialization.h"
#include "logging.h"

#include <string.h>
#include <stdint.h>       // Provides int32_t
#include <sys/utsname.h>  // Provides uname()
#include <dirent.h>       // Provides scandir()
#include <sys/time.h>     // Provides gettimeofday()

#include <string>
#include <fstream>
#include <cctype>
#include <list>

using namespace std;

bool GetNodeInventory(csm_node_inventory_t& node_inventory)
{
  LOG(csmd, info) << "Enter " << __PRETTY_FUNCTION__;
  
  // Set any unused fields to appropriate defaults
  node_inventory.type[0] = '\0';  // Set in the daemon handlers, default to empty string
 
  // Collect the xCAT nodename from /xcatpost/mypostscript
  std::string xcatname("");

  // Try to collect the xcatname using global GetHostname() function 
  // Note: GetHostname() will fail during standalone unit testing
  try
  {
    xcatname = csm::daemon::Configuration::Instance()->GetHostname();
  }
  catch (csm::daemon::Exception &e)
  {
    //LOG(csmd, error) << "Caught exception when trying GetHostname(): " << e.what();
    LOG(csmd, error) << "Caught exception when trying GetHostname()";
  }

  if (!xcatname.empty())
  {
    LOG(csmd, info) << "xcatname (node_name) = " << xcatname;
    strncpy(node_inventory.node_name, xcatname.c_str(), CSM_NODE_NAME_MAX);
    node_inventory.node_name[CSM_NODE_NAME_MAX - 1] = '\0'; 
  }
  else
  {
    node_inventory.node_name[0] = '\0';
    LOG(csmd, error) << "Error: could not determine xcatname (node_name)!";
  }

  // Collect the xCAT osimage name and uuid from /opt/xcat/xcatinfo
  // Note: these values will not be set for all xCAT node types
  // osimage name and uuid should be set for all diskless nodes
  std::ifstream xcat_in("/opt/xcat/xcatinfo");
  std::string imagename("");
  std::string imageuuid("");
  std::string line("");
  const std::string IMAGENAME_TOKEN("IMAGENAME=");
  const std::string IMAGEUUID_TOKEN("IMAGEUUID=");
    
  while (getline(xcat_in, line)) 
  {
    if (line.substr(0, IMAGENAME_TOKEN.size()) == IMAGENAME_TOKEN)
    {
      // $ grep "^IMAGENAME=" /opt/xcat/xcatinfo 
      // IMAGENAME='rhels7.3-ppc64le-diskless-compute'
      // or
      // IMAGENAME=rhels7.5-ppc64le-install-compute-custom
    
      // Grab the imagename value 
      imagename = line.substr(IMAGENAME_TOKEN.size());

      // Remove any optional single quotes at the beginning or end of the string
      if (!imagename.empty() && imagename.front() == '\'')
      {
        imagename.erase(0,1);
      }

      if (!imagename.empty() && imagename.back() == '\'')
      {
        imagename.pop_back();
      }
    }
    else if (line.substr(0, IMAGEUUID_TOKEN.size()) == IMAGEUUID_TOKEN)
    {
      // $ grep "^IMAGEUUID=" /opt/xcat/xcatinfo 
      // IMAGEUUID='ecf0ec15-c4e8-4e8e-9090-84a1e5d1f77e'
      // or
      // IMAGEUUID=ecf0ec15-c4e8-4e8e-9090-84a1e5d1f77e
      
      // Grab the imageuuid value
      imageuuid = line.substr(IMAGEUUID_TOKEN.size());
      
      // Remove any optional single quotes at the beginning or end of the string
      if (!imageuuid.empty() && imageuuid.front() == '\'')
      {
        imageuuid.erase(0,1);
      }

      if (!imageuuid.empty() && imageuuid.back() == '\'')
      {
        imageuuid.pop_back();
      }
    }
  }
  xcat_in.close();
  
  if (!imagename.empty())
  {
    LOG(csmd, info) << "os_image_name = " << imagename;
    strncpy(node_inventory.os_image_name, imagename.c_str(), CSM_OS_IMAGE_NAME_MAX);
    node_inventory.os_image_name[CSM_OS_IMAGE_NAME_MAX - 1] = '\0'; 
  }
  else
  {
    node_inventory.os_image_name[0] = '\0';
    LOG(csmd, warning) << "Could not determine osimage name.";
  }
  
  if (!imageuuid.empty())
  {
    LOG(csmd, info) << "os_image_uuid = " << imageuuid;
    strncpy(node_inventory.os_image_uuid, imageuuid.c_str(), CSM_OS_IMAGE_UUID_MAX);
    node_inventory.os_image_uuid[CSM_OS_IMAGE_UUID_MAX - 1] = '\0'; 
  }
  else
  {
    node_inventory.os_image_uuid[0] = '\0';
    LOG(csmd, warning) << "Could not determine osimage uuid.";
  }
 
  // Collect the machine type
  std::ifstream cpuinfo_in("/proc/cpuinfo");
  std::string machinetype("");
  line.clear();
  const std::string MACHINE_TOKEN("machine");
  
  while (getline(cpuinfo_in, line)) 
  {
    if (line.substr(0, MACHINE_TOKEN.size()) == MACHINE_TOKEN)
    {
      // $ cat /proc/cpuinfo | grep machine
      // machine		: PowerNV 8335-GTA  
      // Collect everything after the ':'
      size_t colonindex = line.find(':');      
      
      if (colonindex != std::string::npos)
      {
        machinetype = line.substr(colonindex+2);
      }
    }
  }
  cpuinfo_in.close();

  machinetype = trimString(machinetype);

  if (!machinetype.empty())
  {
    LOG(csmd, info) << "machinetype = " << machinetype;
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine machinetype!";
  } 
    
  // Collect the machine type model
  std::ifstream model_in("/proc/device-tree/model");
  std::string model("");
  
  // $ cat /proc/device-tree/model 
  // 8335-GTA
  getline(model_in, model);
  model_in.close();
  model = trimString(model); 
  
  if (!model.empty())
  {
    LOG(csmd, info) << "model = " << model;
    strncpy(node_inventory.machine_model, model.c_str(), CSM_MACHINE_MODEL_MAX);
    node_inventory.machine_model[CSM_MACHINE_MODEL_MAX - 1] = '\0'; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine model!";
    node_inventory.machine_model[0] = '\0'; 
  } 
 
  // Collect the serial number
  std::ifstream sn_in("/proc/device-tree/system-id");
  std::string serialnumber("");
  
  // $ cat /proc/device-tree/system-id
  // 210317A  
  sn_in >> serialnumber;
  sn_in.close();
  serialnumber = trimString(serialnumber); 
 
  if (!serialnumber.empty())
  {
    LOG(csmd, info) << "serialnumber = " << serialnumber;
    strncpy(node_inventory.serial_number, serialnumber.c_str(), CSM_SERIAL_NUMBER_MAX);
    node_inventory.serial_number[CSM_SERIAL_NUMBER_MAX - 1] = '\0'; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine serialnumber!";
    node_inventory.serial_number[0] = '\0'; 
  } 

  // Collect the discovery timestamp
  // Example format:
  // 2016-05-12 15:12:11.799506
  struct timeval now_tv;
  time_t now_time_t;
  struct tm *now_tm;
  char tm_buf[64], tm_usec_buf[64];

  gettimeofday(&now_tv, NULL);
  now_time_t = now_tv.tv_sec;
  now_tm = localtime(&now_time_t);
  strftime(tm_buf, sizeof tm_buf, "%Y-%m-%d %H:%M:%S", now_tm);
  snprintf(tm_usec_buf, sizeof tm_usec_buf, "%s.%06lu", tm_buf, now_tv.tv_usec);
    
  strncpy(node_inventory.collection_time, tm_usec_buf, CSM_COLLECTION_TIME_MAX);
  node_inventory.collection_time[CSM_COLLECTION_TIME_MAX - 1] = '\0'; 
  
  LOG(csmd, info) << "collection_time = " << tm_usec_buf;
 
  // Collect the power cap information?

  // Collect the kernel version information using uname
  struct utsname unameinfo;
  int32_t rc(0);
  rc = uname(&unameinfo);
  if (rc == 0)
  {
    LOG(csmd, info) << "uname sysname = " << unameinfo.sysname;
    LOG(csmd, info) << "uname nodename (hostname) = " << unameinfo.nodename;
    LOG(csmd, info) << "uname release = " << unameinfo.release;
    LOG(csmd, info) << "uname version = " << unameinfo.version;
    LOG(csmd, info) << "uname machine = " << unameinfo.machine;
    
    strncpy(node_inventory.kernel_release, unameinfo.release, CSM_KERNEL_RELEASE_MAX);
    node_inventory.kernel_release[CSM_KERNEL_RELEASE_MAX - 1] = '\0'; 
    
    strncpy(node_inventory.kernel_version, unameinfo.version, CSM_KERNEL_VERSION_MAX);
    node_inventory.kernel_version[CSM_KERNEL_VERSION_MAX - 1] = '\0'; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine uname information! rc = " << rc;
    node_inventory.kernel_release[0] = '\0'; 
    node_inventory.kernel_version[0] = '\0'; 
  } 

  // Collect the discovered processors (NUMA nodes with associated CPUs)
  // First find all of the NUMA nodes, later we will check to see if they have associated CPUs
  // $ ls /sys/devices/system/node | grep node
  // node0
  // node252
  // node253
  // node254
  // node255
  // node8
  const char NODE_PATH[] = "/sys/devices/system/node";
  const std::string NUMA_NODE_TOKEN("node");
  std::list<std::string> numanodelist;
  std::list<std::string> numanodecpulist;   // Used to store subset of NUMA nodes that have associated CPUs
  
  struct dirent **filelist;
  int32_t filecount;

  // Return a list of all files in NODE_PATH
  filecount = scandir(NODE_PATH, &filelist, NULL, alphasort);
  if (filecount < 0)
  {
    perror("scandir");
  }
  else 
  {
    //LOG(csmd, info) << "filecount = " << filecount;
   
    // Walk through the list of files looking for files that match NUMA_NODE_TOKEN 
    for (int32_t i=0; i < filecount; i++)
    {
      rc = strncmp(filelist[i]->d_name, NUMA_NODE_TOKEN.c_str(), NUMA_NODE_TOKEN.size());  
      if (rc == 0)
      {
        //LOG(csmd, info) << "Found NUMA node = " << filelist[i]->d_name;
        numanodelist.push_back(filelist[i]->d_name);
      }
      free(filelist[i]);  // Free each entry in the file list
    }
    free(filelist);  // Free the file list itself
    filelist = nullptr;
    filecount = 0;
  }
  
  // Determine the number of CPUs per NUMA node
  // cat /sys/devices/system/node/node0/cpumap 
  // 0000,00000000,00000000,00ffffff,ffffffff,ffffffff
  // Count the bits set to determine the number of CPUs per NUMA node
  uint32_t discoveredsockets(0);
  
  // Used to look up how many bits are set in the hex nibble
  const unsigned char bitssettable[16] = 
  {  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
  // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
 
  // Count up the CPUs in the cpumap for each NUMA node 
  for (std::list<std::string>::iterator numa_itr = numanodelist.begin(); numa_itr != numanodelist.end(); numa_itr++)
  {
    //LOG(csmd, trace) << "Found NUMA node = " << *numa_itr;
    
    std::ifstream cpumap_in("/sys/devices/system/node/" + *numa_itr + "/cpumap");
    uint32_t numanodecpus(0);
    char nextchar('\0');
    uint32_t index(0);

    while (cpumap_in.get(nextchar)) 
    {
      if (isxdigit(nextchar))
      {
        if ((nextchar >= '0') && (nextchar <= '9'))
        { 
          index = nextchar - '0';
        }
        else if ((nextchar >= 'a') && (nextchar <= 'f'))
        {
          index = nextchar - 'a' + 10;
        }
        else if ((nextchar >= 'A') && (nextchar <= 'F'))
        {
          index = nextchar - 'A' + 10;
        } 

        if (index < 16)
        {
          numanodecpus += bitssettable[index];
        }
        else
        {
          LOG(csmd, error) << "Error: unexpected hex conversion from " << nextchar << " to " << index;
        }
      }
    }
    cpumap_in.close();
          
    LOG(csmd, debug) << "NUMA " << *numa_itr << " has " << numanodecpus << " cpus in the cpumap.";
  
    // Only count NUMA nodes with associated CPUs when calculating the discoveredsockets
    if (numanodecpus > 0)
    {
      discoveredsockets++;
      numanodecpulist.push_back(*numa_itr);
    }
  } 

  if (discoveredsockets != 0)
  {
    LOG(csmd, info) << "discoveredsockets = " << discoveredsockets;
    node_inventory.discovered_sockets = discoveredsockets; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine discoveredsockets!";
    node_inventory.discovered_sockets = 0; 
  } 
  
  // Collect the total DIMM memory
  // P9 servers report both DIMM memory and GPU memory in /proc/meminfo MemTotal
  // Note: GPU memory must first be brought online by running nvidia-persistenced for it to be counted
  // Example:
  //
  // # ps -ef | grep [n]vidia-persistenced
  // root      21175      1  0 09:40 ?        00:00:14 /usr/bin/nvidia-persistenced --user root
  //
  // # cat /sys/devices/system/node/has_memory 
  // 0,8,252-255
  //
  // # cat /sys/devices/system/node/has_normal_memory 
  // 0,8
  //
  // # cat /sys/devices/system/node/node*/meminfo | grep "MemTotal"
  // Node 0 MemTotal:       263931968 kB
  // Node 252 MemTotal:       16515072 kB
  // Node 253 MemTotal:       16515072 kB
  // Node 254 MemTotal:       16515072 kB
  // Node 255 MemTotal:       16515072 kB
  // Node 8 MemTotal:       268031040 kB
  // 
  // #  cat /sys/devices/system/node/node*/meminfo | awk '/MemTotal/ {sum += $4} END {print sum}' 
  // 598023296
  //
  // # cat /proc/meminfo | grep "MemTotal"
  // MemTotal:       598023296 kB
  //
  // In order to only count the DIMM memory, count only the total memory associated with
  // NUMA nodes that have CPUs and ignore the GPU NUMA nodes  
  uint64_t memtotal(0);

  // Count up the MemTotal from each NUMA node that contains CPUs
  for (std::list<std::string>::iterator numa_itr = numanodecpulist.begin(); numa_itr != numanodecpulist.end(); numa_itr++)
  {
    //LOG(csmd, trace) << "Found CPU NUMA node = " << *numa_itr;
   
    std::ifstream meminfo_in("/sys/devices/system/node/" + *numa_itr + "/meminfo");
    uint64_t numanodemem(0);
    std::string numanodememunits("");
    std::string token("");
  
    while (meminfo_in >> token) 
    {
      if (token == "MemTotal:")
      {
        // cat /sys/devices/system/node/node0/meminfo | grep MemTotal
        // Node 0 MemTotal:       263931968 kB
        meminfo_in >> numanodemem >> numanodememunits;
      }
    }
    meminfo_in.close();
  
    if (numanodememunits == "kB")
    {
      LOG(csmd, debug) << "NUMA " << *numa_itr << " memtotal = " << numanodemem;
      memtotal += numanodemem; 
    }
    else
    {
      LOG(csmd, error) << "Error: could not determine memtotal for NUMA " << *numa_itr << "!";
    } 
  } 
  
  if (memtotal != 0)
  {
    LOG(csmd, info) << "memtotal = " << memtotal;
    node_inventory.installed_memory = memtotal; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine memtotal!";
    node_inventory.installed_memory = 0; 
  } 

  // Collect the total swap from /proc/meminfo
  std::ifstream meminfo_in("/proc/meminfo");
  uint64_t swaptotal(0);
  std::string swaptotalunits("");
  std::string token("");

  while (meminfo_in >> token) 
  {
    if (token == "SwapTotal:")
    {
      // $ cat /proc/meminfo | grep SwapTotal:
      // SwapTotal:       4194240 kB
      meminfo_in >> swaptotal >> swaptotalunits;
    } 
  }
  meminfo_in.close();
  
  if (swaptotalunits == "kB")
  {
    LOG(csmd, info) << "swaptotal = " << swaptotal;
    node_inventory.installed_swap = swaptotal; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine swaptotal!";
    node_inventory.installed_swap = 0; 
  } 

  // Collect the total discovered cores
  // ls /proc/device-tree/cpus/ | grep -c PowerPC
  // 20 
  uint32_t discoveredcores(0);
  const char CORE_PATH[] = "/proc/device-tree/cpus/";
  const std::string CORE_TOKEN("PowerPC");
  
  // Return a list of all files in CORE_PATH
  filecount = scandir(CORE_PATH, &filelist, NULL, alphasort);
  if (filecount < 0)
  {
    perror("scandir");
  }
  else 
  {
    //LOG(csmd, info) << "filecount = " << filecount;
   
    // Walk through the list of files looking for files that match CORE_TOKEN 
    for (int32_t i=0; i < filecount; i++)
    {
      rc = strncmp(filelist[i]->d_name, CORE_TOKEN.c_str(), CORE_TOKEN.size());  
      if (rc == 0)
      {
        //LOG(csmd, info) << "Found core = " << filelist[i]->d_name;
        discoveredcores++;
      }
      free(filelist[i]);  // Free each entry in the file list
    }
    free(filelist);  // Free the file list itself
    filelist = nullptr;
    filecount = 0;
  }
  
  if (discoveredcores != 0)
  {
    LOG(csmd, info) << "discoveredcores = " << discoveredcores;
    node_inventory.discovered_cores = discoveredcores; 
  }
  else
  {
    LOG(csmd, error) << "Error: could not determine discoveredcores!";
    node_inventory.discovered_cores = 0; 
  } 
  
  LOG(csmd, info) << "Exit " << __PRETTY_FUNCTION__;
  return true;
}

csmi_cmd_t GetCommandNodeInventory()
{
  return CSM_CMD_INV_get_node_inventory;
}

int GetPayloadNodeInventory(csm_node_inventory_t *node, char **payload)
{
  packPrototype packFunc;
  csmi_cdata_t csmi_cdata;
  csmi_buf_t *csmi_buf=nullptr;
  int payload_len;
  
  packFunc = csmi_pack_get(GetCommandNodeInventory());

  if (packFunc == NULL) 
  {
    LOG(csmd, error) << "GetPayloadNodeInventory: Fail to retrieve the pack function";
    return 0;
  }

  csmi_cdata.cdataLen = 1;
  csmi_cdata.cdata = node;
  csmi_buf = packFunc(GetCommandNodeInventory(), &csmi_cdata);
  
  if (csmi_buf == NULL)
  {
    LOG(csmd, error) << "GetPayloadNodeInventory: Fail to pack the data";
    return 0;
  }
  
  *payload = csmi_buf->buf;
  payload_len = csmi_buf->bufLen;
  
  free(csmi_buf);
  
  return payload_len;
}
