/*================================================================================
   
    csmd/src/inv/include/inv_ib_guid.h

  © Copyright IBM Corporation 2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

// This file contains standard functions for converting the different formats
// of IB GUID from one format to another.

// The function standardizeGuid() should be used whenever a GUID is read from an 
// external source to convert it to the standardized format within CSM.

#ifndef __INV_IB_GUID_H
#define __INV_IB_GUID_H

#include <string>
#include <stdint.h>

// This function should be called on any external IB GUID string to use a standardized format internally.
// Expected input formats for _guid:
// Minimum GUID:    ec0d9a0300499a4a
// Prefix GUID:   0xec0d9a0300499a4a
// Colon GUID:   ec0d:9a03:0049:9a4a 
//
// Standardized output format:
// ec0d:9a03:0049:9a4a 
// 
// If the passed string appears to be an invalid guid,
// the input string will be returned unmodified
inline std::string standardizeGuid(const std::string &_guid);

// Removes any "0x" prefix and any internal ":" chars
inline std::string stripGuid(const std::string &i_guid)
{
  std::string guid("");
  uint32_t i(0);

  if ((i_guid[0] == '0') && (i_guid[1] == 'x'))      // Skip over the "0x" prefix
  {
     i = 2;
  }

  for ( ; i < i_guid.size(); i++)
  {
    if (i_guid[i] != ':')
    {
      guid.push_back(i_guid[i]);
    } 
  }

  return(guid);
}

// Strips i_guid and returns a string like this:
// ec0d9a0300499a4a
inline std::string getMinimumGuid(const std::string &i_guid)
{
  std::string guid(stripGuid(i_guid));

  if (guid.size() == 16)
  {
    return guid;
  }
  else
  {
    return i_guid;
  }
}  

// Strips i_guid and returns a string like this:
// 0xec0d9a0300499a4a
inline std::string getPrefixGuid(const std::string &i_guid)
{
  std::string guid(stripGuid(i_guid));

  if (guid.size() == 16)
  {
    return "0x" + guid;
  }
  else
  {
    return i_guid;
  }
}  

// Strips i_guid and returns a string like this:
// ec0d:9a03:0049:9a4a
inline std::string getColonGuid(const std::string &i_guid)
{
  std::string guid(stripGuid(i_guid));

  if (guid.size() == 16)
  {
    guid.insert(12,":");
    guid.insert(8,":");
    guid.insert(4,":");
    return guid;
  }
  else
  {
    return i_guid;
  }
}  

inline std::string standardizeGuid(const std::string &i_guid)
{
  return getColonGuid(i_guid);
}  

#endif
