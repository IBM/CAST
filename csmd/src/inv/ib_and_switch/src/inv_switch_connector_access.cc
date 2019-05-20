/*================================================================================

    csmd/src/inv/ib_and_switch/src/inv_switch_connector_access.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "../include/inv_switch_connector_access.h"
#include "logging.h"

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

INV_SWITCH_CONNECTOR_ACCESS *INV_SWITCH_CONNECTOR_ACCESS::_Instance = nullptr;

/////////////////////////////////////////////////////////////////////////////////
// SWITCH_CONNECTOR support is enabled
/////////////////////////////////////////////////////////////////////////////////

#ifdef SWITCH_CONNECTOR

INV_SWITCH_CONNECTOR_ACCESS::INV_SWITCH_CONNECTOR_ACCESS()
{
	// setting variables
	compiled_with_support = 1;
	number_of_field_vectors = 25;
	
	// setting vector of the comparing strings
	vector_of_the_comparing_strings.push_back("\"name\"");                  // 0
	vector_of_the_comparing_strings.push_back("\"description\"");           // 1 
	vector_of_the_comparing_strings.push_back("\"fw_version\"");            // 2 
	vector_of_the_comparing_strings.push_back("\"guid\"");                  // 3 
	vector_of_the_comparing_strings.push_back("\"has_ufm_agent\"");         // 4 
	vector_of_the_comparing_strings.push_back("\"ip\"");                    // 5 
	vector_of_the_comparing_strings.push_back("\"model\"");                 // 6 
	vector_of_the_comparing_strings.push_back("\"num_modules\"");           // 7
	vector_of_the_comparing_strings.push_back("\"num_ports\"");             // 8
	vector_of_the_comparing_strings.push_back("\"physical_frame\"");        // 9
	vector_of_the_comparing_strings.push_back("\"u_frame\"");               // 10
	vector_of_the_comparing_strings.push_back("\"psid\"");                  // 11
	vector_of_the_comparing_strings.push_back("\"role\"");                  // 12
	vector_of_the_comparing_strings.push_back("\"server_operation_mode\""); // 13
	vector_of_the_comparing_strings.push_back("\"sm_mode\"");               // 14
	vector_of_the_comparing_strings.push_back("\"state\"");                 // 15
	vector_of_the_comparing_strings.push_back("\"sw_version\"");            // 16
	vector_of_the_comparing_strings.push_back("\"system_guid\"");           // 17
	vector_of_the_comparing_strings.push_back("\"system_name\"");           // 18
	vector_of_the_comparing_strings.push_back("\"total_alarms\"");          // 19
	vector_of_the_comparing_strings.push_back("\"type\"");                  // 20
	vector_of_the_comparing_strings.push_back("\"vendor\"");                // 21
	vector_of_the_comparing_strings.push_back("\"serial_number\"");         // 22
	vector_of_the_comparing_strings.push_back("\"os_version\"");            // 23
	vector_of_the_comparing_strings.push_back("\"modules\"");               // 24

	// setting vector of the comparing strings modules
	vector_of_the_comparing_strings_modules.push_back("\"status\"");              
	vector_of_the_comparing_strings_modules.push_back("\"hw_version\"");          
	vector_of_the_comparing_strings_modules.push_back("\"name\"");                
	//vector_of_the_comparing_strings_modules.push_back("\"hosting_system_guid\""); 
	vector_of_the_comparing_strings_modules.push_back("\"number_of_chips\"");     
	vector_of_the_comparing_strings_modules.push_back("\"description\"");         
	vector_of_the_comparing_strings_modules.push_back("\"max_ib_ports\"");        
	//vector_of_the_comparing_strings_modules.push_back("\"fw_version\""); 
	vector_of_the_comparing_strings_modules.push_back("\"module_index\""); 
	//vector_of_the_comparing_strings_modules.push_back("\"temperature\""); 
	vector_of_the_comparing_strings_modules.push_back("\"device_type\""); 
	vector_of_the_comparing_strings_modules.push_back("\"serial_number\""); 
	vector_of_the_comparing_strings_modules.push_back("\"path\""); 
	vector_of_the_comparing_strings_modules.push_back("\"device_name\""); 
	//vector_of_the_comparing_strings_modules.push_back("\"type\""); 
	vector_of_the_comparing_strings_modules.push_back("\"severity\""); 

	vector_of_the_num_values.push_back("\"number_of_chips\"");  
	vector_of_the_num_values.push_back("\"max_ib_ports\""); 
	vector_of_the_num_values.push_back("\"module_index\"");
}

INV_SWITCH_CONNECTOR_ACCESS::~INV_SWITCH_CONNECTOR_ACCESS()
{
	// clearing vectors
	vector_of_the_comparing_strings.clear();
	vector_of_the_switch_names.clear();
	vector_of_the_descriptions.clear();
	vector_of_the_firmware_versions.clear();
	vector_of_the_guids.clear();
	vector_of_the_has_ufm_agents.clear();
	vector_of_the_ips.clear();
	vector_of_the_model.clear();
	vector_of_the_num_modules.clear();
	vector_of_the_num_ports.clear();
	vector_of_the_physical_frame_locations.clear();
	vector_of_the_physical_u_locations.clear();
	vector_of_the_ps_ids.clear();
	vector_of_the_roles.clear();
	vector_of_the_server_operation_modes.clear();
	vector_of_the_sm_modes.clear();
	vector_of_the_states.clear();
	vector_of_the_sw_versions.clear();
	vector_of_the_system_guids.clear();
	vector_of_the_system_names.clear();
	vector_of_the_total_alarms.clear();
	vector_of_the_types.clear();
	vector_of_the_vendors.clear();
	vector_of_the_serial_numbers.clear();
	vector_of_the_os_versions.clear();
	vector_of_the_modules.clear();
	vector_of_the_comparing_strings_modules.clear();
	vector_of_the_num_values.clear();
}

int INV_SWITCH_CONNECTOR_ACCESS::GetCompiledWithSupport()
{
	return compiled_with_support;
}

int INV_SWITCH_CONNECTOR_ACCESS::extractValueFromLine(std::string line, char** value, char* key, bool finalLine)
{
	//Function Variables
	bool num_key_found = false; // Used to store if we found a num key. 
	int offset = 0; // Used for trimming of the key values.
	std::size_t position_delimiter = 0; // Used to hold the position of the ":" delimiter between the key, value pair. 

	//Is this a num key?
	for(unsigned int i = 0; i < vector_of_the_num_values.size(); i++)
	{
		// Loop through our list of num keys.
		// If we find a match, then we will need to adjust the offset later. 
		int found = 0;
		found = strcmp(key, vector_of_the_num_values[i].c_str());
		if (found == 0)
		{
			num_key_found = true;
			i = vector_of_the_num_values.size();
		}
		
	}

	// Is the key a num value?
	// We need to adjust the offset based off the type of value. 
	if(num_key_found)
	{
		// num based key
		offset = 2;
	}else{
		//String based key.
		offset = 3;
	}

	// extraction field
	position_delimiter=line.find(":");
	//Modify the prefix. Trim the opening garbage from the key_value
	line.erase(0,position_delimiter + offset);
	//Modify the suffix. Trim the ending garbage.
	if(finalLine)
	{
		line.erase(line.length() - offset + 2, line.length());
	}else{
		line.erase(line.length() - offset, line.length()-1);
	}

	//allocate space for 'value'
	*value = (char*)calloc(line.length(), sizeof(char*));
	
	//copy the trimmed value into 'value'
	line.copy(*value, line.length());


	return 0;
}

int INV_SWITCH_CONNECTOR_ACCESS::ExecuteDataCollection(std::string rest_address, std::string authentication_string_for_the_http_request, std::string csm_inv_log_dir, std::string switch_errors, bool custom_input_override, std::string ufm_switch_output_file_name, std::string ufm_switch_input_file_name)
{
	try
	{	
		// Get a list of endpoints corresponding to the server name.
		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(rest_address,"http");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
		
		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);
		
		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		
		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << "GET /ufmRest/resources/systems?type=switch&chassis=t HTTP/1.1\r\n";
		request_stream << "Authorization: Basic " << authentication_string_for_the_http_request << " \r\n";
		request_stream << "Host: " << rest_address << " \r\n";
		request_stream << "Connection: close\r\n\r\n";
		
		// Send the request.
		boost::asio::write(socket, request);

		// Read the response status line. The response streambuf will automatically
		// grow to accommodate the entire line. The growth may be limited by passing
		// a maximum size to the streambuf constructor.
		boost::asio::streambuf response;
		boost::asio::read_until(socket, response, "\r\n");
		boost::asio::streambuf::const_buffers_type buf_1 = response.data();
		std::string response_copy_1(boost::asio::buffers_begin(buf_1), boost::asio::buffers_begin(buf_1) + response.size());

		// Check that response is OK.
		std::istream response_stream(&response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		if (!response_stream || http_version.substr(0, 5) != "HTTP/")
		{
			std::cout << "Invalid response\n";
			return 1;
		}
		if (status_code != 200)
		{
			std::cout << "Response returned with status code " << status_code << "\n";
			return 1;
		}

		// Read the response headers, which are terminated by a blank line.
		boost::asio::read_until(socket, response, "\r\n\r\n");
		boost::asio::streambuf::const_buffers_type buf_2 = response.data();
		std::string response_copy_2(boost::asio::buffers_begin(buf_2), boost::asio::buffers_begin(buf_2) + response.size());

		// Process the response headers.
		std::string header;
		while (std::getline(response_stream, header) && header != "\r")
		{
			//LOG(csmd, debug) << header;
			//std::cout << header << "\n";
		}

		// Write whatever content we already have to output.
		if (response.size() > 0)
		{
			//LOG(csmd, debug) << &response;
			//std::cout << &response;
		}

        // set output name to the value passed in via parameter.
		// add in the inv directory
		std::string output_file_name = csm_inv_log_dir + "/" + ufm_switch_output_file_name;
		// opening output file
		std::ofstream output_file(output_file_name.c_str(),std::ios::out);

		// checking if output file is open
		if ( ! output_file.is_open() )
		{
			// printing error and return
			std::cout << "Output file " << output_file_name << " not open, return"  << std::endl;
			return 1;
		} 

		boost::system::error_code error;
		while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
		{
			output_file << &response;
		}

		// closing the output file
		output_file.close();

		// checking for errors
		if (error != boost::asio::error::eof)
		{
			throw boost::system::system_error(error);
		}

		// vectors with the fields
		std::size_t position_delimiter;
		std::string line;
		std::string comparing_string;

		//set input filename

		// opening input file
		std::string input_file_name = "";
		

		if(custom_input_override)
		{
			input_file_name = ufm_switch_input_file_name;
		}else{
			input_file_name = csm_inv_log_dir + "/" + ufm_switch_input_file_name;
		}


		std::ifstream input_file(input_file_name.c_str(),std::ios::in);

		// checking if input file is open
		if ( ! input_file.is_open() )
		{
			// printing error and return
			std::cout << "Input file " << input_file_name << " not open, return" << std::endl;
			return 1;
		}

		//grab time info to stick into the bad switch text file
		time_t rawtime;
		struct tm * timeinfo;
		time (&rawtime);
		timeinfo = localtime (&rawtime);
	
		//opening a error write file
		std::ofstream bad_switch_records;
		
		bad_switch_records.open (csm_inv_log_dir + "/" + switch_errors);
		bad_switch_records << "CSM switch inventory collection" << std::endl;
		bad_switch_records << "File created: " << asctime(timeinfo) << std::endl;
		bad_switch_records << "The following records are incomplete and can not be inserted into CSM database.\n" << std::endl;
		
		//helper variable to keep track of a bad serial number for a record. 
		bool bad_record = false;
		//helper variable to keep track of total records.
		int total_switch_records = 0;
		int NA_serials_count = 0;

		while (std::getline(input_file, line))
		{
			// reading the line
			std::istringstream iss(line);

			// cycling of the cpmaring strings
			for (unsigned int i = 0; i < vector_of_the_comparing_strings.size(); i++)
			{
				// reading comarping string
				comparing_string = vector_of_the_comparing_strings[i];

				// Does this line contain a key?
				if(line.find(comparing_string) == std::string::npos)
				{
					//This line does not contain a valid key. 
					continue;
				}

				if(i == 24){
					// special case: "modules"
					// modules can either be 'slim' or 'expanded'
					//can also be blank? = "[]," this could be a broken case. but we encountered so plan for it.
					// following code determines which is true, and then collects the data.
			
					int number_of_modules_found = 0;
					
					//check for the broken "[],"
					if(line.find("[],") != std::string::npos){
						//push num_modules
						//vector_of_the_num_modules.push_back(std::to_string(number_of_modules_found));
						
						//Should prob report some error here.
						
						//idk, exit early, no need to further continue the main loop.


                        //maybe increase the number of bad switches
                        //because no serial number field from module
                        NA_serials_count++;
                        bad_record = true;
					}else{
						// read the next line
						std::getline(input_file, line);
						
						// determine if 'slim' or 'expanded'
						if(line.find("{") == std::string::npos)
						{
							//did not find a "{"
							// 'slim' case
							// only a list of module names are found

							//definitly bad because this means no serial number
							bad_record = true;
							NA_serials_count++;
							
							// read list of names until end of list
							// exit condition variable
							char close_found = 0;
							do{

								//Modify the prefix. Trim the opening garbage from the key_value
								line.erase(0,13);
								
								//check suffix if ','
								if( line.find(",") == std::string::npos)
								{
									//can't find a ',' - so this is last value
									line.erase(line.length()-1,line.length());
								}else{
									//more values
									//remove the double
									line.erase(line.length()-3,line.length()-1);
								}
								
								//copy to bad_switch_records NOT vector push
								bad_switch_records << "New Module: ??? " << std::endl;
								bad_switch_records << "name:            " << line << std::endl;
								bad_switch_records << "description:     N/A "<< std::endl;
								bad_switch_records << "device_name:     N/A "<< std::endl;
								bad_switch_records << "device_type:     N/A "<< std::endl;
								bad_switch_records << "max_ib_ports:    N/A "<< std::endl;
								bad_switch_records << "module_index:    N/A "<< std::endl;
								bad_switch_records << "number_of_chips: N/A "<< std::endl;
								bad_switch_records << "path:            N/A "<< std::endl;
								bad_switch_records << "serial_number:   N/A "<< std::endl;
								bad_switch_records << "severity:        N/A "<< std::endl;
								bad_switch_records << "status:          N/A "<< std::endl;
								bad_switch_records << std::endl;
								
								//increase the count
								number_of_modules_found++;

								//grab next name
								std::getline(input_file, line);

								if(line.find("]") != std::string::npos)
								{
									// ']' found
									close_found = 1;
								}
						
							}while(close_found == 0);
					
							//push num_modules
							vector_of_the_num_modules.push_back(std::to_string(number_of_modules_found));
							
							//idk, exit early, no need to further contine the main loop.
					
						}else{
							//expanded records are found
							
							//continue reading list of modules until end of list
							//exit condition variable
							char close_found = 0;

							//For personal notes. This should be a "GOOD" record WITH a serial number
							
							do{
								if(line.find("]") == std::string::npos)
								{
									//still searching for modules

									//Create a temp value for the 'value' in our "key/value" pair.
									char* module_value = NULL;
									char* module_key = NULL;
									bool module_key_found = false;
									//for looping the module
									bool more_fields = true;

									do{
										//grab next line
										std::getline(input_file, line);
										// Is this the last module field?
										bool end_of_modules = false;

										if(line.find(",") == std::string::npos)
										{
											// at the end
											// DETAILS: We didn't find a ',' in the line, so we assume it to be the last value. 
											end_of_modules = true;
										}

										for(unsigned int i = 0; i < vector_of_the_comparing_strings_modules.size(); i++)
										{
											std::size_t found = 0;
											found = line.find(vector_of_the_comparing_strings_modules[i]);
											if (found!=std::string::npos)
											{
												module_key = (char*)calloc(vector_of_the_comparing_strings_modules[i].length(),sizeof(char));
												vector_of_the_comparing_strings_modules[i].copy(module_key, vector_of_the_comparing_strings_modules[i].length());
												module_key_found = true;
											}

											if( module_key_found)
											{
												if(end_of_modules)
												{
													INV_SWITCH_CONNECTOR_ACCESS::extractValueFromLine(line, &module_value, module_key, true);
												}else{
													INV_SWITCH_CONNECTOR_ACCESS::extractValueFromLine(line, &module_value, module_key);
												}
												
												vector_of_the_modules.push_back(module_value); 
												module_key_value_vector_builder(module_key, module_value);
												free(module_key);
												free(module_value);
												module_key_found = false;
												//exit loop early
												i = vector_of_the_comparing_strings_modules.size();
											}
										}

										if(end_of_modules)
										{
											more_fields = false;
											//grab next line
											std::getline(input_file, line); // },
											std::getline(input_file, line); // { or ],
										}
											

									}while(more_fields);

									//increase the count
									number_of_modules_found++;
								}else{
									//all modules completed
									close_found = 1;
								}
							}while(close_found == 0);
						}
						//push num_modules
						vector_of_the_num_modules.push_back(std::to_string(number_of_modules_found));
						//idk, exit early, no need to further contine the main loop.
					}
				}
		
				// extraction field
				position_delimiter=line.find(":");
					
				//Modify the prefix. Trim the opening garbage from the key_value
				if(i == 19 || i == 4 ){
					//delimiter, space
					//2 char spaces
					line.erase(0,position_delimiter+2);
				}else{
					//delimiter, space, double quote
					//3 char spaces
					line.erase(0,position_delimiter+3);
				}
		
				//Modify the suffix. Trim the ending garbage.
				if(i == 19 || i == 4 ){
					line.erase(line.length()-2,line.length());
				}else if( i == (vector_of_the_comparing_strings.size()-1) ){
					//I think Fausto meant "last line of the record" which would have no comma and therefor need less trim
					//ToDo: check later
					line.erase(line.length()-3,line.length()-1);
				}else{
					line.erase(line.length()-3,line.length()-1);					
				}

				// updating vectors
				switch (i)
				{
					case 0:
						vector_of_the_switch_names.push_back(line);
					    total_switch_records++;
						break;
					case 1:
						vector_of_the_descriptions.push_back(line);
						break;
					case 2:
						vector_of_the_firmware_versions.push_back(line);
						break;
					case 3:
						vector_of_the_guids.push_back(line);
						break;
					case 4:
						vector_of_the_has_ufm_agents.push_back(line);
						break;
					case 5:
						vector_of_the_ips.push_back(line);
						break;
					case 6:
						//last record for bad record
						// unfortunately only record
						if(bad_record){ 
							bad_switch_records << "model:                 " << line << std::endl; 
							//because its last field. pad the record with a new line. 
							bad_switch_records << std::endl;
						} 
					    else{ vector_of_the_model.push_back(line); }
					    // because last field of ufm record. 
					    //reset to a new record
						bad_record = false;
						break;
					case 7:
						//vector_of_the_num_modules.push_back(line);
						break;
					case 8:
						vector_of_the_num_ports.push_back(line);
						break;
					case 9:
						vector_of_the_physical_frame_locations.push_back(line);
						break;
					case 10:
						vector_of_the_physical_u_locations.push_back(line);
						break;
					case 11:
						vector_of_the_ps_ids.push_back(line);
						break;
					case 12:
						vector_of_the_roles.push_back(line);
						break;
					case 13:
						vector_of_the_server_operation_modes.push_back(line);
						break;
					case 14:
						vector_of_the_sm_modes.push_back(line);
						break;
					case 15:
						vector_of_the_states.push_back(line);
						break;
					case 16:
						vector_of_the_sw_versions.push_back(line);
						break;
					case 17:
						vector_of_the_system_guids.push_back(line);
						break;
					case 18:
						vector_of_the_system_names.push_back(line);
						break;
					case 19:
						vector_of_the_total_alarms.push_back(line);
						break;
					case 20:
						vector_of_the_types.push_back(line);
						break;
					case 21:
						vector_of_the_vendors.push_back(line);
						break;
					case 22:
						vector_of_the_serial_numbers.push_back(line);
						break;
					case 23:
						vector_of_the_os_versions.push_back(line);
						break;
				}

				//I think unfortunately for now . we have to do this here. this way
				if(bad_record)
				{
					//copy the already added fields to the bad_record file
					bad_switch_records << "Switch: " << total_switch_records << std::endl;
					bad_switch_records << "ip:                    " << vector_of_the_ips[vector_of_the_ips.size()-1] << std::endl;
					bad_switch_records << "fw_version:            " << vector_of_the_firmware_versions[vector_of_the_firmware_versions.size()-1] << std::endl;
					bad_switch_records << "total_alarms:          " << vector_of_the_total_alarms[vector_of_the_total_alarms.size()-1] << std::endl;
					bad_switch_records << "psid:                  " << vector_of_the_ps_ids[vector_of_the_ps_ids.size()-1] << std::endl;
					bad_switch_records << "guid:                  " << vector_of_the_guids[vector_of_the_guids.size()-1] << std::endl;
					bad_switch_records << "state:                 " << vector_of_the_states[vector_of_the_states.size()-1] << std::endl;
					bad_switch_records << "role:                  " << vector_of_the_roles[vector_of_the_roles.size()-1] << std::endl;
					bad_switch_records << "type:                  " << vector_of_the_types[vector_of_the_types.size()-1] << std::endl;
					bad_switch_records << "vendor:                " << vector_of_the_vendors[vector_of_the_vendors.size()-1] << std::endl;
					bad_switch_records << "description:           " << vector_of_the_descriptions[vector_of_the_descriptions.size()-1] << std::endl;
					bad_switch_records << "has_ufm_agent:         " << vector_of_the_has_ufm_agents[vector_of_the_has_ufm_agents.size()-1] << std::endl;
					bad_switch_records << "server_operation_mode: " << vector_of_the_server_operation_modes[vector_of_the_server_operation_modes.size()-1] << std::endl;
					bad_switch_records << "sm_mode:               " << vector_of_the_sm_modes[vector_of_the_sm_modes.size()-1] << std::endl;
					bad_switch_records << "system_name:           " << vector_of_the_system_names[vector_of_the_system_names.size()-1] << std::endl;
					bad_switch_records << "sw_version:            " << vector_of_the_sw_versions[vector_of_the_sw_versions.size()-1] << std::endl;
					bad_switch_records << "system_guid:           " << vector_of_the_system_guids[vector_of_the_system_guids.size()-1] << std::endl;
					bad_switch_records << "name:                  " << vector_of_the_switch_names[vector_of_the_switch_names.size()-1] << std::endl;
					bad_switch_records << "modules:               ???" << std::endl;
					bad_switch_records << "serial_number:         N/A" << std::endl;

					//remove already added fields to lists.
					vector_of_the_ips.pop_back();
					vector_of_the_firmware_versions.pop_back();
					vector_of_the_total_alarms.pop_back();
					vector_of_the_ps_ids.pop_back();
					vector_of_the_guids.pop_back();
					vector_of_the_states.pop_back();
					vector_of_the_roles.pop_back();
					vector_of_the_types.pop_back();
					vector_of_the_vendors.pop_back();
					vector_of_the_descriptions.pop_back();
					vector_of_the_has_ufm_agents.pop_back();
					vector_of_the_server_operation_modes.pop_back();
					vector_of_the_sm_modes.pop_back();
					vector_of_the_system_names.pop_back();
					vector_of_the_sw_versions.pop_back();
					vector_of_the_system_guids.pop_back();
					vector_of_the_switch_names.pop_back();
				}
			}				
		}

		std::cout << "UFM reported " << total_switch_records << " switch records." << std::endl;
		std::cout << "This report from UFM can be found in '" << ufm_switch_output_file_name << "' located at '" << csm_inv_log_dir << "'" << std::endl;

		if(NA_serials_count > 0){
			std::cerr << "WARNING: " << NA_serials_count << " Switches found with 'N/A' serial numbers and have been removed from CSM inventory collection data." << std::endl;
			std::cerr << "These records copied into '" << switch_errors <<"' located at '" << csm_inv_log_dir << "'" << std::endl;
		}

		std::cout << std::endl;

		bad_switch_records << "\nTotal Bad Records: " << NA_serials_count << "\n" << std::endl;

		// closing the input file
		input_file.close();
		//close the error file
		bad_switch_records.close();
	}
	catch (std::exception& e)
	{
		// prnting exception
		std::cout << "Exception thrown during Switch inventory connection: " << e.what() << std::endl;
	}

	return 0;
}

int INV_SWITCH_CONNECTOR_ACCESS::module_key_value_vector_builder(char* module_key, char* module_value)
{

	int check = 0; 

	// take in keys and values and make fautso vectors
	for(unsigned int i = 0; i < vector_of_the_comparing_strings_modules.size(); i++)
	{
		
		check = strcmp(module_key, vector_of_the_comparing_strings_modules[i].c_str());

		if( check != 0)
		{
			continue;
		}

		switch(i)
		{
			case 0:
				module_status.push_back(module_value);
				break;
			case 1:
				module_hw_version.push_back(module_value);
				break;
			case 2:
				module_name.push_back(module_value);
				break;
			case 3:
				module_number_of_chips.push_back(module_value);
				break;
			case 4:
				module_description.push_back(module_value);
				break;
			case 5:
				module_max_ib_ports.push_back(module_value);
				break;
			case 6:
				module_module_index.push_back(module_value);
				break;
			case 7:
				module_device_type.push_back(module_value);
				break;
			case 8:
				module_serial_number.push_back(module_value);
				break;
			case 9:
				module_path.push_back(module_value);
				break;
			case 10:
				module_device_name.push_back(module_value);
				break;
			case 11:
				module_severity.push_back(module_value);
				break;
			default:
				//error?
				break;
		}

	}

	return 0;
}

std::string INV_SWITCH_CONNECTOR_ACCESS::ReturnFieldValue_module(std::string key, unsigned long int index)
{

	// setting field value
	std::string field_value = "NULL";
	std::size_t found; 
	
	// Notes to remember when someone fixes later.
	// inventory can have different number of modules. 
	// this is tracked outside this function. (via index parameter)
	// the way fautso vectors work is one long list.
	// gotta offset this list by the number of total modules coming before
	// when we build a json tree from the raw json file, hopefully we can eliminate this whole list system. 

	// take in keys and values and make fautso vectors
	for(unsigned int i = 0; i < vector_of_the_comparing_strings_modules.size(); i++)
	{

		found = vector_of_the_comparing_strings_modules[i].find("\""+key+"\"");

		if( found == std::string::npos )
		{
			continue;
		}

		// id and index in bound
		switch (i)
		{
			case 0:
				if(index < module_status.size())
				{
					field_value = module_status.at( index );
				}
				break;
			case 1:
				if(index < module_hw_version.size())
				{
					field_value = module_hw_version.at(index);
				}
				break;
			case 2:
				if(index < module_name.size())
				{
					field_value = module_name.at(index);
				}
				break;
			case 3:
				if(index < module_number_of_chips.size())
				{
					field_value = module_number_of_chips.at(index);
				}
				break;
			case 4:
				if(index < module_description.size())
				{
					field_value = module_description.at(index);
				}
				break;
			case 5:
				if(index < module_max_ib_ports.size())
				{
					field_value = module_max_ib_ports.at(index);
				}
				break;
			case 6:
				if(index < module_module_index.size())
				{
					field_value = module_module_index.at(index);
				}
				break;
			case 7:
				if(index < module_device_type.size())
				{
					field_value = module_device_type.at(index);
				}
				break;
			case 8:
				if(index < module_serial_number.size())
				{
					field_value = module_serial_number.at(index);
				}
				break;
			case 9:
				if(index < module_path.size())
				{
					field_value = module_path.at(index);
				}
				break;
			case 10:
				if(index < module_device_name.size())
				{
					field_value = module_device_name.at(index);
				}
				break;
			case 11:
				if(index < module_severity.size())
				{
					field_value = module_severity.at(index);
				}
				break;
			default:
				//error?
				break;
		}

		// return real value of the field
		return field_value;

	}

	// return real value of the field
	return field_value;
}


std::string INV_SWITCH_CONNECTOR_ACCESS::ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector)
{
	// setting field value
	std::string field_value = "NULL";

	// checking on the vector_id
	if ( vector_id >= number_of_field_vectors )
	{
		// return NULL value for the field
		return field_value;
	}

	// checking on the index in the vector
	if ( index_in_the_vector >= vector_of_the_switch_names.size() && vector_id != 24)
	{
		// return NULL value for the field
		return field_value;
	}

	// id and index in bound
	switch (vector_id)
	{
		case 0:
			field_value = vector_of_the_switch_names.at( index_in_the_vector );
			break;
		case 1:
			field_value = vector_of_the_descriptions.at( index_in_the_vector );
			break;
		case 2:
			field_value = vector_of_the_firmware_versions.at( index_in_the_vector );
			break;
		case 3:
			field_value = vector_of_the_guids.at( index_in_the_vector );
			break;
		case 4:
			field_value = vector_of_the_has_ufm_agents.at( index_in_the_vector );
			break;
		case 5:
			field_value = vector_of_the_ips.at( index_in_the_vector );
			break;
		case 6:
			field_value = vector_of_the_model.at( index_in_the_vector );
			break;
		case 7:
			field_value = vector_of_the_num_modules.at( index_in_the_vector );
			break;
		case 8:
			field_value = vector_of_the_num_ports.at( index_in_the_vector );
			break;
		case 9:
			field_value = vector_of_the_physical_frame_locations.at( index_in_the_vector );
			break;
		case 10:
			field_value = vector_of_the_physical_u_locations.at( index_in_the_vector );
			break;
		case 11:
			field_value = vector_of_the_ps_ids.at( index_in_the_vector );
			break;
		case 12:
			field_value = vector_of_the_roles.at( index_in_the_vector );
			break;
		case 13:
			field_value = vector_of_the_server_operation_modes.at( index_in_the_vector );
			break;
		case 14:
			field_value = vector_of_the_sm_modes.at( index_in_the_vector );
			break;
		case 15:
			field_value = vector_of_the_states.at( index_in_the_vector );
			break;
		case 16:
			field_value = vector_of_the_sw_versions.at( index_in_the_vector );
			break;
		case 17:
			field_value = vector_of_the_system_guids.at( index_in_the_vector );
			break;
		case 18:
			field_value = vector_of_the_system_names.at( index_in_the_vector );
			break;
		case 19:
			field_value = vector_of_the_total_alarms.at( index_in_the_vector );
			break;
		case 20:
			field_value = vector_of_the_types.at( index_in_the_vector );
			break;
		case 21:
			field_value = vector_of_the_vendors.at( index_in_the_vector );
			break;
		case 22:
			field_value = vector_of_the_serial_numbers.at( index_in_the_vector );
			break;
		case 23:
			field_value = vector_of_the_os_versions.at( index_in_the_vector );
			break;
		case 24:
			if(index_in_the_vector >= vector_of_the_modules.size())
			{
				field_value = "N/A";
			}else{
				field_value = vector_of_the_modules.at( index_in_the_vector );
				//field_value = "N/A";
			}
			
			break;
	}

	// return real value of the field
	return field_value;
}

int INV_SWITCH_CONNECTOR_ACCESS::TotalNumberOfRecords()
{
	return vector_of_the_switch_names.size();
}

#endif  // SWITCH_CONNECTOR support is enabled

/////////////////////////////////////////////////////////////////////////////////
// SWITCH_CONNECTOR support is disabled
/////////////////////////////////////////////////////////////////////////////////

#ifndef SWITCH_CONNECTOR

INV_SWITCH_CONNECTORACCESS::INV_SWITCH_CONNECTOR_ACCESS()
{
	// setting variables
	compiled_with_support = 0;
}

INV_SWITCH_CONNECTOR_ACCESS::~INV_SWITCH_CONNECTOR_ACCESS()
{

}

int INV_SWITCH_CONNECTOR_ACCESS::GetCompiledWithSupport()
{
	return compiled_with_support;
}

int INV_SWITCH_CONNECTOR_ACCESS::ExecuteDataCollection()
{
	return 0;
}

std::string INV_SWITCH_CONNECTOR_ACCESS::ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector)
{
	return "NULL";
}

std::string INV_SWITCH_CONNECTOR_ACCESS::ReturnFieldValue_module(std::string key, unsigned long int index)
{
	return "NULL";
}

int INV_SWITCH_CONNECTOR_ACCESS::TotalNumberOfRecords()
{
	return vector_of_the_switch_names.size();
}

int INV_SWITCH_CONNECTOR_ACCESS::extractValueFromLine(std::string line, char** value, char* key, bool finalLine)
{

	// need to go through and look at this code and clean it up and document...it really is a mess. :( 


	return 0;
}



#endif  // SWITCH_CONNECTOR support is disabled
