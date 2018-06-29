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


CSM_Environmental_Data::CSM_Environmental_Data()
{
  _Data_Mask.reset();
}
  
CSM_Environmental_Data::CSM_Environmental_Data( const CSM_Environmental_Data& in ) : 
  _Data_Mask( in._Data_Mask ),
  _source_node( in._source_node ),
  _timestamp ( in._timestamp ),
  _CPU_Data( in._CPU_Data ),
  _GPU_Double_Data( in._GPU_Double_Data ),
  _GPU_Long_Data( in._GPU_Long_Data ),
  _GPU_Double_Label_Data( in._GPU_Double_Label_Data ),
  _GPU_Long_Label_Data( in._GPU_Long_Label_Data ),
  _data_list( in._data_list )
{
}

CSM_Environmental_Data::~CSM_Environmental_Data()
{
}

void CSM_Environmental_Data::Get_GPU_Double_DCGM_Field_Values_And_Set_Bit() 
{ 
  _GPU_Double_Data.Get_Double_DCGM_Field_Values();
  _Data_Mask.set( GPU_DOUBLE_DATA_BIT ); 
}

void CSM_Environmental_Data::Get_GPU_Long_DCGM_Field_Values_And_Set_Bit() 
{
  _GPU_Long_Data.Get_Long_DCGM_Field_Values();
  _Data_Mask.set( GPU_LONG_DATA_BIT );
}

void CSM_Environmental_Data::Get_GPU_Double_DCGM_Field_String_Identifiers_And_Set_Bit()
{ 
  _GPU_Double_Label_Data.Get_Double_DCGM_Field_String_Identifiers();
  _Data_Mask.set( GPU_DOUBLE_LABEL_BIT ); 
}

void CSM_Environmental_Data::Get_GPU_Long_DCGM_Field_String_Identifiers_And_Set_Bit()
{
  _GPU_Long_Label_Data.Get_Long_DCGM_Field_String_Identifiers();
  _Data_Mask.set( GPU_LONG_LABEL_BIT );
}

void CSM_Environmental_Data::Print_GPU_Double_DCGM_Field_Values()
{
  _GPU_Double_Data.Print_Double_DCGM_Field_Values();
}

void CSM_Environmental_Data::Print_GPU_Long_DCGM_Field_Values()
{
  _GPU_Long_Data.Print_Long_DCGM_Field_Values();
}

void CSM_Environmental_Data::Print_GPU_Double_DCGM_Field_String_Identifiers()
{
  _GPU_Double_Label_Data.Print_Double_DCGM_Field_String_Identifiers();
}

void CSM_Environmental_Data::Print_GPU_Long_DCGM_Field_String_Identifiers()
{
  _GPU_Long_Label_Data.Print_Long_DCGM_Field_String_Identifiers();
}

void CSM_Environmental_Data::Print()
{
  LOG( csmenv, debug ) << " ENVDATA: BitSet:" << _Data_Mask.to_string();

  if( _Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) ){ Print_GPU_Double_DCGM_Field_String_Identifiers(); }
  if( _Data_Mask.test( GPU_DOUBLE_DATA_BIT ) ){ Print_GPU_Double_DCGM_Field_Values(); }

  if( _Data_Mask.test( GPU_LONG_LABEL_BIT ) ){ Print_GPU_Long_DCGM_Field_String_Identifiers(); }
  if( _Data_Mask.test( GPU_LONG_DATA_BIT ) ){ Print_GPU_Long_DCGM_Field_Values(); }

  /*
  if( _Data_Mask.test( CPU_DATA_BIT ) ){ LOG( csmd, debug ) << "       DummyInt = " << _CPUData._DummyTestInt; }
  */
}

CSM_CPU_Data& CSM_Environmental_Data::Return_CPU_Data_Object()
{ 
  return (this->_CPU_Data);
}

CSM_GPU_Double_Data& CSM_Environmental_Data::Return_GPU_Double_Data_Object()
{
  return (this->_GPU_Double_Data);
}

CSM_GPU_Long_Data& CSM_Environmental_Data::Return_GPU_Long_Data_Object()
{ 
  return (this->_GPU_Long_Data); 
}

CSM_GPU_Double_Label_Data& CSM_Environmental_Data::Return_GPU_Double_Label_Data_Object()
{ 
  return (this->_GPU_Double_Label_Data);
}

