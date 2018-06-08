/*================================================================================

    csmd/src/daemon/src/csm_environmental_data.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    /U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "include/csm_environmental_data.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>

#include <sys/time.h>

CSM_Environmental_Data::CSM_Environmental_Data()
{
  _Data_Mask.reset();
}
  
CSM_Environmental_Data::CSM_Environmental_Data( const CSM_Environmental_Data& in ) : 
  _Data_Mask( in._Data_Mask ),
  _CPU_Data( in._CPU_Data ),
  _GPU_Double_Data( in._GPU_Double_Data ),
  _GPU_Long_Data( in._GPU_Long_Data ),
  _GPU_Double_Label_Data( in._GPU_Double_Label_Data ),
  _GPU_Long_Label_Data( in._GPU_Long_Label_Data )
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

std::string CSM_Environmental_Data::Get_Json_String()
{
  std::string json("");

  // This function will return a series of json documents in a single string
  // Each json document has a set of common parent fields followed by fields specific to 
  // the type of environmental data being collected
  // Build the common fields first, to be used in each of the individual json documents
  #define CSM_BDS_KEY_TYPE "type"
  #define CSM_BDS_TYPE_ENV_GPU "csm-env-gpu"
  
  #define CSM_BDS_KEY_TIME_STAMP "timestamp"
  
  #define CSM_ENV_DATA_GPU_PREFIX "data"
  #define CSM_ENV_DATA_KEY_NODE "node"

  // Generate a time_stamp
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
  
  std::string node("testnode01"); 
  
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
        gpu_pt.put(CSM_BDS_KEY_TYPE, CSM_BDS_TYPE_ENV_GPU);
        gpu_pt.put(CSM_BDS_KEY_TIME_STAMP, time_stamp_with_usec);   
        
        gpu_pt.put(CSM_ENV_DATA_GPU_PREFIX "." CSM_ENV_DATA_KEY_NODE, node);   

        for ( uint32_t i = 0; i < gpu_double_labels.size() && j < gpu_double_data.size(); i++ )
        {
          gpu_pt.put( CSM_ENV_DATA_GPU_PREFIX "." + gpu_double_labels[i], std::to_string(gpu_double_data[j]) );
          j++;
        }       
        
        for ( uint32_t x = 0; x < gpu_long_labels.size() && y < gpu_long_data.size(); x++ )
        {
          gpu_pt.put( CSM_ENV_DATA_GPU_PREFIX "." + gpu_long_labels[x], std::to_string(gpu_long_data[y]) );
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
  
  //LOG( csmenv, debug ) << json;
 
  return json;
}

CSM_Environmental_Data& CSM_Environmental_Data::operator=( const CSM_Environmental_Data& in )
{
  _Data_Mask = in._Data_Mask;
  _GPU_Double_Data = in._GPU_Double_Data;
  _GPU_Long_Data = in._GPU_Long_Data;
  _GPU_Double_Label_Data = in._GPU_Double_Label_Data;
  _GPU_Long_Label_Data = in._GPU_Long_Label_Data;
  _CPU_Data = in._CPU_Data;
  return *this;
}

// operator to only update the items that are present in the input
CSM_Environmental_Data& CSM_Environmental_Data::operator|=( const CSM_Environmental_Data& in )
{
  _Data_Mask |= in._Data_Mask;

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

  return *this;
}

bool CSM_Environmental_Data::HasData() const
{
  return _Data_Mask.any();
}
