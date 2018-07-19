/*================================================================================

    csmd/src/daemon/src/csm_environmental_data.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    /U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_environmental_data.h"

#include "csm_bds_keys.h"
#include "csm_daemon_config.h"
#include "OCCSensorData.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <sstream>
#include <string>
#include <unordered_map>

#include <sys/time.h>
#include <stdint.h>


CSM_Environmental_Data::CSM_Environmental_Data() :
  _version(CSM_ENVIRONMENTAL_DATA_V1),
  _archive_mask(),
  _source_node(),
  _timestamp(),
  _data_list()
{
   // For version 1, serialize these fields:
   if (_version == CSM_ENVIRONMENTAL_DATA_V1)
   {
      _archive_mask.set(SOURCE_NODE_BIT);
      _archive_mask.set(TIMESTAMP_BIT);
      _archive_mask.set(DATA_LIST_BIT);
   }
}

CSM_Environmental_Data::~CSM_Environmental_Data()
{
}

void CSM_Environmental_Data::Print()
{
   LOG(csmenv, debug) << "ENVDATA: version: " << _version << " archive_mask: " << _archive_mask.to_string();
   LOG(csmenv, debug) << GetJsonString(); 
}

// Helper function to copy parts of one ptree to the other
// CopyPtSubtree(src, dst, ""); -> copy all keys in the tree recursively
// CopyPtSubtree(src, dst, "data"); -> copy all keys under "data" in the tree recursively
bool CopyPtSubtree(const boost::property_tree::ptree &src_pt, boost::property_tree::ptree &dst_pt, const std::string &key, int32_t count=0)
{
  const int32_t MAX_COUNT(200);

  if (count > MAX_COUNT)
  {
    LOG(csmenv, warning) << "CopyPtSubtree: MAX_COUNT exceeded";
    return false;
  }

  try
  {
    for (const auto &child : src_pt.get_child(key))
    {
      std::string fullkey = key.empty() ? child.first : key + "." + child.first; 
      count++;
      
      if ( child.second.empty() )
      {
        //LOG(csmenv, debug) << "CopyPtSubtree:  " << fullkey << ": " << child.second.data() << " count=" << count;
        dst_pt.put(fullkey, child.second.data());
      }
      else
      {
        //LOG(csmenv, debug) << "CopyPtSubtree:  " << fullkey << ": " << child.second.data() << " count=" << count;
        CopyPtSubtree(src_pt, dst_pt, fullkey, count);
      }
    }      
  } 
  catch (...)
  {
    LOG(csmenv, warning) << "CopyPtSubtree: Caught exception while attempting to copy subtree " << key;
    return false;
  }      

  return true;
}     
 
std::string CSM_Environmental_Data::GetJsonString()
{
  std::string json("");

  // This function will return a series of json documents in a single string
  // Each json document has a set of common parent fields followed by fields specific to 
  // the type of environmental data being collected
  for (auto data_itr = _data_list.begin(); data_itr != _data_list.end(); data_itr++)
  {
    // Set the top level fields into the json
    const std::string EMPTY_STRING("");
    std::string key_type = data_itr->get(CSM_BDS_KEY_TYPE, EMPTY_STRING);

    if (key_type == EMPTY_STRING)
    {
      LOG(csmenv, error) << "Found data item with unknown " << CSM_BDS_KEY_TYPE << " key, skipping.";
    }
    else
    {    
      boost::property_tree::ptree data_pt;
  
      // Add the common fields to the new object
      data_pt.put(CSM_BDS_KEY_TYPE, key_type);   
      data_pt.put(CSM_BDS_KEY_SOURCE, _source_node);   
      data_pt.put(CSM_BDS_KEY_TIME_STAMP, _timestamp);

      // Recursively copy the fields in the data section to the new object 
      bool copy_success = CopyPtSubtree(*data_itr, data_pt, CSM_BDS_SECTION_DATA);
      if (copy_success == true)
      {
        std::ostringstream data_oss;
        boost::property_tree::json_parser::write_json(data_oss, data_pt, false);
        data_oss << std::endl;
        json += data_oss.str();
      }
      else
      {
        LOG(csmenv, error) << "Found data item with no data fields set, skipping.";
      }
    }
  }
 
  //LOG(csmenv, debug) << json;
 
  return json;
}

void CSM_Environmental_Data::CollectNodeData()
{
  // Set _timestamp
  char time_stamp_buffer[80];
  char time_stamp_with_usec[80];

  struct timeval now_tv;
  time_t rawtime;
  struct tm *info;

  gettimeofday(&now_tv, NULL);
  rawtime = now_tv.tv_sec;
  info = localtime( &rawtime );

  strftime(time_stamp_buffer, 80, "%Y-%m-%d %H:%M:%S", info);
  snprintf(time_stamp_with_usec, 80, "%s.%06lu", time_stamp_buffer, now_tv.tv_usec);
  _timestamp = time_stamp_with_usec;

  // Set _source_node
  try
  {
    _source_node = csm::daemon::Configuration::Instance()->GetHostname();
  }
  catch (csm::daemon::Exception &e)
  {
    LOG(csmenv, error) << "Caught exception when trying GetHostname()";
  }
}

bool CSM_Environmental_Data::CollectEnvironmentalData()
{
   LOG(csmenv, debug) << "Start CollectEnvironmentalData()";
   
   const std::list<std::string> node_sensors =
   {
      "PWRSYS",
   };   
   
   const std::list<std::string> chip_sensors =
   {
      "PWRPROC",
      "PWRGPU",
      "PWRMEM",
      "TEMPNEST"
   };   
   
   const std::list<std::list<std::string>> gpu_sensors =
   {
      { "TEMPGPU0", "TEMPGPU0MEM" }, // GPU 0
      { "TEMPGPU1", "TEMPGPU1MEM" }, // GPU 1
      { "TEMPGPU2", "TEMPGPU2MEM" }  // GPU 2
   };   
 
   const std::list<std::string> dimm_sensors =
   {
      "TEMPDIMM02",
      "TEMPDIMM03",
      "TEMPDIMM04",
      "TEMPDIMM05",
      "TEMPDIMM10",
      "TEMPDIMM11",
      "TEMPDIMM12",
      "TEMPDIMM13"
   };
   
   // Generate the value map for the query.
   std::unordered_map<std::string, csm::daemon::helper::CsmOCCSensorRecord> request_map =
   {
      {"PWRSYS",       {0,0,0,0}},
      {"PWRPROC",      {0,0,0,0}},
      {"PWRGPU",       {0,0,0,0}},
      {"PWRMEM",       {0,0,0,0}},
      {"TEMPNEST",     {0,0,0,0}},
      {"TEMPGPU0",     {0,0,0,0}},
      {"TEMPGPU0MEM",  {0,0,0,0}},
      {"TEMPGPU1",     {0,0,0,0}},
      {"TEMPGPU1MEM",  {0,0,0,0}},
      {"TEMPGPU2",     {0,0,0,0}},
      {"TEMPGPU2MEM",  {0,0,0,0}},
      {"TEMPDIMM02",   {0,0,0,0}},
      {"TEMPDIMM03",   {0,0,0,0}},
      {"TEMPDIMM04",   {0,0,0,0}},
      {"TEMPDIMM05",   {0,0,0,0}},
      {"TEMPDIMM10",   {0,0,0,0}},
      {"TEMPDIMM11",   {0,0,0,0}},
      {"TEMPDIMM12",   {0,0,0,0}},
      {"TEMPDIMM13",   {0,0,0,0}}
   };

   // Query and check for success.
   std::vector<std::unordered_map<std::string, csm::daemon::helper::CsmOCCSensorRecord>> current_values;
   
   bool success = csm::daemon::helper::GetExtendedOCCSensorData(request_map, current_values);
   boost::property_tree::ptree env_pt;

   if (success)
   {
      uint8_t gpu_id(0);
      uint8_t dimm_id(0);

      for (uint32_t chip = 0; chip < current_values.size(); chip++)
      {
         // Node level data (full system sensors are associated with chip 0 by OCC)
         if (chip == 0)
         {
            boost::property_tree::ptree node_pt;
            node_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_NODE_ENV);

            for (auto node_itr = node_sensors.begin(); node_itr != node_sensors.end(); node_itr++)
            {
               auto occ_itr = current_values[chip].find(*node_itr);
               if (occ_itr != current_values[chip].end())
               {
                  if (*node_itr == "PWRSYS")
                  {
                     //node_pt.put( "data.system_power", std::to_string(occ_itr->second.sample) );
                     node_pt.put( "data.system_energy", std::to_string(occ_itr->second.accumulator) );
                  }
               }
            }

            if (node_pt.count("data") > 0)
            {
               _data_list.push_back(node_pt);
            }
         }     
         
         // Processor socket level data
         boost::property_tree::ptree chip_pt;
         chip_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_PROCESSOR_ENV);
         bool has_chip_data(false);        

         // lambda used to insert chip id data as the first elements in data when a sensor match occurs 
         auto check_and_insert_chip_id_fields = [&]()
         {
            if (!has_chip_data)
            {
               has_chip_data = true;
               chip_pt.put( "data.processor_id", std::to_string(chip) );
               //chip_pt.put( "data.serial_number", "ABC123" );
            }
         };

         for (auto chip_itr = chip_sensors.begin(); chip_itr != chip_sensors.end(); chip_itr++)
         {
            auto occ_itr = current_values[chip].find(*chip_itr);
            if (occ_itr != current_values[chip].end())
            {
               if (*chip_itr == "PWRPROC")
               {
                  check_and_insert_chip_id_fields();
                  //chip_pt.put( "data.processor_power", std::to_string(occ_itr->second.sample) );
                  chip_pt.put( "data.processor_energy", std::to_string(occ_itr->second.accumulator) );
               }
               else if (*chip_itr == "PWRGPU")
               {
                  check_and_insert_chip_id_fields();
                  //chip_pt.put( "data.gpu_power", std::to_string(occ_itr->second.sample) );
                  chip_pt.put( "data.gpu_energy", std::to_string(occ_itr->second.accumulator) );
               }
               else if (*chip_itr == "PWRMEM")
               {
                  check_and_insert_chip_id_fields();
                  //chip_pt.put( "data.memory_power", std::to_string(occ_itr->second.sample) );
                  chip_pt.put( "data.memory_energy", std::to_string(occ_itr->second.accumulator) );
               }
               else if (*chip_itr == "TEMPNEST")
               {
                  check_and_insert_chip_id_fields();
                  chip_pt.put( "data.processor_temp", std::to_string(occ_itr->second.sample) );
                  chip_pt.put( "data.processor_temp_min", std::to_string(occ_itr->second.csm_min) );
                  chip_pt.put( "data.processor_temp_max", std::to_string(occ_itr->second.csm_max) );
               }
            }
         }

         if (has_chip_data)
         {
            _data_list.push_back(chip_pt);
         }
        
         // GPU level data
         for (auto gpu_itr = gpu_sensors.begin(); gpu_itr != gpu_sensors.end(); gpu_itr++)
         {
            boost::property_tree::ptree gpu_pt;
            gpu_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_GPU_ENV);
            bool has_gpu_data(false);        

            // lambda used to insert gpu id data as the first elements in data when a sensor match occurs 
            auto check_and_insert_gpu_id_fields = [&]()
            {
               if (!has_gpu_data)
               {
                  has_gpu_data = true;
                  gpu_pt.put( "data.gpu_id", std::to_string(gpu_id) );
                  //gpu_pt.put( "data.serial_number", "ABC123" );
               }
            };

            for (auto sensor_itr = gpu_itr->begin(); sensor_itr != gpu_itr->end(); sensor_itr++)
            {
               auto occ_itr = current_values[chip].find(*sensor_itr);
               if (occ_itr != current_values[chip].end())
               {
                  if ( (*sensor_itr == "TEMPGPU0") || (*sensor_itr == "TEMPGPU1") || (*sensor_itr == "TEMPGPU2") ) 
                  {
                     check_and_insert_gpu_id_fields();
                     gpu_pt.put( "data.gpu_temp", std::to_string(occ_itr->second.sample) );
                     gpu_pt.put( "data.gpu_temp_min", std::to_string(occ_itr->second.csm_min) );
                     gpu_pt.put( "data.gpu_temp_max", std::to_string(occ_itr->second.csm_max) );
                  }
                  else if ( (*sensor_itr == "TEMPGPU0MEM") || (*sensor_itr == "TEMPGPU1MEM") || (*sensor_itr == "TEMPGPU2MEM") ) 
                  {
                     check_and_insert_gpu_id_fields();
                     gpu_pt.put( "data.gpu_mem_temp", std::to_string(occ_itr->second.sample) );
                     gpu_pt.put( "data.gpu_mem_temp_min", std::to_string(occ_itr->second.csm_min) );
                     gpu_pt.put( "data.gpu_mem_temp_max", std::to_string(occ_itr->second.csm_max) );
                  }
               }
            }
      
            if (has_gpu_data)
            {
               _data_list.push_back(gpu_pt);
            }
            
            gpu_id++;
         }
 
         // Dimm level data 
         for (auto dimm_itr = dimm_sensors.begin(); dimm_itr != dimm_sensors.end(); dimm_itr++)
         {
            boost::property_tree::ptree dimm_pt;
      
            dimm_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_DIMM_ENV);
            dimm_pt.put( "data.dimm_id", std::to_string(dimm_id) );
            //dimm_pt.put( "data.serial_number", "ABC123" );
         
            auto occ_itr = current_values[chip].find(*dimm_itr);
            if (occ_itr != current_values[chip].end())
            {
               dimm_pt.put( "data.dimm_temp", std::to_string(occ_itr->second.sample) );
               dimm_pt.put( "data.dimm_temp_min", std::to_string(occ_itr->second.csm_min) );
               dimm_pt.put( "data.dimm_temp_max", std::to_string(occ_itr->second.csm_max) );
            }      

            _data_list.push_back(dimm_pt);
            dimm_id++;
         }        
         
         //for (auto current_itr = current_values[chip].begin(); current_itr != current_values[chip].end(); current_itr++)
         //{
         //   LOG(csmenv, debug) << "ENV: Read OCC data " << current_itr->first << ": current: " << current_itr->second.sample
         //      << " min: " << current_itr->second.csm_min << " max: " << current_itr->second.csm_max
         //      << " accumulator: " << current_itr->second.accumulator;

         //   env_pt.put(current_itr->first, current_itr->second.sample);
         //}
      }
   }

   LOG(csmenv, debug) << "Finish CollectEnvironmentalData()";

   return success;
}

void CSM_Environmental_Data::AddDataItems(const std::list<boost::property_tree::ptree> &data_pt_list)
{
   _data_list.insert(_data_list.end(), data_pt_list.begin(), data_pt_list.end());
}

void CSM_Environmental_Data::AddDataItem(const boost::property_tree::ptree &data_pt)
{
   _data_list.push_back(data_pt);
}

bool CSM_Environmental_Data::HasData() const
{
  return !_data_list.empty();
}