CSM_GPU_Long_Label_Data& CSM_Environmental_Data::Return_GPU_Long_Label_Data_Object()
{
  return (this->_GPU_Long_Label_Data);
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Double_Data& GPU_Double_Data_To_Copy )
{
  _GPU_Double_Data = GPU_Double_Data_To_Copy;
  _Data_Mask.set( GPU_DOUBLE_DATA_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Long_Data& GPU_Long_Data_To_Copy )
{
  _GPU_Long_Data = GPU_Long_Data_To_Copy;
  _Data_Mask.set( GPU_LONG_DATA_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Double_Label_Data& GPU_Double_Label_Data_To_Copy )
{
  _GPU_Double_Label_Data = GPU_Double_Label_Data_To_Copy;
  _Data_Mask.set( GPU_DOUBLE_LABEL_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Long_Label_Data& GPU_Long_Label_Data_To_Copy )
{
  _GPU_Long_Label_Data = GPU_Long_Label_Data_To_Copy;
  _Data_Mask.set( GPU_LONG_LABEL_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_CPU_Data& CPU_data_to_copy )
{
  _CPU_Data = CPU_data_to_copy;
  _Data_Mask.set( CPU_DATA_BIT );
}

bool CSM_Environmental_Data::Set_Labels( const CSM_Environmental_Data& in )
{
  // Attempt to copy associated labels from in to this object for any data that is set in this object
  if( _Data_Mask.test( GPU_DOUBLE_DATA_BIT ) )
  {
    if( in._Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) )
    {
      Set_Data( in._GPU_Double_Label_Data );
    }
    else
    {
      return false;
    }
  }
  
  if( _Data_Mask.test( GPU_LONG_DATA_BIT ) )
  {
    if( in._Data_Mask.test( GPU_LONG_LABEL_BIT ) )
    {
      Set_Data( in._GPU_Long_Label_Data );
    }
    else
    {
      return false;
    }
  }

  return true;
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
 
std::string CSM_Environmental_Data::Get_Json_String()
{
  std::string json("");

  // This function will return a series of json documents in a single string
  // Each json document has a set of common parent fields followed by fields specific to 
  // the type of environmental data being collected
  // Build the common fields first, to be used in each of the individual json documents
  
  #define CSM_ENV_DATA_GPU_KEY_SERIAL_NUMBER "serial_number"

  // Create any GPU json documents 
  if ( _Data_Mask.test(GPU_DOUBLE_DATA_BIT) || _Data_Mask.test(GPU_LONG_DATA_BIT) )
  {
    std::vector<double> gpu_double_data;
    std::vector<std::string> gpu_double_labels;
    
    std::vector<long> gpu_long_data;
    std::vector<std::string> gpu_long_labels;
    
    if ( _Data_Mask.test(GPU_DOUBLE_DATA_BIT) )
    {
      gpu_double_data = _GPU_Double_Data.Return_Double_DCGM_Vector();
      gpu_double_labels = _GPU_Double_Label_Data.Return_Double_DCGM_Field_String_Identifiers();
    }  
    
    if ( _Data_Mask.test(GPU_LONG_DATA_BIT) )
    {
      gpu_long_data = _GPU_Long_Data.Return_Long_DCGM_Vector();
      gpu_long_labels = _GPU_Long_Label_Data.Return_Long_DCGM_Field_String_Identifiers();
    }

    // Data for each GPU has to be created from pieces stored in four different areas
    // Do one big consistency check here before proceeding
    // For both double data and long data, we should have N times as many entries in the data vector as the label vector
    // where N is the number of GPUs on the node. We then need to reassemble the data into N json documents, one for each GPU.
    // We need to make sure that both the long data and double data both contains data for an equal number of GPUs.
    // We also need to support the case where there may only be long data or only double data.
    uint32_t gpu_double_count(0);
    uint32_t gpu_long_count(0);
    
    if ( (gpu_double_labels.size() > 0) && (gpu_double_data.size() % gpu_double_labels.size() == 0) )
    {
      gpu_double_count = (gpu_double_data.size() / gpu_double_labels.size());
    }
    
    if ( (gpu_long_labels.size() > 0) && (gpu_long_data.size() % gpu_long_labels.size() == 0) )
    {
      gpu_long_count = (gpu_long_data.size() / gpu_long_labels.size());
    }

    if ( ( gpu_double_count == 0 && gpu_long_count == 0 ) ||
         ( gpu_double_count > 0 && gpu_long_count > 0 && gpu_double_count != gpu_long_count ) ) 
    {
        LOG(csmenv, error) << "Inconsistent GPU data detected: gpu_double_data.size() = " << gpu_double_data.size() 
                           << ", gpu_double_labels.size() = " << gpu_double_labels.size()
                           << ", gpu_long_data.size() = " << gpu_long_data.size()
                           << ", gpu_long_labels.size() = " << gpu_long_labels.size();
    } 
    else
    {
      uint32_t gpu_count(gpu_long_count); 

      uint32_t j(0);  // index into gpu_double_data vector
      uint32_t y(0);  // index into gpu_long_data vector
      
      for (uint32_t gpu = 0; gpu < gpu_count; gpu++)
      {
        // Set the top level fields into the json
        boost::property_tree::ptree gpu_pt;
        gpu_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_GPU_COUNTERS);
        gpu_pt.put(CSM_BDS_KEY_SOURCE, _source_node);   
        gpu_pt.put(CSM_BDS_KEY_TIME_STAMP, _timestamp);   
  
        // Temporarily set an artificial serial_number
        gpu_pt.put( std::string(CSM_BDS_SECTION_DATA) + "." + CSM_ENV_DATA_GPU_KEY_SERIAL_NUMBER, std::to_string(gpu) );

        for ( uint32_t i = 0; i < gpu_double_labels.size() && j < gpu_double_data.size(); i++ )
        {
          gpu_pt.put( CSM_BDS_SECTION_DATA "." + gpu_double_labels[i], std::to_string(gpu_double_data[j]) );
          j++;
        }       
        
        for ( uint32_t x = 0; x < gpu_long_labels.size() && y < gpu_long_data.size(); x++ )
        {
          gpu_pt.put( CSM_BDS_SECTION_DATA "." + gpu_long_labels[x], std::to_string(gpu_long_data[y]) );
          y++;
        } 
  
        // Generate the json for this GPU and add it to overall json string
        std::ostringstream gpu_oss;
        boost::property_tree::json_parser::write_json(gpu_oss, gpu_pt, false);
        gpu_oss << std::endl;
        json += gpu_oss.str();
      } 
    }
  }

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
 
  //LOG( csmenv, debug ) << json;
 
  return json;
}

void CSM_Environmental_Data::Collect_Node_Data()
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
    LOG(csmd, error) << "Caught exception when trying GetHostname()";
  }
}

bool CSM_Environmental_Data::Collect_Environmental_Data()
{
   LOG(csmenv, debug) << "Start Collect_Environmental_Data()";
   
   const std::list<std::string> node_sensors =
   {
      "PWRSYS",
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
      {"PWRSYS"      , {0,0,0,0}},
      {"PWRGPU"      , {0,0,0,0}},
      {"TEMPDIMM02",   {0,0,0,0}},
      {"TEMPDIMM03",   {0,0,0,0}},
      {"TEMPDIMM04",   {0,0,0,0}},
      {"TEMPDIMM05",   {0,0,0,0}},
      {"TEMPDIMM10",   {0,0,0,0}},
      {"TEMPDIMM11",   {0,0,0,0}},
      {"TEMPDIMM12",   {0,0,0,0}},
      {"TEMPDIMM13",   {0,0,0,0}},
      {"TEMPGPU0",     {0,0,0,0}},
      {"TEMPGPU0MEM",  {0,0,0,0}},
      {"TEMPGPU1",     {0,0,0,0}},
      {"TEMPGPU1MEM",  {0,0,0,0}},
      {"TEMPGPU2",     {0,0,0,0}},
      {"TEMPGPU2MEM",  {0,0,0,0}},
      //{"TEMPNEST",   {0,0,0,0}},
      //{"TEMPPROCTHRMC00", {0,0,0,0}},
      //{"TEMPPROCTHRMC01", {0,0,0,0}},
      //{"TEMPPROCTHRMC02", {0,0,0,0}},
      //{"TEMPPROCTHRMC03", {0,0,0,0}},
      //{"TEMPPROCTHRMC04", {0,0,0,0}},
      //{"TEMPPROCTHRMC05", {0,0,0,0}},
      //{"TEMPPROCTHRMC06", {0,0,0,0}},
      //{"TEMPPROCTHRMC07", {0,0,0,0}},
      //{"TEMPPROCTHRMC08", {0,0,0,0}},
      //{"TEMPPROCTHRMC09", {0,0,0,0}},
      //{"TEMPPROCTHRMC10", {0,0,0,0}},
      //{"TEMPPROCTHRMC11", {0,0,0,0}},
      //{"TEMPPROCTHRMC12", {0,0,0,0}},
      //{"TEMPPROCTHRMC13", {0,0,0,0}},
      //{"TEMPPROCTHRMC14", {0,0,0,0}},
      //{"TEMPPROCTHRMC15", {0,0,0,0}},
      //{"TEMPPROCTHRMC16", {0,0,0,0}},
      //{"TEMPPROCTHRMC17", {0,0,0,0}},
      //{"TEMPPROCTHRMC18", {0,0,0,0}},
      //{"TEMPPROCTHRMC19", {0,0,0,0}},
      //{"TEMPPROCTHRMC20", {0,0,0,0}},
      //{"TEMPPROCTHRMC21", {0,0,0,0}},
      //{"TEMPPROCTHRMC22", {0,0,0,0}},
      //{"TEMPPROCTHRMC23", {0,0,0,0}},
      //{"TEMPVDD", {0,0,0,0}}
      
      // Sensors not in use
//    {"TEMPDIMM00", {0,0,0,0}},
//    {"TEMPDIMM01", {0,0,0,0}},
//    {"TEMPDIMM06", {0,0,0,0}},
//    {"TEMPDIMM07", {0,0,0,0}},
//    {"TEMPDIMM08", {0,0,0,0}},
//    {"TEMPDIMM09", {0,0,0,0}},
//    {"TEMPDIMM14", {0,0,0,0}},
//    {"TEMPDIMM15", {0,0,0,0}},
   };

   // Query and check for success.
   std::vector<std::unordered_map<std::string, csm::daemon::helper::CsmOCCSensorRecord>> current_values;
   
   bool success = csm::daemon::helper::GetExtendedOCCSensorData(request_map, current_values);
   boost::property_tree::ptree env_pt;

   if (success)
   {
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

         // Dimm data 
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

   // Processor socket level data
   const int MAX_CHIP(2);
   
   for (int chip = 0; chip < MAX_CHIP; chip++)
   {
      boost::property_tree::ptree chip_pt;
      
      chip_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_PROCESSOR_ENV);
      chip_pt.put( "data.processor_id", std::to_string(chip) );
      chip_pt.put( "data.serial_number", "ABC123" );
      
      chip_pt.put( "data.processor_energy", "44444" );
      chip_pt.put( "data.processor_temp", "23" );
      chip_pt.put( "data.processor_temp_min", "20" );
      chip_pt.put( "data.processor_temp_max", "27" );
      //chip_pt.put( "data.gpu_energy", "22222" );
      //chip_pt.put( "data.mem_energy", "11111" );
   
      _data_list.push_back(chip_pt);
   }   
   
   // GPU level data
   const int MAX_GPU(6);
   
   for (int gpu = 0; gpu < MAX_GPU; gpu++)
   {
      boost::property_tree::ptree gpu_pt;
      
      gpu_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_GPU_ENV);
      gpu_pt.put( "data.gpu_id", std::to_string(gpu) );
      gpu_pt.put( "data.serial_number", "ABC123" );
         
      gpu_pt.put( "data.gpu_temp", "23" );
      gpu_pt.put( "data.gpu_temp_min", "20" );
      gpu_pt.put( "data.gpu_temp_max", "27" );
      gpu_pt.put( "data.gpu_mem_temp", "23" );
      gpu_pt.put( "data.gpu_mem_temp_min", "20" );
      gpu_pt.put( "data.gpu_mem_temp_max", "27" );
      
      _data_list.push_back(gpu_pt);
   } 
   
   LOG(csmenv, debug) << "Finish Collect_Environmental_Data()";

   return success;
}

