/*================================================================================

    csmd/src/daemon/include/csm_environmental_data.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    /U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_ENVIRONMENTAL_DATA_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_ENVIRONMENTAL_DATA_H_

#include <bitset>
#include <string>
#include <list>
#include <boost/property_tree/ptree.hpp>
#include <boost/serialization/list.hpp>
#include <boost/property_tree/ptree_serialization.hpp>

#include "csm_CPU_data.h"
#include "csm_GPU_double_data.h"
#include "csm_GPU_long_data.h"
#include "csm_GPU_double_labels.h"
#include "csm_GPU_long_labels.h"

class CSM_Environmental_Data
{

  typedef enum
  {

    CPU_DATA_BIT,
    CPU_LABEL_BIT,

    GPU_DOUBLE_DATA_BIT,
    GPU_LONG_DATA_BIT,

    GPU_DOUBLE_LABEL_BIT,
    GPU_LONG_LABEL_BIT,

    SSD_DATA_BIT,
    SSD_LABEL_BIT,

    MAX_DATA_BIT

  } BitDefinitions;

public:

  CSM_Environmental_Data();
  
  CSM_Environmental_Data( const CSM_Environmental_Data& in );

  ~CSM_Environmental_Data();

  void Get_GPU_Double_DCGM_Field_Values_And_Set_Bit();
  void Get_GPU_Long_DCGM_Field_Values_And_Set_Bit();

  void Get_GPU_Double_DCGM_Field_String_Identifiers_And_Set_Bit();
  void Get_GPU_Long_DCGM_Field_String_Identifiers_And_Set_Bit();

  void Print_GPU_Double_DCGM_Field_Values();
  void Print_GPU_Long_DCGM_Field_Values();

  void Print_GPU_Double_DCGM_Field_String_Identifiers();
  void Print_GPU_Long_DCGM_Field_String_Identifiers();

  void Print();

  CSM_CPU_Data& Return_CPU_Data_Object();

  CSM_GPU_Double_Data& Return_GPU_Double_Data_Object();
  CSM_GPU_Long_Data& Return_GPU_Long_Data_Object();
  CSM_GPU_Double_Label_Data& Return_GPU_Double_Label_Data_Object();
  CSM_GPU_Long_Label_Data& Return_GPU_Long_Label_Data_Object();

  void Set_Data( const CSM_GPU_Double_Data& GPU_Double_Data_To_Copy );

  void Set_Data( const CSM_GPU_Long_Data& GPU_Long_Data_To_Copy );

  void Set_Data( const CSM_GPU_Double_Label_Data& GPU_Double_Label_Data_To_Copy );

  void Set_Data( const CSM_GPU_Long_Label_Data& GPU_Long_Label_Data_To_Copy );

  void Set_Data( const CSM_CPU_Data& CPU_data_to_copy );

  bool Set_Labels( const CSM_Environmental_Data& in );

  std::string Get_Json_String();

  // Collects the common node level data and sets it in the object
  void Collect_Node_Data();

  // Collects the environmental temperature and power data and sets it in the object
  bool Collect_Environmental_Data();

  CSM_Environmental_Data& operator=( const CSM_Environmental_Data& in );

  // operator to only update the items that are present in the input
  CSM_Environmental_Data& operator|=( const CSM_Environmental_Data& in );

  bool HasData() const;

private:
   friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &archive, const unsigned int version)
  {
     // serialize the bitset as a sting
     std::string dmString = _Data_Mask.to_string();
     archive & dmString;

     archive & _source_node;
     archive & _timestamp;

     // update from the string for the deserialization path
     _Data_Mask = std::bitset<MAX_DATA_BIT>( dmString );

     // check and archive the content
     if( _Data_Mask.test( GPU_DOUBLE_DATA_BIT ) )
        archive & _GPU_Double_Data;

     if( _Data_Mask.test( GPU_LONG_DATA_BIT ) )
        archive & _GPU_Long_Data;

     if( _Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) )
        archive & _GPU_Double_Label_Data;

     if( _Data_Mask.test( GPU_LONG_LABEL_BIT ) )
        archive & _GPU_Long_Label_Data;

     if( _Data_Mask.test( CPU_DATA_BIT ) )
        archive & _CPU_Data;

     archive & _data_list;
  }

 private:

  std::bitset<MAX_DATA_BIT> _Data_Mask;

  // Node level data common to all elements 
  std::string _source_node;
  std::string _timestamp;

  CSM_CPU_Data _CPU_Data;

  CSM_GPU_Double_Data _GPU_Double_Data;
  CSM_GPU_Long_Data _GPU_Long_Data;
  
  CSM_GPU_Double_Label_Data _GPU_Double_Label_Data;
  CSM_GPU_Long_Label_Data _GPU_Long_Label_Data;

  // List of property trees containing the collected data from the different configured buckets
  std::list<boost::property_tree::ptree> _data_list;
};

#endif
