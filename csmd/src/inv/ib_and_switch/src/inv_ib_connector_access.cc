/*================================================================================

    csmd/src/inv/ib_and_switch/src/inv_ib_connector_access.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "../include/inv_ib_connector_access.h"
#include "logging.h"

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

INV_IB_CONNECTOR_ACCESS *INV_IB_CONNECTOR_ACCESS::_Instance = nullptr;

/////////////////////////////////////////////////////////////////////////////////
// IB_CONNECTOR support is enabled
/////////////////////////////////////////////////////////////////////////////////

#ifdef IB_CONNECTOR

INV_IB_CONNECTOR_ACCESS::INV_IB_CONNECTOR_ACCESS()
{
	// setting variables
	compiled_with_support = 1;
	number_of_field_vectors = 13;
	
	// setting vector of the comparing strings
	vector_of_the_comparing_strings.push_back("\"part_number\"");        // 0  
	vector_of_the_comparing_strings.push_back("\"length\"");             // 1
	vector_of_the_comparing_strings.push_back("\"serial_number\"");      // 2
	vector_of_the_comparing_strings.push_back("\"identifier\"");         // 3
	vector_of_the_comparing_strings.push_back("\"technology\"");         // 4
	vector_of_the_comparing_strings.push_back("\"revision\"");           // 5
	vector_of_the_comparing_strings.push_back("\"severity\"");           // 6
	vector_of_the_comparing_strings.push_back("\"source_guid\"");        // 7
	vector_of_the_comparing_strings.push_back("\"width\"");              // 8
	vector_of_the_comparing_strings.push_back("\"source_port\"");        // 9
	vector_of_the_comparing_strings.push_back("\"destination_port\"");   // 10
	vector_of_the_comparing_strings.push_back("\"destination_guid\"");   // 11
	vector_of_the_comparing_strings.push_back("\"name\"");               // 12
}

INV_IB_CONNECTOR_ACCESS::~INV_IB_CONNECTOR_ACCESS()
{
	// clearing vectors
	vector_of_the_comparing_strings.clear();        // 0
	vector_of_the_part_numbers.clear();
	vector_of_the_lengths.clear();
	vector_of_the_serial_numbers.clear();
	vector_of_the_identifiers.clear();
	vector_of_the_technologies.clear();
	vector_of_the_revisions.clear();
	vector_of_the_severities.clear();
	vector_of_the_source_guids.clear();
	vector_of_the_widths.clear();
	vector_of_the_source_ports.clear();
	vector_of_the_destination_ports.clear();
	vector_of_the_destination_guids.clear();
	vector_of_the_names.clear();
}

int INV_IB_CONNECTOR_ACCESS::GetCompiledWithSupport()
{
	return compiled_with_support;
}

int INV_IB_CONNECTOR_ACCESS::ExecuteDataCollection(std::string rest_address, std::string authentication_string_for_the_http_request, std::string csm_inv_log_dir, std::string ib_cable_errors)
{
	try
	{
		// Get a list of endpoints corresponding to the server name.
		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(rest_address.c_str(),"http");
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	
		// Try each endpoint until we successfully establish a connection.
		tcp::socket socket(io_service);
		boost::asio::connect(socket, endpoint_iterator);
	
		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		
		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << "GET /ufmRest/resources/links?cable_info=true HTTP/1.1\r\n";
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
		if (!response_stream || http_version.substr(0, 5) != "HTTP/"){
			std::cerr << "Invalid response\n";
			return 1;
		}
		
		if (status_code != 200){
			std::cerr << "Response returned with status code " << status_code << "\n";
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
			// ???
		}
	
		// Write whatever content we already have to output.
		std::stringstream ss;
		if (response.size() > 0){
			// necessary at cause of repetition
			ss << &response;
		}
		
		// opening output file
		std::string output_file_name = csm_inv_log_dir + "/ufm_ib_cable_output_file.txt";
		std::ofstream output_file(output_file_name.c_str(),std::ios::out);
		
		// checking if output file is open
		if ( ! output_file.is_open() ){
			// printing error and return
			std::cerr << "Output file " << output_file_name << " not open, return"  << std::endl;
			return 1;
		}else{
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
			if (error != boost::asio::error::eof){
				throw boost::system::system_error(error);
			}
		}
	
		// vectors with the fields
		std::size_t position_delimiter;
		std::string line;
		std::string comparing_string;
		
		// opening input file
		std::string input_file_name = output_file_name;
		std::ifstream input_file(input_file_name.c_str(),std::ios::in);
		
		// checking if input file is open
		if ( ! input_file.is_open() ){
			// printing error and return
			std::cerr << "Input file " << input_file_name << " not open, return" << std::endl;
			return 1;
		
		} else {
			//grab time info to stick into the bad cable text file
			time_t rawtime;
			struct tm * timeinfo;
	
			time (&rawtime);
			timeinfo = localtime (&rawtime);
		
			//opening a error write file
			std::ofstream bad_ib_cable_records;
			
			// csm_inv_log_dir is path
			// ib_cable_errors is file
			std::cout << "ib error full path: " << csm_inv_log_dir << "/" << ib_cable_errors << std::endl;
			
			bad_ib_cable_records.open (csm_inv_log_dir + "/" + ib_cable_errors);
			bad_ib_cable_records << "CSM IB cable inventory collection" << std::endl;
			bad_ib_cable_records << "File created: " << asctime(timeinfo) << std::endl;
			bad_ib_cable_records << "The following records are incomplete and can not be inserted into CSM database.\n" << std::endl;
	
			// printing
			std::cout << "reading input file" << std::endl;
			
			//helper variable to keep track of a bad serial number for a record. 
			bool bad_record = false;
			bool missing_cable_info = false;
			//helper variable to keep track of total records.
			int total_ib_records = 0;
			int NA_serials_count = 0;
			int missing_cable_info_count = 0;
			
			// reading the input file lines
			std::string line;
			while (std::getline(input_file, line))
			{
				// reading the line
				std::istringstream iss(line);
			
				// check if this is one of the strange lines (cable not complete)
				if ( line.find("\"cable_info\": {},") != std::string::npos ){
					//copy the missing cable_info fields to the bad_record file
					bad_ib_cable_records << "Cable: " << vector_of_the_part_numbers.size()-1 << std::endl;
					bad_ib_cable_records << "part_number:      " << "NOT AVAILABLE" << std::endl;
					bad_ib_cable_records << "length:           " << "NOT AVAILABLE" << std::endl;
					bad_ib_cable_records << "serial_number:    " << "NOT AVAILABLE" << std::endl;
					bad_ib_cable_records << "identifier:       " << "NOT AVAILABLE" << std::endl;
					bad_ib_cable_records << "technology:       " << "NOT AVAILABLE" << std::endl;
					bad_ib_cable_records << "revision:         " << "NOT AVAILABLE" << std::endl;
					missing_cable_info = true;
					total_ib_records++;
					missing_cable_info_count++;
				}
	
				// cycling of the comparing strings
				for (unsigned int i = 0; i < vector_of_the_comparing_strings.size(); i++)
				{
					// reading comparing string
					comparing_string=vector_of_the_comparing_strings[i];
				
					// checking if comparing string is contained in the read line
					if ( line.find(comparing_string) != std::string::npos )
					{
						// extraction field
						//use the delimiter to find where to start grabbing the value
						position_delimiter=line.find(":");
						//erase information before the delimiter
						line.erase(0,position_delimiter+3);
						
						// modification field, the last is length and is particular (this work this day 2015 08 23 at 12:24) (-1 and -3)
						// modification field at cause of UFM 5.9.5 to make the parse work also for length, last field (2015 09 20 at 10:51)
						if ( i == 5 || i == 12 ){
							//remove the suffix.
							//revision i==5 and name i== 12 are the last field. aka no comma. only remove one character
							line.erase(line.length()-1,line.length());
						} else {
							//remove the suffix.
							//normal line. needs to remove 3 characters
							line.erase(line.length()-3,line.length()-1);
						}
						
						//Is this a serial number?
						if(i == 2){
							//is it a bad serial_number?
							if(line.find("NA") != std::string::npos){
								//override i value, set to extra case 13
								i = 13;
							}
						}
		
						// updating vectors
						switch (i)
						{
							case 0:
								vector_of_the_part_numbers.push_back(line);
								break;
							case 1:
								vector_of_the_lengths.push_back(line);
								break;
							case 2:
								vector_of_the_serial_numbers.push_back(line);
								//increase the record count
								total_ib_records++;
								break;
							case 3:
								if(bad_record){ bad_ib_cable_records << "identifier:       " << line << std::endl; } 
								else{ vector_of_the_identifiers.push_back(line);}
								break;
							case 4:
								if(bad_record){ bad_ib_cable_records << "technology:       " << line << std::endl; }
								else{vector_of_the_technologies.push_back(line);}
								break;
							case 5:
								if(bad_record){ bad_ib_cable_records << "revision:         " << line << std::endl; }
								else{vector_of_the_revisions.push_back(line);}
								break;
							case 6:
								if(bad_record || missing_cable_info){ bad_ib_cable_records << "severity:         " << line << std::endl; }
								else{vector_of_the_severities.push_back(line);}
								break;
							case 7:
								if(bad_record || missing_cable_info){ bad_ib_cable_records << "source_guid:      " << line << std::endl; }
								else{vector_of_the_source_guids.push_back(line); }
								break;
							case 8:
								if(bad_record || missing_cable_info){ bad_ib_cable_records << "width:            " << line << std::endl; }
								else{vector_of_the_widths.push_back(line); }
								break;
							case 9:
								if(bad_record || missing_cable_info){ bad_ib_cable_records << "source_port:      " << line << std::endl; }
								else{vector_of_the_source_ports.push_back(line); }
								break;
							case 10:
								if(bad_record || missing_cable_info){ bad_ib_cable_records << "destination_port: " << line << std::endl; }
								else{vector_of_the_destination_ports.push_back(line); }
								break;
							case 11:
								if(bad_record || missing_cable_info){ bad_ib_cable_records << "destination_guid: " << line << std::endl; }
								else{vector_of_the_destination_guids.push_back(line); }
								break;
							case 12:
								if(bad_record || missing_cable_info){ 
									bad_ib_cable_records << "name:             " << line << std::endl;
									//pad the end of a record in the bad output file
									bad_ib_cable_records << "" << std::endl;
								}else{ 
									vector_of_the_names.push_back(line); 
								}
								//names is last value of a record. reset bad record back to normal.
								bad_record = false;
								missing_cable_info = false;
								break;
							case 13:
								//special case with a bad serial number. 
								//copy the already added fields to the bad_record file
								bad_ib_cable_records << "Cable: " << vector_of_the_part_numbers.size()-1 << std::endl;
								bad_ib_cable_records << "part_number:      " << vector_of_the_part_numbers[vector_of_the_part_numbers.size()-1] << std::endl;
								bad_ib_cable_records << "length:           " << vector_of_the_lengths[vector_of_the_lengths.size()-1] << std::endl;
								bad_ib_cable_records << "serial_number:    " << line << std::endl;
								//remove already added fields to lists.
								vector_of_the_part_numbers.pop_back();
								vector_of_the_lengths.pop_back();
								bad_record = true;
								//increase the record count
								total_ib_records++;
								NA_serials_count++;
								break;
						}
						//a match was found. set i = max so we don't continue to check for matches.
						i = vector_of_the_comparing_strings.size();
					}
				}
			}
	
			if(NA_serials_count > 0){
				std::cerr << "\nWARNING: " << NA_serials_count << " IB cables found with 'NA' serial numbers and have been removed from CSM inventory collection data." << std::endl;
				std::cerr << "These records copied into 'bad_ib_cable_records.txt' located at '/var/log/ibm/csm/inv/' \n" << std::endl;
			}
			
			if(missing_cable_info_count > 0){
				std::cerr << "\nWARNING: " << missing_cable_info_count << " IB cables found with no 'cable_info' and have been removed from CSM inventory collection data." << std::endl;
				std::cerr << "These records copied into 'bad_ib_cable_records.txt' located at '/var/log/ibm/csm/inv/' \n" << std::endl;
			}
			
			std::cout << "\nUFM reported " << total_ib_records << " IB records.\n" << std::endl;
		
			// closing the input file
			input_file.close();
			//close the error file
			bad_ib_cable_records.close();
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Exception thrown during IB inventory connection: " << e.what() << std::endl;
	}

	return 1;
}

std::string INV_IB_CONNECTOR_ACCESS::ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector)
{
	// setting field value
	std::string field_value = "NULL";
	
	// checking on the vector_id
	if ( vector_id >= number_of_field_vectors ){
		// return NULL value for the field
		return field_value;
	}

	// checking on the index in the vector
	if ( index_in_the_vector >= vector_of_the_serial_numbers.size() ){
		// return NULL value for the field
		return field_value;
	}

	// id and index in bound
	switch (vector_id)
	{
		case 0:
			field_value = vector_of_the_part_numbers.at( index_in_the_vector );
			break;
		case 1:
			field_value = vector_of_the_lengths.at( index_in_the_vector );
			break;
		case 2:
			field_value = vector_of_the_serial_numbers.at( index_in_the_vector );
			break;
		case 3:
			field_value = vector_of_the_identifiers.at( index_in_the_vector );
			break;
		case 4:
			field_value = vector_of_the_technologies.at( index_in_the_vector );
			break;
		case 5:
			field_value = vector_of_the_revisions.at( index_in_the_vector );
			break;
		case 6:
			field_value = vector_of_the_severities.at( index_in_the_vector );
			break;
		case 7:
			field_value = vector_of_the_source_guids.at( index_in_the_vector );
			break;
		case 8:
			field_value = vector_of_the_widths.at( index_in_the_vector );
			break;
		case 9:
			field_value = vector_of_the_source_ports.at( index_in_the_vector );
			break;
		case 10:
			field_value = vector_of_the_destination_ports.at( index_in_the_vector );
			break;
		case 11:
			field_value = vector_of_the_destination_guids.at( index_in_the_vector );
			break;
		case 12:
			field_value = vector_of_the_names.at( index_in_the_vector );
			break;	
	}

	// return real value of the field
	return field_value;
}

int INV_IB_CONNECTOR_ACCESS::TotalNumberOfRecords()
{
	return vector_of_the_serial_numbers.size();
}

#endif  // IB_CONNECTOR support is enabled

/////////////////////////////////////////////////////////////////////////////////
// IB_CONNECTOR support is disabled
/////////////////////////////////////////////////////////////////////////////////

#ifndef IB_CONNECTOR

INV_IB_CONNECTOR_ACCESS::INV_IB_CONNECTOR_ACCESS()
{
	// setting variables
	compiled_with_support = 0;
}

INV_IB_CONNECTOR_ACCESS::~INV_IB_CONNECTOR_ACCESS()
{

}

int INV_IB_CONNECTOR_ACCESS::GetCompiledWithSupport()
{
	return compiled_with_support;
}

int INV_IB_CONNECTOR_ACCESS::ExecuteDataCollection()
{
	return 0;
}

std::string INV_IB_CONNECTOR_ACCESS::ReturnFieldValue(unsigned long int vector_id, unsigned long int index_in_the_vector)
{
	return "NULL";
}

int INV_IB_CONNECTOR_ACCESS::TotalNumberOfRecords()
{
	return vector_of_the_serial_numbers.size();
}

#endif  // IB_CONNECTOR support is disabled