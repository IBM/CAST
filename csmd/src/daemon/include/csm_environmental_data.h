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

public:

  CSM_Environmental_Data();
  
  ~CSM_Environmental_Data();

  void Print();

  std::string GetJsonString();

  // Collects the common node level data and sets it in the object
  void CollectNodeData();

  // Collects the environmental temperature and power data and sets it in the object
  bool CollectEnvironmentalData();

  void AddDataItems(const std::list<boost::property_tree::ptree> &data_pt_list);

  void AddDataItem(const boost::property_tree::ptree &data_pt);

  bool HasData() const;

private:
   friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &archive, const unsigned int version)
  {
     archive & _version;

     // serialize the _archive_mask as a uint32_t
     uint32_t bitmask = _archive_mask.to_ulong();
     archive & bitmask;
     _archive_mask = std::bitset<MAX_ARCHIVE_BIT>(bitmask);

     // check and archive the content
     if ( _archive_mask.test(SOURCE_NODE_BIT) )
        archive & _source_node;

     if ( _archive_mask.test(TIMESTAMP_BIT) )
        archive & _timestamp;

     if ( _archive_mask.test(DATA_LIST_BIT) )
        archive & _data_list;
  }

private:
  
  // Used for controlling serialization and compatibility between daemon versions 
  enum CsmEnvironmentalDataVersion : int8_t
  {
    CSM_ENVIRONMENTAL_DATA_V1 = 1
  };
 
  enum ArchiveBits
  {
    SOURCE_NODE_BIT,
    TIMESTAMP_BIT,
    DATA_LIST_BIT,
    MAX_ARCHIVE_BIT = 32
  };

  // Private data members
  
  // Used for message serialization only
  CsmEnvironmentalDataVersion _version;
  std::bitset<MAX_ARCHIVE_BIT> _archive_mask;

  // Node level data common to all elements 
  std::string _source_node;
  std::string _timestamp;

  // List of property trees containing the collected data from the different configured buckets
  std::list<boost::property_tree::ptree> _data_list;
};

#endif
