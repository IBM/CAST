/*================================================================================

    csmd/src/inv/ib_and_switch/include/inv_ib_connector_access.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#ifndef CSMD_SRC_INV_INCLUDE_INV_IB_CONNECTOR_ACCESS_H_
#define CSMD_SRC_INV_INCLUDE_INV_IB_CONNECTOR_ACCESS_H_

/////////////////////////////////////////////////////////////////////////////////
// IB_CONNECTOR support is enabled
/////////////////////////////////////////////////////////////////////////////////

#ifdef IB_CONNECTOR
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <sstream>
#include <time.h>
#include <vector>

//#include "../../include/inv_ib_guid.h"

#include <boost/asio.hpp>

class INV_IB_CONNECTOR_ACCESS
{
public:

  // functions

  static INV_IB_CONNECTOR_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_IB_CONNECTOR_ACCESS(); } return _Instance; } // get the istance of the class object
  int GetCompiledWithSupport(); // get compiled_with_support_flag
  int ExecuteDataCollection(std::string rest_address, std::string authentication_string_for_the_http_request); // execute data collection
  std::string ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector); // return the value of the field
  int TotalNumberOfRecords(); 
  ~INV_IB_CONNECTOR_ACCESS();

private:
  INV_IB_CONNECTOR_ACCESS();

private:

  // pointer to the class object
  static INV_IB_CONNECTOR_ACCESS *_Instance;

  // other variables
  int compiled_with_support;
  //std::size_t position_delimiter;
  std::string line;
  std::string comparing_string;
  unsigned long int number_of_field_vectors;

  // vectors of the fields
  std::vector<std::string> vector_of_the_comparing_strings;   
  std::vector<std::string> vector_of_the_part_numbers;
  std::vector<std::string> vector_of_the_lengths;
  std::vector<std::string> vector_of_the_serial_numbers;
  std::vector<std::string> vector_of_the_identifiers;
  std::vector<std::string> vector_of_the_technologies;
  std::vector<std::string> vector_of_the_revisions;
  std::vector<std::string> vector_of_the_severities;
  std::vector<std::string> vector_of_the_source_guids;
  std::vector<std::string> vector_of_the_widths;
  std::vector<std::string> vector_of_the_source_ports;
  std::vector<std::string> vector_of_the_destination_ports;
  std::vector<std::string> vector_of_the_destination_guids;
  std::vector<std::string> vector_of_the_names;
};

#endif // IB_CONNECTOR support is enabled

/////////////////////////////////////////////////////////////////////////////////
// IB_CONNECTOR support is disabled
/////////////////////////////////////////////////////////////////////////////////

#ifndef IB_CONNECTOR

#include <vector>
#include <string>

class INV_IB_CONNECTOR_ACCESS
{
public:

  // functions

  static INV_IB_CONNECTOR_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_IB_CONNECTOR_ACCESS(); } return _Instance; } // get the istance of the class object
  int GetCompiledWithSupport(); // get compiled_with_support
  int ExecuteDataCollection(std::string rest_address, std::string authentication_string_for_the_http_request); // execute data collection
  std::string ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector); // return the value of the field
  int TotalNumberOfRecords();
  ~INV_IB_CONNECTOR_ACCESS();

private:
  INV_IB_CONNECTOR_ACCESS();

private:

  // pointer to the class object
  static INV_IB_CONNECTOR_ACCESS *_Instance;

  // other variables
  int compiled_with_support;
  std::size_t position_delimiter;
  std::string line;
  std::string comparing_string;
  unsigned long int number_of_field_vectors;

  // vectors of the fields
  std::vector<std::string> vector_of_the_comparing_strings;
  std::vector<std::string> vector_of_the_part_numbers;
  std::vector<std::string> vector_of_the_lengths;
  std::vector<std::string> vector_of_the_serial_numbers;
  std::vector<std::string> vector_of_the_identifiers;
  std::vector<std::string> vector_of_the_technologies;
  std::vector<std::string> vector_of_the_revisions;
  std::vector<std::string> vector_of_the_severities;
  std::vector<std::string> vector_of_the_source_guids;
  std::vector<std::string> vector_of_the_widths;
  std::vector<std::string> vector_of_the_source_ports;
  std::vector<std::string> vector_of_the_destination_ports;
  std::vector<std::string> vector_of_the_destination_guids;
  std::vector<std::string> vector_of_the_names;

};

#endif // IB_CONNECTOR support is disabled

#endif /* CSMD_SRC_INV_INCLUDE_INV_IB_CONNECTOR_ACCESS_H_ */

