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

  void Print();

  std::string GetJsonString();

  // Collects the common node level data and sets it in the object
  void CollectNodeData();

  // Collects the environmental temperature and power data and sets it in the object
  bool CollectEnvironmentalData();

  void AddDataItems(const std::list<boost::property_tree::ptree> &data_pt_list);

  void AddDataItem(const boost::property_tree::ptree &data_pt);

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

#ifdef REMOVED
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
#endif

     archive & _data_list;
  }

 private:

  std::bitset<MAX_DATA_BIT> _Data_Mask;

  // Node level data common to all elements 
  std::string _source_node;
  std::string _timestamp;

  // List of property trees containing the collected data from the different configured buckets
  std::list<boost::property_tree::ptree> _data_list;
};

#endif
