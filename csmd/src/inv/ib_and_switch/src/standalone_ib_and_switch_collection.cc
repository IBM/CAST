/*================================================================================

    csmd/src/inv/ib_and_switch/src/standalone_ib_and_switch_collection.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

// C Include
#include <assert.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
// CSM Include
#include "csmi/include/csm_api_inventory.h"
#include "csmi/include/csm_api_consts.h"
// Needed for infrastructure logging
#include "csmutil/include/csmutil_logging.h"
// Command line macros for ease of use.
#include "csmi/src/common/include/csmi_internal_macros.h"
// include to convert Mellanox guid to use CSM standardized guid format
#include "../../include/inv_ib_guid.h"

/////////////////////////////////////////////////////////
#include "../include/inv_ib_connector_access.h"
#include "../include/inv_switch_connector_access.h"

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
/////////////////////////////////////////////////////////

///< For use as the usage variable in the input parsers.
#define USAGE help

void help(){
    puts("_____CSM_INVENTORY_COLLECTION_CMD_HELP_____");
    puts("USAGE:");
    puts("  standalone_ib_and_switch_collection ARGUMENTS [OPTIONS]");
    puts("  standalone_ib_and_switch_collection [-c config] [-t type] [-h] [-v verbose_level]");
    puts("");
    puts("SUMMARY: Used to collect switch and ib inventory and insert into CSM database.");
    puts("");
    puts("EXIT STATUS:");
    puts("  0  if OK,");
    puts("  1  if ERROR.");
    puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    standalone_ib_and_switch_collection expects 2 mandatory parameters");
	puts("    Argument      | Example value                 | Description  ");                                                 
	puts("    --------------|-------------------------------|--------------");
	puts("    -c, --config  | \"/etc/ibm/csm/csm_master.cfg\" | (STRING) Full path to the master configure file");
	puts("                  |                               | Default Value: \"/etc/ibm/csm/csm_master.cfg\"");
	puts("    -t, --type    | 3                             | (INT) Type of inventory collection.");
	puts("                  |                               | Valid values: ");
	puts("                  |                               |   1 = ib cables");
	puts("                  |                               |   2 = switches");
	puts("                  |                               |   3 = ib cables and switches");
	puts("                  |                               | Default Value: 3");
	puts("                  |                               | ");
	puts("  OPTIONAL:");
	puts("    standalone_ib_and_switch_collection can have 1 optional parameters");
	puts("    Argument             | Example value       | Description  ");                                                 
	puts("    ---------------------|---------------------|--------------");
	puts("    -d, --details        | NOT APPLICABLE      | (FLAG) Turn on a more detailed output for UFM inventory collection.");
	puts("    -i, --input_override | \"/temp/myFile.json\" | (STRING) Override the input field in the master config file.");
	puts("                         |                     | ");
	puts("");
    puts("GENERAL OPTIONS:");
    puts("[-h, --help]                  | Help.");
    puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
    puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  standalone_ib_and_switch_collection -c \"/etc/ibm/csm/csm_master.cfg\" -t 3");
	puts("");
    puts("____________________");
}

struct option longopts[] = {
    //general options
    {"help",           no_argument,       0, 'h'},
    {"verbose",        required_argument, 0, 'v'},
	{"config",         required_argument, 0, 'c'},
	{"type",           required_argument, 0, 't'},
	{"details",        no_argument,       0, 'd'},
	{"input_override", required_argument, 0, 'i'},
    {0,0,0,0}
};

// main
int main(int argc, char *argv[])
{
    // Program variables
    // -----------------
    // used for a check at the very end of program.
    // where we want to print success, but the let user know to scropll up to see some errors that were printed. 
    bool successful_with_errors = false; 

    // CSM Variables
    csm_api_object *csm_obj = NULL;

    // Helper Variables
    int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 2;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/* Variables for checking cmd line args */
	int opt;
    //char *arg_check = NULL; ///< Used in verifying the long arg values.
	/* getopt_long stores the option index here. */
	int indexptr = 0;
    //int i = 0;
    // value to help with some prints. informs if they should print or not.
    bool details = false; 
    //override the path and filename for switch input to be read from 'inv_switch_connector_access'
    bool custom_input_override = false;
    std::string override_path_and_filename = "";
	
    int totalSwitchRecords = 0;
    int totalIBRecords = 0;
	
	//temp place for input variables
	// full path to the master config file
	//default to etc path
	std::string full_path_to_the_master_config_file = "/etc/ibm/csm/csm_master.cfg";
	requiredParameterCounter++;
	// type of collection
	//default to full collection, aka 3
	long int type_of_collection = 3;
	requiredParameterCounter++;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:c:di:t:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'c':
				full_path_to_the_master_config_file = strdup(optarg);
                break;
			case 'd':
				details = true;
			    break;
			case 'i':
				custom_input_override = true;
				override_path_and_filename = strdup(optarg);
			    break;
			case 't':
				type_of_collection = std::atol(optarg);
				if(type_of_collection < 1 || type_of_collection > 3)
				{
					//error prevention. do something better later.
					//for now default to full inventory collection. 
					type_of_collection = 3;
				}
				break;
			default:
                csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
				return CSMERR_INVALID_PARAM;
		}
	}

	/*Handle command line args*/
	argc -= optind;
	argv += optind;
	
	/*Collect mandatory args*/
	/*Check to see if expected number of arguments is correct.*/
	if(requiredParameterCounter < NUMBER_OF_REQUIRED_ARGUMENTS || optionalParameterCounter < MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS){
		/*We don't have the correct number of needed arguments passed in.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s).");
		csmutil_logging(error, "    Encountered %i required parameter(s). Expected %i required parameter(s).", requiredParameterCounter, NUMBER_OF_REQUIRED_ARGUMENTS);
		csmutil_logging(error, "    Encountered %i optional parameter(s). Expected at least %i optional parameter(s).", optionalParameterCounter, MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS);
        USAGE();
		return CSMERR_MISSING_PARAM;
	}

	// printing
	if(details)
	{
		std::cout << "Loading the inputs..." << std::endl;
		std::cout << "full path to the master config file: " << full_path_to_the_master_config_file << std::endl;
		std::cout << "Type of collection: " << type_of_collection << std::endl;
		std::cout << std::endl;
	}
	
	
	// opening master config file
	std::ifstream input_file(full_path_to_the_master_config_file.c_str(),std::ios::in);
	
	// rest variables
	std::string rest_address  = "";
	std::string rest_username = "";
	std::string rest_password = "";
	
	//csm config variables
	std::string csm_inv_log_dir = "";
	std::string ib_cable_errors = "";
	std::string switch_errors = "";
	std::string ufm_switch_output_file_name = "";
	std::string ufm_switch_input_file_name = "";

	if(custom_input_override)
	{
		ufm_switch_input_file_name = override_path_and_filename;
	}

	std::string ssl_file_path = "";
	std::string ssl_file_name = "";

	// checking if master cinfig file is open
	if ( !input_file.is_open() )
	{
		// printing
		std::cout << "The master config file is not open" << std::endl;
		std::cout << "Did you provide the full correct path to the master config file?" << std::endl;
		std::cout << "run '-h' or '--help' for more details." << std::endl;
		std::cout << "Return without executing data collection" << std::endl;
		
		// return error
		return 1;
	} 
	else 
	{
		// variables
		std::size_t position_delimiter;
		
		// vector of the comparing strings
		std::vector<std::string> vector_of_the_comparing_strings;
		//ugm rest
		vector_of_the_comparing_strings.push_back("rest_address");
		vector_of_the_comparing_strings.push_back("rest_user");
		vector_of_the_comparing_strings.push_back("rest_password");
		//csm reporting
		vector_of_the_comparing_strings.push_back("csm_inv_log_dir");
		vector_of_the_comparing_strings.push_back("ib_cable_errors");
		vector_of_the_comparing_strings.push_back("switch_errors");
		//std::cout << "Initialize vector_of_the_comparing_strings" << std::endl;

		//elements in the list specific for checking a valid config file
		vector_of_the_comparing_strings.push_back("inventory");
		vector_of_the_comparing_strings.push_back("ufm");

		//elements in the list for finding the ssl file
		vector_of_the_comparing_strings.push_back("ufm_ssl_file_path");
		vector_of_the_comparing_strings.push_back("ufm_ssl_file_name");

		//elements in the list for finding the input and output file names
		vector_of_the_comparing_strings.push_back("ufm_switch_output_file_name");
		vector_of_the_comparing_strings.push_back("ufm_switch_input_file_name");

		//Boolean values to see if we find all data we expect to see.
		//It is possible someone points to a bad config file missing expected fields. 
		//As we find vaild fields we will "turn on" these booleans, proving we found the data
		//If there are booleans that are still off after the file has been parsed, then print errors. 
		bool config_inventory = false;                     // main section of inventory in JSON
		bool config_inventory_csm_inv_log_dir = false;     // sub field of `inventory` called 'csm_inv_log_dir'
		bool config_inventory_ufm = false;                 // sub section of 'inventory' called 'ufm' 
		bool config_inventory_ufm_ib_cable_errors = false; // sub field of 'ufm' called 'ib_cable_errors'
		bool config_inventory_ufm_switch_errors = false;   // sub field of 'ufm' called 'switch_errors'
		//bool config_inventory_ufm_ufm_counters = false;    // sub field of 'ufm' called 'ufm_counters'
		bool config_inventory_ufm_ufm_ssl_file_path = false;    // sub field of 'ufm' called 'ufm_ssl_file_path'
		bool config_inventory_ufm_ufm_ssl_file_name = false;    // sub field of 'ufm' called 'ufm_ssl_file_name'
		bool config_inventory_ufm_ufm_switch_output_file_name = false; // sub field of 'ufm' called 'ufm_switch_output_file_name'
		bool config_inventory_ufm_ufm_switch_input_file_name = false;   // sub field of 'ufm' called 'ufm_switch_input_file_name'
		
		// reading the input file lines
		//std::cout << "Reading master config file" << std::endl;
		std::string line;
		while (std::getline(input_file, line))
		{
			// reading the line
			std::istringstream iss(line);
			//std::cout << "line: " << line << std::endl;
			
			// cycling of the cpmaring strings
			for (unsigned int i = 0; i < vector_of_the_comparing_strings.size(); i++)
			{
				// reading comarping string
				std::string comparing_string = vector_of_the_comparing_strings[i];
				//std::cout << "comparing string: " << comparing_string << std::endl;
		
				// checking if comparing string is contained in the read line
				if ( line.find(comparing_string) != std::string::npos )
				{
					 // printing
					 //std::cout << "the line contain " << comparing_string << std::endl;

					if(i != 6 && i != 7)
					{
                        // extraction field
					    position_delimiter=line.find(":");
					    //std::cout << "position delimiter: " << position_delimiter << std::endl;
					    line.erase(0,position_delimiter);
					    //std::cout << "line after the modification: " << line << std::endl;
					    position_delimiter=line.find("\"");
					    //std::cout << "position delimiter: " << position_delimiter << std::endl;
					    line.erase(0,position_delimiter+1);
					    //std::cout << "line after the modification: " << line << std::endl;
					    position_delimiter=line.find("\"");
					    //std::cout << "position delimiter: " << position_delimiter << std::endl;
					    line.erase(position_delimiter,line.size());
					}

					 

					// updating variables
					switch (i)
					{
						case 0:
							rest_address = line;
							//std::cout << "updating rest_address: " << rest_address << std::endl;
							break;
						case 1:
							rest_username = line;
							//std::cout << "updating rest_username " << rest_username << std::endl;
							break;
						case 2:
							rest_password = line;
							//std::cout << "updating rest_password " << rest_password << std::endl;
							break;
						case 3:
							csm_inv_log_dir = line;
							config_inventory_csm_inv_log_dir = true;
							break;
						case 4:
							ib_cable_errors = line;
							config_inventory_ufm_ib_cable_errors = true;
							break;
						case 5:
							switch_errors = line;
							config_inventory_ufm_switch_errors = true;
							break;
						case 6:
						    // 'inventory' field was found.
						    // update the logging bool
                            config_inventory = true; 
						    break;
						case 7:
						    // 'ufm' field was found.
						    // update the logging bool
                            config_inventory_ufm = true; 
						    break;
						case 8:
							ssl_file_path = line;
							config_inventory_ufm_ufm_ssl_file_path = true;
							break;
						case 9:
							ssl_file_name = line;
							config_inventory_ufm_ufm_ssl_file_name = true;
							break;
						case 10:
							ufm_switch_output_file_name = line;
							config_inventory_ufm_ufm_switch_output_file_name = true;
							break;
						case 11:
							if(custom_input_override)
							{
								ufm_switch_input_file_name = override_path_and_filename;
							}else{
								ufm_switch_input_file_name = line;
								config_inventory_ufm_ufm_switch_input_file_name = true;
							}
							break;
					}
				}
			}
		}

		// Are any of these booleans still false?
		if( config_inventory == false || 
			config_inventory_csm_inv_log_dir == false || 
			config_inventory_ufm == false ||
			config_inventory_ufm_ib_cable_errors == false || 
			config_inventory_ufm_switch_errors == false ||
			config_inventory_ufm_ufm_ssl_file_path == false ||
			config_inventory_ufm_ufm_ssl_file_name == false ||
			config_inventory_ufm_ufm_switch_output_file_name == false ||
			config_inventory_ufm_ufm_switch_input_file_name == false
		)
		{
			std::cout << std::endl;
			std::cout << "========" << std::endl;
            std::cout << "WARNING: Missing expected key values in config file." << std::endl;
            std::cout << "--------" << std::endl;
            std::cout << "Details:" << std::endl;
            if(config_inventory == false)
            {
            	std::cout << "Missing 'inventory' section. " << std::endl;
            }
            if(config_inventory_csm_inv_log_dir == false)
            {
            	std::cout << "Missing 'csm_inv_log_dir' field. Setting this to a default vaule of: \"/var/log/ibm/csm/inv\"" << std::endl;
            	csm_inv_log_dir = "/var/log/ibm/csm/inv";
            }
            if(config_inventory_ufm == false)
            {
            	std::cout << "Missing 'ufm' section. " << std::endl;
            }
            if(config_inventory_ufm_ib_cable_errors == false)
            {
            	std::cout << "Missing 'ib_cable_errors' field. Setting this to a default vaule of: \"bad_ib_cable_records.txt\"" << std::endl;
            	ib_cable_errors = "bad_ib_cable_records.txt";
            }
            if(config_inventory_ufm_switch_errors == false)
            {
            	std::cout << "Missing 'switch_errors' field. Setting this to a default vaule of: \"bad_switch_records.txt\"" << std::endl;
            	switch_errors = "bad_switch_records.txt";
            }
            if(config_inventory_ufm_ufm_ssl_file_path == false)
            {
            	std::cout << "Missing 'ssl_file_path' field. Setting this to a default vaule of: \"/etc/ibm/csm\"" << std::endl;
            	ssl_file_path = "/etc/ibm/csm";
            }
            if(config_inventory_ufm_ufm_ssl_file_name == false)
            {
            	std::cout << "Missing 'ssl_file_name' field. Setting this to a default vaule of: \"csm_ufm_ssl_key.txt\"" << std::endl;
            	ssl_file_name = "csm_ufm_ssl_key.txt";
            }
            if(config_inventory_ufm_ufm_switch_output_file_name == false)
            {
            	std::cout << "Missing 'ufm_switch_output_file_name' field. Setting this to a default vaule of: \"ufm_switch_output_file.json\"" << std::endl;
            	ufm_switch_output_file_name = "ufm_switch_output_file.json";
            }
            if(config_inventory_ufm_ufm_switch_input_file_name == false)
            {
            	//only set default if the user hasn't overriden value.
            	if(custom_input_override == false)
            	{
            		std::cout << "Missing 'ufm_switch_input_file_name' field. Setting this to a default vaule of: \"ufm_switch_output_file.json\"" << std::endl;
            		ufm_switch_input_file_name = "ufm_switch_output_file.json";
            	}
            }
            std::cout << "========" << std::endl;
            std::cout << std::endl;
            successful_with_errors = true;
		}
	}

	// closing master config file
	input_file.close();

	// rest variables
	//std::cout << "rest_address  " << rest_address  << std::endl;
	//std::cout << "rest_username " << rest_username << std::endl;
	//std::cout << "rest_password " << rest_password << std::endl;

	// check on the type of collection
	if ( ( type_of_collection < 1 ) || ( type_of_collection > 3 ) )
	{
		// printing
		std::cout << "Type of collection is smaller than 1 or greater than 3" << std::endl;
		std::cout << "You need to use an integer number between 1 and 3 included" << std::endl;
		std::cout << "Return without executing data collection" << std::endl;
		
		// return
		return 1;
	}

	// name of the temp file
	ssl_file_path = "/etc/ibm/csm";
	ssl_file_name = "csm_ufm_ssl_key.txt";
	std::string ssl_full_path = ssl_file_path + "/" + ssl_file_name;
	
    // printing
	if(details)
	{
		std::cout << "full path to the ssl key file: " << ssl_full_path << std::endl;
	}
	
    // ToDo: 
    // Store the username and password in a ssl key in a file
    // Eventually we will require that the sys admin create a file with an ssl key inside of it. 
    // to prep for that we save the ssl key to a file today
    // we then read from that file
    // pipeline is in place to make that switch.

	//for old temp file. leave for now
	// generating authentication string for the HTTP request
	//std::string command = "openssl base64 -e <<< " + rest_username+ ":" + rest_password + " > " + temp_file_name;
	//system(command.c_str());
	
	// declaring the authentication string for the http request
	std::string authentication_string_for_the_http_request;
	
	// opening temp file
	std::ifstream ssl_file(ssl_full_path.c_str(),std::ios::in);

	// checking if temp file is open
	if ( ! ssl_file.is_open() )
	{
		// printing
		std::cout << "SSL file " << ssl_full_path << " not open, return"  << std::endl;
		std::cout << "Return without executing data collection" << std::endl;
		
		// return error
		return 1;
	} 
	else 
	{
		// getting and modfifying the authentication string for the http request
		ssl_file >> authentication_string_for_the_http_request;
		//std::cout << "before modifications authentication_string_for_the_http_request " << authentication_string_for_the_http_request << std::endl;
		authentication_string_for_the_http_request = authentication_string_for_the_http_request.substr(0,authentication_string_for_the_http_request.size()-4);
		//std::cout << "after modifications authentication_string_for_the_http_request " << authentication_string_for_the_http_request << std::endl;
	}

	if( authentication_string_for_the_http_request[0] == '\0')
	{
		std::cout << "WARNING: SSL file provided is blank. Please create an SSL key and save it in this file." << std::endl;
		std::cout << "Return without executing data collection" << std::endl;
		
		// return error
		return 1;
	}

	// closing temp output file
	ssl_file.close();
	std::cout << std::endl;
	
	//old temp file - leave for now
	// cleaning
	//command = "rm " + ssl_file_name;
	//system(command.c_str());
	
	// gathering flags
	int ib_flag = INV_IB_CONNECTOR_ACCESS::GetInstance()->GetCompiledWithSupport();
	
	// gathering flags
	int switch_flag = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->GetCompiledWithSupport();

	// printing
	if(details)
	{
		std::cout << "Configuration successful." << std::endl;
		std::cout << "Connecting to UFM to collect inventory..." << std::endl;
		std::cout << std::endl;
	}

	// checking type of collection
	if ( type_of_collection == 1 )
	{
		// checking ib flag
		if ( ib_flag == 0 )
		{
			// printing
			std::cout << "compiled without ib support, no ib collection" << std::endl;
		} 
		else 
		{
			int returnCode = 0;
			// execute data collection for the ib cables
			returnCode = INV_IB_CONNECTOR_ACCESS::GetInstance()->ExecuteDataCollection(rest_address,authentication_string_for_the_http_request, csm_inv_log_dir, ib_cable_errors);
			if(returnCode == 1)
			{
				// return error
				std::cout << "INV_IB_CONNECTOR_ACCESS failed" << std::endl;
				return 1;
			}
		}
	} 

	// checking type of collection
	if ( type_of_collection == 2 )
	{
		// checking switch flag
		if ( switch_flag == 0 )
		{
			// printing
			std::cout << "compiled without switch support, no switch collection" << std::endl;
		} 
		else 
		{
			int returnCode = 0;
			// execute data collection for the switch cables
			returnCode = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ExecuteDataCollection(rest_address,authentication_string_for_the_http_request, csm_inv_log_dir, switch_errors, custom_input_override, ufm_switch_output_file_name, ufm_switch_input_file_name);
			if(returnCode == 1)
			{
				// return error
				std::cout << "INV_SWITCH_CONNECTOR_ACCESS failed" << std::endl;
				return 1;
			}
		}
	}

	// checking type of collection
	if ( type_of_collection == 3 )
	{
		// checking ib flag
		if ( ib_flag == 0 )
		{
			// printing
			std::cout << "compiled without ib support, no ib collection" << std::endl;
		} 
		else 
		{
			int returnCode = 0;
			// execute data collection for the ib cables
			returnCode = INV_IB_CONNECTOR_ACCESS::GetInstance()->ExecuteDataCollection(rest_address,authentication_string_for_the_http_request, csm_inv_log_dir, ib_cable_errors);
			if(returnCode == 1)
			{
				// return error
				std::cout << "INV_IB_CONNECTOR_ACCESS failed" << std::endl;
				return 1;
			}
		}

		// checking switch flag
		if ( switch_flag == 0 )
		{
			// printing
			std::cout << "compiled without switch support, no switch collection" << std::endl;
		}
		else
		{
			int returnCode = 0;
			// execute data collection for the switch cables
			returnCode = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ExecuteDataCollection(rest_address,authentication_string_for_the_http_request, csm_inv_log_dir, switch_errors, custom_input_override, ufm_switch_output_file_name, ufm_switch_input_file_name);
			if(returnCode == 1)
			{
				// return error
				std::cout << "INV_SWITCH_CONNECTOR_ACCESS failed" << std::endl;
				return 1;
			}
		}
	}

	// test for the ib and switch field values
	std::string ib_field_value;
	std::string switch_field_value;

	totalIBRecords = INV_IB_CONNECTOR_ACCESS::GetInstance()->TotalNumberOfRecords();
	totalSwitchRecords = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->TotalNumberOfRecords();
	
	// printing
	if(details)
	{
		std::cout << "UFM collection successful." << std::endl;
		std::cout << "CSM has approved the following records for insertion into the CSM Database: " << std::endl;
		std::cout << "IB Records: " << totalIBRecords << std::endl;
	    std::cout << "Switch Records: " << totalSwitchRecords << std::endl;
	    std::cout << std::endl;
	}
	
	// printing
	if(details)
	{
		std::cout << "Converting UFM data into CSM structs." << std::endl;
	    std::cout << std::endl;
	}
	
	
	// IB
	// Set up data to call API
	csm_ib_cable_inventory_collection_input_t* IBinput = NULL;
	// CSM API initalise and malloc function
	csm_init_struct_ptr(csm_ib_cable_inventory_collection_input_t, IBinput);
	csm_ib_cable_inventory_collection_output_t* IBoutput = NULL;
	
	// SWITCH
	// Set up data to call API
	csm_switch_inventory_collection_input_t* SWITCHinput = NULL;
	// CSM API initalise and malloc function
	csm_init_struct_ptr(csm_switch_inventory_collection_input_t, SWITCHinput);
	csm_switch_inventory_collection_output_t* SWITCHoutput = NULL;
	
	// Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()
	return_value = csm_init_lib();
    if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initalization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
		csm_free_struct_ptr(csm_switch_inventory_collection_input_t, SWITCHinput);
		csm_free_struct_ptr(csm_ib_cable_inventory_collection_input_t, IBinput);
		std::cout << "The library and the daemon are not able to communicate" << std::endl;
		std::cout << "Return without executing data collection" << std::endl;
		return return_value;
    }
	
    // checking on the total number of IB records
    if ( totalIBRecords > 0 )
    {
		/*
		// IB
		// Set up data to call API
		csm_ib_cable_inventory_collection_input_t* IBinput = NULL;
		// CSM API initalise and malloc function
		csm_init_struct_ptr(csm_ib_cable_inventory_collection_input_t, IBinput);
		csm_ib_cable_inventory_collection_output_t* IBoutput = NULL;
		*/
	
		// number of records to insert.
		IBinput->inventory_count = totalIBRecords;
		// create memory for it
		IBinput->inventory = (csmi_ib_cable_record_t**)calloc(IBinput->inventory_count, sizeof(csmi_ib_cable_record_t*));
	
		// loop through copy
		// will most likely be something like
		// IBinput->inventory[i]->value = strdup(vector[i]->value);
		std::string temp_string;
        for(uint32_t i = 0; i < IBinput->inventory_count; i++){
            //malloc space for record
            csm_init_struct_ptr(csmi_ib_cable_record_t, IBinput->inventory[i]);

            // actual part
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(2,i); 	IBinput->inventory[i]->serial_number   = strdup(temp_string.c_str());
																							IBinput->inventory[i]->discovery_time  = strdup("NULL");
																							IBinput->inventory[i]->collection_time = strdup("NULL");
																							IBinput->inventory[i]->comment         = strdup("comment");
			// Convert Mellanox guid to use CSM standardized guid format
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(7,i); 
            temp_string = standardizeGuid(temp_string);
            IBinput->inventory[i]->guid_s1         = strdup(temp_string.c_str());
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(11,i); 	
            temp_string = standardizeGuid(temp_string);
            IBinput->inventory[i]->guid_s2         = strdup(temp_string.c_str());

			temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(3,i); 	IBinput->inventory[i]->identifier      = strdup(temp_string.c_str());																				
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(1,i); 	IBinput->inventory[i]->length          = strdup(temp_string.c_str());
			temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(12,i); 	IBinput->inventory[i]->name            = strdup(temp_string.c_str());
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(0,i); 	IBinput->inventory[i]->part_number     = strdup(temp_string.c_str());
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(9,i); 	IBinput->inventory[i]->port_s1         = strdup(temp_string.c_str());
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(10,i); 	IBinput->inventory[i]->port_s2         = strdup(temp_string.c_str());
			temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(5,i); 	IBinput->inventory[i]->revision        = strdup(temp_string.c_str());
			temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(6,i); 	IBinput->inventory[i]->severity        = strdup(temp_string.c_str());
            temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(4,i); 	IBinput->inventory[i]->type            = strdup(temp_string.c_str());
			temp_string = INV_IB_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(8,i); 	IBinput->inventory[i]->width           = strdup(temp_string.c_str());
		}

		// printing
		if(details)
		{
			std::cout << "IB cable data successfully converted into CSM structs." << std::endl;
			std::cout << "Inserting into CSM Database..." << std::endl;
		    std::cout << std::endl;
		}

        // Call the CSM API
        return_value = csm_ib_cable_inventory_collection(&csm_obj, IBinput, &IBoutput);

        switch( return_value )
        {
			case CSMI_SUCCESS:
				printf( "---\n# ib inventory successful\n...\n");
				break;
			case CSMERR_UPDATE_MISMATCH:
				printf("---\n");
				printf("...\n");
				break;
			default:
				printf("%s FAILED: errcode: %d errmsg: %s\n",argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
        }
		
		//prevent a reading from output if API fails
		if(return_value == CSMI_SUCCESS)
		{
			std::cout << "# total ib inventory collected: " << IBinput->inventory_count << std::endl;
			std::cout << "# new ib records inserted into database: " << IBoutput->insert_count << std::endl;
			std::cout << "# old ib records updated in database: " << IBoutput->update_count << std::endl;
			std::cout << "# old ib records removed from the database: " << IBoutput->delete_count << std::endl;
			
			if(((unsigned)IBoutput->insert_count + IBoutput->update_count) != IBinput->inventory_count){
				std::cout <<  "# WARNING: inserted records and updated records do not match total inventory collected."  << std::endl;
				std::cout <<  "# records dropped: " << IBinput->inventory_count - IBoutput->insert_count - IBoutput->update_count << std::endl;
			}
		}
		
		
		

        // Use CSM API free to release arguments. We no longer need them.
        csm_free_struct_ptr(csm_ib_cable_inventory_collection_input_t, IBinput);
        // Call internal CSM API clean up.
        csm_api_object_destroy(csm_obj);

	}

	

	
    // checking on the total number of SWITCH records
    if ( totalSwitchRecords > 0 )
    {
		/*
		// SWITCH
		// Set up data to call API
		csm_switch_inventory_collection_input_t* SWITCHinput = NULL;
		// CSM API initalise and malloc function
		csm_init_struct_ptr(csm_switch_inventory_collection_input_t, SWITCHinput);
		csm_switch_inventory_collection_output_t* SWITCHoutput = NULL;
		*/

		//rolling inventory counter
		// number of modules may not be the same on each switch.
        // used to later adjust/offset a search index accordingly 
        // probably a better solution.
        // use this for now until we switch over to json reader tree.
		uint32_t rolling_total_module_counter = 0;

		// number of records to insert.
		SWITCHinput->inventory_count = totalSwitchRecords;
		// create memory for it
		SWITCHinput->inventory = (csmi_switch_details_t**)calloc(SWITCHinput->inventory_count, sizeof(csmi_switch_details_t*));

		// loop through copy
		// will most likely be something like
		// SWITCHinput->inventory[i]->value = strdup(vector[i]->value);
		std::string temp_string;
		for(uint32_t i = 0; i < SWITCHinput->inventory_count; i++){
			
			/*Helper Variables*/
			char* pEnd;
			
            //malloc space for record
            csm_init_struct_ptr(csmi_switch_details_t, SWITCHinput->inventory[i]);

			//malloc space for base switch record
            csm_init_struct_ptr(csmi_switch_record_t, SWITCHinput->inventory[i]->switch_data);

            // copy data from switch connector into API struct
            temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(0,i);  SWITCHinput->inventory[i]->switch_data->switch_name             = strdup(temp_string.c_str());
																							  SWITCHinput->inventory[i]->switch_data->serial_number           = strdup("N/A");
                                                                                              SWITCHinput->inventory[i]->switch_data->discovery_time          = strdup("N/A");
                                                                                              SWITCHinput->inventory[i]->switch_data->collection_time         = strdup("N/A");																								 
                                                                                              SWITCHinput->inventory[i]->switch_data->comment                 = strdup("N/A");
            temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(1,i);  SWITCHinput->inventory[i]->switch_data->description             = strdup(temp_string.c_str());																								 
            temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(2,i);  SWITCHinput->inventory[i]->switch_data->fw_version              = strdup(temp_string.c_str());

            // Convert Mellanox guid to use CSM standardized guid format
            temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(3,i); 
            temp_string = standardizeGuid(temp_string); 
            SWITCHinput->inventory[i]->switch_data->gu_id = strdup(temp_string.c_str());
			
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(4,i); 
			if(strcmp(temp_string.c_str(),"true") == 0)
			{
				SWITCHinput->inventory[i]->switch_data->has_ufm_agent = CSM_TRUE;
			}
			else
			{
				//default to false? -- not technically accurate to the original data. 
				SWITCHinput->inventory[i]->switch_data->has_ufm_agent = CSM_FALSE;
			}
			//eventually implement something better, when have the time.
			/* if(strcmp(temp_string.c_str(),"true") == 0 || strcmp(temp_string.c_str(),"false") == 0 )
			{
				SWITCHinput->inventory[i]->switch_data->has_ufm_agent = 1;
			}
			else
			{
				//place holder for a future error case?
				SWITCHinput->inventory[i]->switch_data->has_ufm_agent = 1;
			} */
			
			//UFM promised to give us this field in a future restAPI release.
			SWITCHinput->inventory[i]->switch_data->hw_version = strdup("N/A");
			//continue with actual data.
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(5,i);  SWITCHinput->inventory[i]->switch_data->ip                      = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(6,i);  SWITCHinput->inventory[i]->switch_data->model                   = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(7,i);  SWITCHinput->inventory[i]->switch_data->num_modules             = std::strtol(temp_string.c_str(), &pEnd, 10); if(pEnd == temp_string.c_str()) SWITCHinput->inventory[i]->switch_data->num_modules  = -1.0; 
			
			
			//switch modules inventory begin
			uint32_t j = 0; // module
			//set number of inventory pieces (modules) on a switch
			if(SWITCHinput->inventory[i]->switch_data->num_modules == -1)
			{
				SWITCHinput->inventory[i]->inventory_count = 0;
			}
			else
			{
				SWITCHinput->inventory[i]->inventory_count = SWITCHinput->inventory[i]->switch_data->num_modules;
				
			}
			
			
			//allocate space for the array
			SWITCHinput->inventory[i]->inventory = (csmi_switch_inventory_record_t**)calloc(SWITCHinput->inventory[i]->inventory_count, sizeof(csmi_switch_inventory_record_t*));
			
			//loop through modules inventory on this switch
			for(j = 0; j < SWITCHinput->inventory[i]->inventory_count; j++){
				bool isSystem = false;
				//malloc space for switch inventory record
				csm_init_struct_ptr(csmi_switch_inventory_record_t, SWITCHinput->inventory[i]->inventory[j]);

                // number of modules may not be the same on each switch.
                // adjust search index accordingly 
				unsigned long int adjusted_index = rolling_total_module_counter + j;
				
				// individual module fields  
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("status", adjusted_index);          SWITCHinput->inventory[i]->inventory[j]->status           = strdup(temp_string.c_str());
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("hw_version", adjusted_index);      SWITCHinput->inventory[i]->inventory[j]->hw_version       = strdup(temp_string.c_str());
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("name", adjusted_index);            SWITCHinput->inventory[i]->inventory[j]->name             = strdup(temp_string.c_str());
				                                                                                                                      SWITCHinput->inventory[i]->inventory[j]->host_system_guid = strdup(SWITCHinput->inventory[i]->switch_data->gu_id);
				
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("number_of_chips", adjusted_index); SWITCHinput->inventory[i]->inventory[j]->number_of_chips  = std::strtol(temp_string.c_str(), &pEnd, 10); if(pEnd == temp_string.c_str()) SWITCHinput->inventory[i]->inventory[j]->number_of_chips  = -1.0;

				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("description", adjusted_index);     SWITCHinput->inventory[i]->inventory[j]->description      = strdup(temp_string.c_str()); 
				
				//check for special "system" which is a UFM hack to give us main serial number
				if(strcmp(SWITCHinput->inventory[i]->inventory[j]->description, "system") == 0 || strcmp(SWITCHinput->inventory[i]->inventory[j]->description, "SYSTEM") == 0)
				{
					isSystem = true;
				}

				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("max_ib_ports", adjusted_index);    SWITCHinput->inventory[i]->inventory[j]->max_ib_ports     = std::strtol(temp_string.c_str(), &pEnd, 10); if(pEnd == temp_string.c_str()) SWITCHinput->inventory[i]->inventory[j]->max_ib_ports  = -1.0; 
				//fw version , can go here
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("module_index", adjusted_index);    SWITCHinput->inventory[i]->inventory[j]->module_index     = std::strtol(temp_string.c_str(), &pEnd, 10); if(pEnd == temp_string.c_str()) SWITCHinput->inventory[i]->inventory[j]->module_index  = -1.0;
				//temperature we aren't gathering.
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("device_type", adjusted_index);     SWITCHinput->inventory[i]->inventory[j]->device_type      = strdup(temp_string.c_str());
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("serial_number", adjusted_index);   SWITCHinput->inventory[i]->inventory[j]->serial_number    = strdup(temp_string.c_str());  
				//if this is the system module then copy the serial number back to the main switch record. 
				//For now, we decided to keep this "hack" module in the CSM database.
				//maybe in the future, we may decided to not record the "system" module
				if(isSystem == true)
				{
					if(SWITCHinput->inventory[i]->switch_data->serial_number != NULL)
					{
						free(SWITCHinput->inventory[i]->switch_data->serial_number);
					}
					SWITCHinput->inventory[i]->switch_data->serial_number = strdup(SWITCHinput->inventory[i]->inventory[j]->serial_number); 
				}
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("path", adjusted_index);             SWITCHinput->inventory[i]->inventory[j]->path             = strdup(temp_string.c_str()); 
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("device_name", adjusted_index);      SWITCHinput->inventory[i]->inventory[j]->device_name      = strdup(temp_string.c_str());
				//type field
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("severity", adjusted_index);         SWITCHinput->inventory[i]->inventory[j]->severity         = strdup(temp_string.c_str());
				                                                                                                          SWITCHinput->inventory[i]->inventory[j]->discovery_time   = strdup(strdup("N/A"));                                      
				                                                                                                          SWITCHinput->inventory[i]->inventory[j]->collection_time  = strdup(strdup("N/A"));                                      
				                                                                                                          SWITCHinput->inventory[i]->inventory[j]->comment          = strdup(strdup("N/A"));   
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("type", adjusted_index);             SWITCHinput->inventory[i]->inventory[j]->type             = strdup(temp_string.c_str()); 
				temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue_module("fw_version", adjusted_index);       SWITCHinput->inventory[i]->inventory[j]->fw_version       = strdup(temp_string.c_str());                               
			}
			//resume base switch information
			                                                                                 
                                                                                              SWITCHinput->inventory[i]->switch_data->physical_frame_location = strdup("WARNING - SYSTEM ADMINISTRATOR MUST SET THIS FIELD AT LEAST ONCE VIA CSM API switch_attributes_update.");
                                                                                              SWITCHinput->inventory[i]->switch_data->physical_u_location     = strdup("WARNING - SYSTEM ADMINISTRATOR MUST SET THIS FIELD AT LEAST ONCE VIA CSM API switch_attributes_update.");
            temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(11,i); SWITCHinput->inventory[i]->switch_data->ps_id                   = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(12,i); SWITCHinput->inventory[i]->switch_data->role                    = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(13,i); SWITCHinput->inventory[i]->switch_data->server_operation_mode   = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(14,i); SWITCHinput->inventory[i]->switch_data->sm_mode                 = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(15,i); SWITCHinput->inventory[i]->switch_data->state                   = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(16,i); SWITCHinput->inventory[i]->switch_data->sw_version              = strdup(temp_string.c_str());

			// Convert Mellanox guid to use CSM standardized guid format
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(17,i); 
			temp_string = standardizeGuid(temp_string);
			SWITCHinput->inventory[i]->switch_data->system_guid = strdup(temp_string.c_str());

			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(18,i); SWITCHinput->inventory[i]->switch_data->system_name             = strdup(temp_string.c_str());
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(19,i); SWITCHinput->inventory[i]->switch_data->total_alarms            = std::strtol(temp_string.c_str(), &pEnd, 10); if(pEnd == temp_string.c_str()) SWITCHinput->inventory[i]->switch_data->total_alarms  = -1.0; 
			temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(20,i); SWITCHinput->inventory[i]->switch_data->type                    = strdup(temp_string.c_str());
            temp_string = INV_SWITCH_CONNECTOR_ACCESS::GetInstance()->ReturnFieldValue(21,i); SWITCHinput->inventory[i]->switch_data->vendor                  = strdup(temp_string.c_str());


            // temp fix? 
			// after this inventory has been constructed
			// keep track of where we are in the total module list.
			// oh god this is awful
			rolling_total_module_counter = rolling_total_module_counter + SWITCHinput->inventory[i]->inventory_count;
		}

		// Call the CSM API
		return_value = csm_switch_inventory_collection(&csm_obj, SWITCHinput, &SWITCHoutput);

		switch( return_value )
		{
			case CSMI_SUCCESS:
				printf( "---\n# switch inventory successful\n...\n");
				break;
			case CSMERR_UPDATE_MISMATCH:
				printf("---\n");
				printf("...\n");
				break;
			default:
				printf("%s FAILED: errcode: %d errmsg: %s\n",argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
		}

		//prevent a reading from output if API fails
		if(return_value == CSMI_SUCCESS)
		{
            std::cout << "# total switch inventory collected: " << SWITCHinput->inventory_count << std::endl;
			std::cout << "# new switch records inserted into database: " << SWITCHoutput->insert_count << std::endl;
			std::cout << "# old switch records updated in database: " << SWITCHoutput->update_count << std::endl;
			std::cout << "# old switch records removed from the database: " << SWITCHoutput->delete_count << std::endl;
			std::cout << "# old switch module records removed from the database because they were associated with the above removed switches: " << SWITCHoutput->delete_module_count << std::endl;
			
			if(((unsigned)SWITCHoutput->insert_count + SWITCHoutput->update_count) != SWITCHinput->inventory_count){
				std::cout <<  "# WARNING: inserted records and updated records do not match total inventory collected."  << std::endl;
				std::cout <<  "# records dropped: " << SWITCHinput->inventory_count - SWITCHoutput->insert_count - SWITCHoutput->update_count << std::endl;
			}
		}

		// Use CSM API free to release arguments. We no longer need them.
		//csm_free_struct_ptr(csm_switch_inventory_collection_input_t, SWITCHinput);
		// Call internal CSM API clean up.
		csm_api_object_destroy(csm_obj);
		
		csm_switch_children_inventory_collection_output_t* SWITCHoutput_children = NULL;
		
		//run temp inventory sub children collection
		return_value = csm_switch_children_inventory_collection(&csm_obj, SWITCHinput, &SWITCHoutput_children);

		switch( return_value )
		{
			case CSMI_SUCCESS:
				printf( "---\n# switch module inventory successful\n...\n");
				break;
			case CSMERR_UPDATE_MISMATCH:
				printf("---\n");
				printf("...\n");
				break;
			default:
				printf("%s FAILED: errcode: %d errmsg: %s\n",argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
		}

		//prevent a reading from output if API fails
		if(return_value == CSMI_SUCCESS)
		{
			//Get the total number of switch modules.
			//Go through each switch and count their modules. 
            uint32_t total_modules = 0;
            for(uint32_t i = 0; i < SWITCHinput->inventory_count; i++)
            {
            	//switch modules inventory begin
			    uint32_t j = 0; // module
                //loop through modules inventory on this switch
			    for(j = 0; j < SWITCHinput->inventory[i]->inventory_count; j++){
                    total_modules++;
                }
            }


			std::cout << "# total switch module inventory collected: " << total_modules << std::endl;
			std::cout << "# new switch module records inserted into database: " << SWITCHoutput_children->insert_count << std::endl;
			std::cout << "# old switch module records updated in database: " << SWITCHoutput_children->update_count << std::endl;
			std::cout << "# old switch module records removed from the database: " << SWITCHoutput_children->delete_count << std::endl;
			
			if(((unsigned)SWITCHoutput_children->insert_count + SWITCHoutput_children->update_count) != total_modules){
				std::cout <<  "# WARNING: inserted records and updated records do not match total inventory collected."  << std::endl;
				std::cout <<  "# records dropped: " << total_modules - SWITCHoutput_children->insert_count - SWITCHoutput_children->update_count << std::endl;
			}
		}
		
		// Use CSM API free to release arguments. We no longer need them.
		csm_free_struct_ptr(csm_switch_inventory_collection_input_t, SWITCHinput);
		// Call internal CSM API clean up.
		csm_api_object_destroy(csm_obj);
    }

    // Cleanup the library and print the error.
    int lib_return_value = csm_term_lib();
    if( lib_return_value != 0 )
    {
		csmutil_logging(error, "csm_term_lib rc= %d, Initalization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", lib_return_value);
		return lib_return_value;
    }

    if(successful_with_errors)
    {
    	std::cout << "\n WARNING: Program completed successfully, but with some unexpected errors. Scroll up to view the output log. \n" << std::endl;
    }

    return return_value;

	// return success
	return 0;
}
