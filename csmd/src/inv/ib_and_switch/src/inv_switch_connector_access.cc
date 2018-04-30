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
}

int INV_SWITCH_CONNECTOR_ACCESS::GetCompiledWithSupport()
{
	return compiled_with_support;
}

int INV_SWITCH_CONNECTOR_ACCESS::ExecuteDataCollection(std::string rest_address, std::string authentication_string_for_the_http_request)
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
		
		// opening output file
		std::string output_file_name = "output_file.txt";
		std::cout << "output file name: " << output_file_name << std::endl;
		std::ofstream output_file(output_file_name.c_str(),std::ios::out);

		// checking if output file is open
		if ( ! output_file.is_open() )
		{
			// printing error and return
			std::cout << "Output file " << output_file_name << " not open, return"  << std::endl;
			return 1;
		} 
		else 
		{
			// updating output file
			// response_copy_1 is not nedded because response_copy_2 is equal to response_copy_2
			output_file << response_copy_2;
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
		}

		// vectors with the fields
		std::size_t position_delimiter;
		std::string line;
		std::string comparing_string;

		// opening input file
		std::string input_file_name = "output_file.txt";
		std::cout << "input file name: " << input_file_name << std::endl;
		std::ifstream input_file(input_file_name.c_str(),std::ios::in);

		// checking if input file is open
		if ( ! input_file.is_open() )
		{
			// printing error and return
			std::cout << "Input file " << input_file_name << " not open, return" << std::endl;
			return 1;
		}else{
			// printing
			std::cout << "reading input file..." << std::endl;

			// setting the number of lines to skip
			int number_of_lines_to_skip = 0;

			// reading the input file lines
			std::string line;
			while (std::getline(input_file, line))
			{
				// reading the line
				std::istringstream iss(line);
		
				number_of_lines_to_skip = number_of_lines_to_skip + 1;
				if ( number_of_lines_to_skip < 21 )
				{
					continue;
				}

				// cycling of the cpmaring strings
				for (unsigned int i = 0; i < vector_of_the_comparing_strings.size(); i++)
				{
					// reading comarping string
					comparing_string = vector_of_the_comparing_strings[i];
		
					// checking if comparing string is contained in the read line
					if ( line.find(comparing_string) != std::string::npos )
					{
						if(i == 24){
							// special case: "modules"
							// modules can either be 'slim' or 'expanded'
							//can also be blank? = "[]," this could be a broken case. but we encountered so plan for it.
							// following code determines which is true, and then collects the data.
					
							int number_of_modules_found = 0;
							
							//check for the broken "[],"
							if(line.find("[],") != std::string::npos){
								//push num_modules
								vector_of_the_num_modules.push_back(std::to_string(number_of_modules_found));
								
								//Should prob report some error here.
								
								//idk, exit early, no need to further continue the main loop.
							}else{
								// read the next line
								std::getline(input_file, line);
								
								// determine if 'slim' or 'expanded'
								if(line.find("{") == std::string::npos){
									// 'slim' case
									// only a list of module names are found
									
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
							
									//SAVE NAME HERE:
									vector_of_the_modules.push_back(line); //name
									//SAVE OTHER FIELDS AS N/A
									vector_of_the_modules.push_back("N/A"); // description
									vector_of_the_modules.push_back("N/A"); // device_name
									vector_of_the_modules.push_back("N/A"); // device_type
									vector_of_the_modules.push_back("N/A"); // max_ib_ports
									vector_of_the_modules.push_back("N/A"); // module_index
									vector_of_the_modules.push_back("N/A"); // number_of_chips
									vector_of_the_modules.push_back("N/A"); // path 
									vector_of_the_modules.push_back("N/A"); // serial_number
									vector_of_the_modules.push_back("N/A"); // severity
									vector_of_the_modules.push_back("N/A"); // status
							
									//increase the count
									number_of_modules_found++;
									
									//continue reading list of names until end of list
									//exit condition variable
									char close_found = 0;
									do{
										//grab next name
										std::getline(input_file, line);
										
										if(line.find("]") == std::string::npos)
										{
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
											
											//not in correct order - but doesn't matter because all "N/A"
											//SAVE OTHER FIELDS AS N/A
											vector_of_the_modules.push_back("N/A"); // description
											vector_of_the_modules.push_back("N/A"); // device_name
											vector_of_the_modules.push_back("N/A"); // device_type
											vector_of_the_modules.push_back("N/A"); // max_ib_ports
											vector_of_the_modules.push_back("N/A"); // module_index
											vector_of_the_modules.push_back("N/A"); // number_of_chips
											vector_of_the_modules.push_back("N/A"); // path 
											vector_of_the_modules.push_back("N/A"); // serial_number
											vector_of_the_modules.push_back("N/A"); // severity
											vector_of_the_modules.push_back("N/A"); // status
											//SAVE NAME HERE:
											vector_of_the_modules.push_back(line); //name
									
											//increase the count
											number_of_modules_found++;
										}else{
											std::cout << "] found " << std::endl;
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
									
									do{
										if(line.find("]") == std::string::npos)
										{
											//still searching for modules
											//grab next name
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											vector_of_the_modules.push_back(line); // status 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											vector_of_the_modules.push_back(line); // description 
									
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											//don't push host
											//vector_of_the_modules.push_back(line); // hosting_system_guid 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											vector_of_the_modules.push_back(line); // severity 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+2);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-2,line.length());
											vector_of_the_modules.push_back(line); // number_of_chips 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+2);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-2,line.length());
											vector_of_the_modules.push_back(line); // max_ib_ports 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+2);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-2,line.length());
											vector_of_the_modules.push_back(line); // module_index 
											
											//grab next line
											std::getline(input_file, line);
									
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											//don't push temp
											//vector_of_the_modules.push_back(line); // temperature 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											vector_of_the_modules.push_back(line); // device_type 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											vector_of_the_modules.push_back(line); // serial_number 
									
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											vector_of_the_modules.push_back(line); // path 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-3,line.length()-1);
											vector_of_the_modules.push_back(line); // device_name 
											
											//grab next line
											std::getline(input_file, line);
											
											// extraction field
											position_delimiter=line.find(":");
											//Modify the prefix. Trim the opening garbage from the key_value
											line.erase(0,position_delimiter+3);
											//Modify the suffix. Trim the ending garbage.
											line.erase(line.length()-1,line.length());
											vector_of_the_modules.push_back(line); // name
											
											//grab next line
											std::getline(input_file, line); // },
											std::getline(input_file, line); // { or ],
												
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
						if(i == 19 || i == 4 || i == 16){
							//delimiter, space
							//2 char spaces
							line.erase(0,position_delimiter+2);
						}else{
							//delimiter, space, double quote
							//3 char spaces
							line.erase(0,position_delimiter+3);
						}
				
						//Modify the suffix. Trim the ending garbage.
						if(i == 19 || i == 4 || i == 16){
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
								//LOG(csmd, debug) << "updating vector of the switch names";
								//std::cout << "updating vector of the switch names" <<std::endl;
								vector_of_the_switch_names.push_back(line);
								break;
							case 1:
								//LOG(csmd, debug) << "updating vector of the serial numbers";
								//std::cout << "updating vector of the serial numbers" << std::endl;
								vector_of_the_descriptions.push_back(line);
								break;
							case 2:
								//LOG(csmd, debug) << "updating vector of the os versions";
								//std::cout << "updating vector of the os versions" << std::endl;
								vector_of_the_firmware_versions.push_back(line);
								break;
							case 3:
								//LOG(csmd, debug) << "updating vector of the firmware versions";
								//std::cout << "updating vector of the tirmware versions" << std::endl;
								vector_of_the_guids.push_back(line);
								break;
							case 4:
								//LOG(csmd, debug) << "updating vector of the pysical frame locations";
								//std::cout << "updating vector of the physical frame locations" << std::endl;
								vector_of_the_has_ufm_agents.push_back(line);
								break;
							case 5:
								//LOG(csmd, debug) << "updating vector of the physical u locations";
								//std::cout << "updating vector of the physical u locations" << std::endl;
								vector_of_the_ips.push_back(line);
								break;
							case 6:
								//LOG(csmd, debug) << "updating vector of the inventories";
								//std::cout << "updating vector of the inventories" << std::endl;
								vector_of_the_model.push_back(line);
								break;
							case 7:
								//LOG(csmd, debug) << "updating vector of the types";
								//std::cout << "updating vector of the types" << std::endl;
								//vector_of_the_num_modules.push_back(line);
								break;
							case 8:
								//LOG(csmd, debug) << "updating vector of the guids";
								//std::cout << "updating vector of the guids" << std::endl;
								//line = standardizeGuid( line );
								vector_of_the_num_ports.push_back(line);
								break;
							case 9:
								//LOG(csmd, debug) << "updating vector of the statuses";
								//std::cout << "updating vector of the statuses" << std::endl;
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
					}
				}
			}

			// closing the input file
			input_file.close();
		}
	}
	catch (std::exception& e)
	{
		// prnting exception
		std::cout << "Exception thrown during Switch inventory connection: " << e.what() << std::endl;
	}

	return 1;
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
			field_value = vector_of_the_modules.at( index_in_the_vector );
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

int INV_SWITCH_CONNECTOR_ACCESS::TotalNumberOfRecords()
{
	return vector_of_the_switch_names.size();
}

#endif  // SWITCH_CONNECTOR support is disabled
