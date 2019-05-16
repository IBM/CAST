/*================================================================================

    csmd/src/inv/ib_and_switch/include/inv_switch_connector_access.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#ifndef CSMD_SRC_INV_INCLUDE_INV_SWITCH_CONNECTOR_ACCESS_H_
#define CSMD_SRC_INV_INCLUDE_INV_SWITCH_CONNECTOR_ACCESS_H_

/////////////////////////////////////////////////////////////////////////////////
// SWITCH_CONNECTOR support is enabled
/////////////////////////////////////////////////////////////////////////////////

#ifdef SWITCH_CONNECTOR
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <sstream>
#include <vector>

//#include "../../include/inv_ib_guid.h"

#include <boost/asio.hpp>

class INV_SWITCH_CONNECTOR_ACCESS
{
public:

  // functions

  static INV_SWITCH_CONNECTOR_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_SWITCH_CONNECTOR_ACCESS(); } return _Instance; } // get the istance of the class object
  int GetCompiledWithSupport(); // get compiled_with_support_flag
  int ExecuteDataCollection(std::string rest_address, std::string authentication_string_for_the_http_request, std::string csm_inv_log_dir, std::string switch_errors, bool custom_input_override, std::string ufm_switch_output_file_name, std::string ufm_switch_input_file_name); // execute data collection
  std::string ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector); // return the value of the field
  std::string ReturnFieldValue_module(std::string key, unsigned long int index); // return the value of the field
  int TotalNumberOfRecords();
  ~INV_SWITCH_CONNECTOR_ACCESS();

private:
  INV_SWITCH_CONNECTOR_ACCESS();
  int extractValueFromLine(std::string line, char** value, char* key, bool finalLine = false);
  int module_key_value_vector_builder(char* module_key, char* module_value);
  
private:

  // pointer to the class object
  static INV_SWITCH_CONNECTOR_ACCESS *_Instance;

  // other variables
  int compiled_with_support;
  //std::size_t position_delimiter;
  std::string line;
  std::string comparing_string;
  unsigned long int number_of_field_vectors;

  // vectors of the fields
  std::vector<std::string> vector_of_the_comparing_strings;
  std::vector<std::string> vector_of_the_switch_names;
  std::vector<std::string> vector_of_the_descriptions;
  std::vector<std::string> vector_of_the_firmware_versions;
  std::vector<std::string> vector_of_the_guids;
  std::vector<std::string> vector_of_the_has_ufm_agents;
  std::vector<std::string> vector_of_the_ips;
  std::vector<std::string> vector_of_the_model;
  std::vector<std::string> vector_of_the_num_modules;
  std::vector<std::string> vector_of_the_num_ports;
  std::vector<std::string> vector_of_the_physical_frame_locations;
  std::vector<std::string> vector_of_the_physical_u_locations;
  std::vector<std::string> vector_of_the_ps_ids;
  std::vector<std::string> vector_of_the_roles;
  std::vector<std::string> vector_of_the_server_operation_modes;
  std::vector<std::string> vector_of_the_sm_modes;
  std::vector<std::string> vector_of_the_states;
  std::vector<std::string> vector_of_the_sw_versions;
  std::vector<std::string> vector_of_the_system_guids;
  std::vector<std::string> vector_of_the_system_names;
  std::vector<std::string> vector_of_the_total_alarms;
  std::vector<std::string> vector_of_the_types;
  std::vector<std::string> vector_of_the_vendors;
  std::vector<std::string> vector_of_the_serial_numbers;
  std::vector<std::string> vector_of_the_os_versions;
  std::vector<std::string> vector_of_the_modules;
  std::vector<std::string> vector_of_the_comparing_strings_modules;
  std::vector<std::string> vector_of_the_num_values;

  //vectors of modules
  std::vector<std::string> module_status;
  std::vector<std::string> module_hw_version;
  std::vector<std::string> module_name;
  std::vector<std::string> module_number_of_chips;
  std::vector<std::string> module_description;
  std::vector<std::string> module_max_ib_ports;
  std::vector<std::string> module_module_index;
  std::vector<std::string> module_device_type;
  std::vector<std::string> module_serial_number;
  std::vector<std::string> module_path;
  std::vector<std::string> module_device_name;
  std::vector<std::string> module_severity;
  

};

#endif // SWITCH support is enabled

/////////////////////////////////////////////////////////////////////////////////
// SWITCH_CONNECTOR support is disabled
/////////////////////////////////////////////////////////////////////////////////

#ifndef SWITCH_CONNECTOR

#include <vector>
#include <string>

class INV_SWITCH_CONNECTOR_ACCESS
{
public:

  // functions

  static INV_SWITCH_CONNECTOR_ACCESS* GetInstance(){ if( _Instance == nullptr ){ _Instance = new INV_SWITCH_CONNECTOR_ACCESS(); } return _Instance; } // get the istance of the class object
  int GetCompiledWithSupport(); // get compiled_with_support
  int ExecuteDataCollection(); // execute data collection
  std::string ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector); // return the value of the field
  std::string ReturnFieldValue_module(std::string key, unsigned long int index); // return the value of the field
  int TotalNumberOfRecords();
  ~INV_SWITCH_CONNECTOR_ACCESS();

private:
  INV_SWITCH_CONNECTOR_ACCESS();
  int extractValueFromLine(std::string line, char** value, char* key, bool finalLine = false);
  int module_key_value_vector_builder(char* module_key, char* module_value);

private:

  // pointer to the class object
  static INV_SWITCH_CONNECTOR_ACCESS *_Instance;

  // other variables
  int compiled_with_support;
  std::size_t position_delimiter;
  std::string line;
  std::string comparing_string;
  unsigned long int number_of_field_vectors;

  // vectors of the fields
  std::vector<std::string> vector_of_the_comparing_strings;
  std::vector<std::string> vector_of_the_switch_names;
  std::vector<std::string> vector_of_the_descriptions;
  std::vector<std::string> vector_of_the_firmware_versions;
  std::vector<std::string> vector_of_the_guids;
  std::vector<std::string> vector_of_the_has_ufm_agents;
  std::vector<std::string> vector_of_the_ips;
  std::vector<std::string> vector_of_the_model;
  std::vector<std::string> vector_of_the_num_modules;
  std::vector<std::string> vector_of_the_num_ports;
  std::vector<std::string> vector_of_the_physical_frame_locations;
  std::vector<std::string> vector_of_the_physical_u_locations;
  std::vector<std::string> vector_of_the_ps_ids;
  std::vector<std::string> vector_of_the_roles;
  std::vector<std::string> vector_of_the_server_operation_modes;
  std::vector<std::string> vector_of_the_sm_modes;
  std::vector<std::string> vector_of_the_states;
  std::vector<std::string> vector_of_the_sw_versions;
  std::vector<std::string> vector_of_the_system_guids;
  std::vector<std::string> vector_of_the_system_names;
  std::vector<std::string> vector_of_the_total_alarms;
  std::vector<std::string> vector_of_the_types;
  std::vector<std::string> vector_of_the_vendors;
  std::vector<std::string> vector_of_the_serial_numbers;
  std::vector<std::string> vector_of_the_os_versions;
  std::vector<std::string> vector_of_the_modules;
  std::vector<std::string> vector_of_the_comparing_strings_modules;
  std::vector<std::string> vector_of_the_num_values;

  //vectors of modules
  std::vector<std::string> module_status;
  std::vector<std::string> module_hw_version;
  std::vector<std::string> module_name;
  std::vector<std::string> module_number_of_chips;
  std::vector<std::string> module_description;
  std::vector<std::string> module_max_ib_ports;
  std::vector<std::string> module_module_index;
  std::vector<std::string> module_device_type;
  std::vector<std::string> module_serial_number;
  std::vector<std::string> module_path;
  std::vector<std::string> module_device_name;
  std::vector<std::string> module_severity;

};

#endif // SWITCH_CONNECTOR support is disabled

#endif /* CSMD_SRC_INV_INCLUDE_INV_SWITCH_CONNECTOR_ACCESS_H_ */