CSM_Environmental_Data& CSM_Environmental_Data::operator=( const CSM_Environmental_Data& in )
{
  _Data_Mask = in._Data_Mask;
  _source_node = in._source_node;
  _timestamp = in._timestamp;
  _GPU_Double_Data = in._GPU_Double_Data;
  _GPU_Long_Data = in._GPU_Long_Data;
  _GPU_Double_Label_Data = in._GPU_Double_Label_Data;
  _GPU_Long_Label_Data = in._GPU_Long_Label_Data;
  _CPU_Data = in._CPU_Data;
  _data_list = in._data_list;
  return *this;
}

// operator to only update the items that are present in the input
CSM_Environmental_Data& CSM_Environmental_Data::operator|=( const CSM_Environmental_Data& in )
{
  _Data_Mask |= in._Data_Mask;
  
  if( !in._source_node.empty() )
    _source_node = in._source_node;
  
  if( !in._timestamp.empty() )
    _timestamp = in._timestamp;

  if( in._Data_Mask.test( GPU_DOUBLE_DATA_BIT ) )
    _GPU_Double_Data = in._GPU_Double_Data;

  if( in._Data_Mask.test( GPU_LONG_DATA_BIT ) )
    _GPU_Long_Data = in._GPU_Long_Data;

  if( in._Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) )
    _GPU_Double_Label_Data = in._GPU_Double_Label_Data;

  if( in._Data_Mask.test( GPU_LONG_LABEL_BIT ) )
    _GPU_Long_Label_Data = in._GPU_Long_Label_Data;

  if( in._Data_Mask.test( CPU_DATA_BIT ) )
    _CPU_Data = in._CPU_Data;
  
  if( !in._data_list.empty() )
    _data_list = in._data_list;

  return *this;
}

bool CSM_Environmental_Data::HasData() const
{
  return _Data_Mask.any();
}
