/*================================================================================

    csmi/src/inv/cmd/switch_attributes_query.c

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota
* Email: nbuonar@us.ibm.com
*/
/*C Include*/
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
/*CORAL includes*/
#include "utilities/include/string_tools.h"
/*CSM Include*/
#include "csmi/include/csm_api_inventory.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"
#include "csmi/src/inv/include/csmi_inv_internal.h"

#define API_PARAMETER_INPUT_TYPE csm_switch_attributes_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_switch_attributes_query_output_t

#define API_FORMAT_START "results_count,results{"
#define API_FORMAT_STRING "switch_name,serial_number"
#define API_FORMAT_END   "}"

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(csm_switch_attributes_query_input_t, input); help

void help(){
	puts("_____CSM_SWITCH_ATTRIBUTES_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_switch_attributes_query ARGUMENTS [OPTIONS]");
	puts("  csm_switch_attributes_query [-r roles] [-s switch_names] [-S serial_number] [-t state] [-l limit] [-o offset] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_switch' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_switch_attributes_query can have 3 optional arguments");
	puts("    Argument            | Example value         | Description  ");                                                 
	puts("    --------------------|-----------------------|--------------");
	puts("    -r, --roles         | \"tor,core\"            | (STRING) This is a csv field of roles to query. Filter results to only include records that have a matching role.");
	puts("    -s, --switch_names  | \"switch_01,switch_02\" | (STRING) This is a csv field of switch names to query. Filter results to only include records that have a matching switch name. The switch name is a unique identification for a switch.");
	puts("    -S, --serial_number | \"abc123\"              | (STRING) Filter results to only include records that have a matching serial number.");
	puts("    -t, --state         | \"active\"              | (STRING) Filter results to only include records that have a matching state.");
	puts("                        |                       | Valid values according to Mellanox: \"active\", \"rebooting\", \"down\", or \"error\"");
	puts("                        |                       | ");
	puts("  FILTERS:");
	puts("    csm_switch_attributes_query can have 2 optional filters.");
	puts("    Argument       | Example value | Description  ");                                                 
	puts("    ---------------|---------------|--------------");
	puts("    -l, --limit    | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset   | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("    -O, --order_by | a             | (CHAR) SQL 'ORDER BY' numeric value. Default Value: 'a'");
	puts("                                   | Valid Values: [a] = 'ORDER BY switch_name ASC NULLS LAST'"); 
	puts("                                   |               [b] = 'ORDER BY switch_name DESC NULLS LAST'");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("    -f, --format | allocation_id,compute_nodes | (STRING) Format string detailing the fields to display.");
    puts("                                   Values are comma delimited, matching the contents of the struct.");
    puts("                                   If a result has a subresult (e.g. struct pointer) those elements");
    puts("                                   may be indicated by <subresult_field>\"{\"<formatting>\"}\".");
    puts("                                   \"%\" Indicates all values should be shown for that result level and deeper.");
	puts("[-h, --help]                  | Help.");
	puts("[-J, --JSON]                  | Set output to print in JSON. ");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");

	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_switch_attributes_query -s \"abc123\"");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	{"format",        required_argument, 0, 'f'},
	{"JSON",          no_argument,       0, 'J'},
	//api arguments
	{"role",          required_argument, 0, 'r'},
	{"switch_name",   required_argument, 0, 's'},
	{"serial_number", required_argument, 0, 'S'},
	{"state",         required_argument, 0, 't'},
	//filters
	{"limit",         required_argument, 0, 'l'},
	{"offset",        required_argument, 0, 'o'},
	{"order_by",      required_argument, 0, 'O'},
	{0,0,0,0}
};

/*
* Summary: Simple command line interface for the CSM API 'node attributes update'. Takes in information for a single node attributes update (nau) struct via cmd line parameters.
*/
int main(int argc, char *argv[])
{	
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 0;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	uint32_t i = 0;
	/*For format printing later. */
	char JSON = 0;
	char *format = NULL;
	char format_output = 0;

	/*Set up data to call API*/
	csm_switch_attributes_query_input_t* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(csm_switch_attributes_query_input_t, input);
	csm_switch_attributes_query_output_t* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:f:Jl:o:O:r:s:S:t:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'f':
			{
				CSM_WRAP_FORMAT_STRING( format, API_FORMAT_START, optarg, API_FORMAT_END );
				format_output = 1;
                break;
			}
			case 'J':
				JSON = 1;
				break;
			case 'l':
				csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
			case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
				break;
			case 'O':
				if(strlen(optarg) == 1 && 
                    (  optarg[0] == 'a' 
					|| optarg[0] == 'b'
					)
				)
                {
					input->order_by = optarg[0];
				}else{
					csmutil_logging(error, "Invalid parameter for -O: optarg , encountered: %s", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				break;
			case 'r':
			{
				csm_optarg_test( "-r, --roles", optarg, USAGE );
				csm_parse_csv( optarg, input->roles, input->roles_count,
                            char*, csm_str_to_char, NULL, "-r, --roles", USAGE );
				optionalParameterCounter++;
				break;
			}
			case 's':
			{
				csm_optarg_test( "-s, --switch_names", optarg, USAGE );
				csm_parse_csv( optarg, input->switch_names, input->switch_names_count,
                            char*, csm_str_to_char, NULL, "-s, --switch_names", USAGE );
				optionalParameterCounter++;
				break;
			}
			case 'S':
                csm_optarg_test( "-S, --serial_number", optarg, USAGE );
				input->serial_number = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 't':
                csm_optarg_test( "-t, --state", optarg, USAGE );
				input->state = strdup(optarg);
				optionalParameterCounter++;
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
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        csm_free_struct_ptr(csm_switch_attributes_query_input_t, input);
		return return_value;           
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:         %p", input);
	csmutil_logging(debug, "  address of input:       %p", &input);
	csmutil_logging(debug, "  csm_switch_attributes_query_input_t contains the following:");
	csmutil_logging(debug, "    limit:       %i", input->limit);
	csmutil_logging(debug, "    offset:      %i", input->offset);
	csmutil_logging(debug, "    roles_count: %i", input->roles_count);
	csmutil_logging(debug, "    roles:       %p", input->roles);
	for(i = 0; i < input->roles_count; i++){
		csmutil_logging(debug, "      roles[%i]: %s", i, input->roles[i]);
	}
	csmutil_logging(debug, "    switch_names_count: %i", input->switch_names_count);
	csmutil_logging(debug, "    switch_names:       %p", input->switch_names);
	for(i = 0; i < input->switch_names_count; i++){
		csmutil_logging(debug, "      switch_names[%i]: %s", i, input->switch_names[i]);
	}
	csmutil_logging(debug, "    state:      %s", input->state);
	csmutil_logging(debug, "  value of output:        %p", output);
	csmutil_logging(debug, "  address of output:      %p", &output);
	csmutil_logging(debug, "  csm_switch_attributes_query_output_t contains the following:");
	
	/* Call the actual CSM API */
	return_value = csm_switch_attributes_query(&csm_obj, input, &output);
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(csm_switch_attributes_query_input_t, input);
	
    switch( return_value )
    {
        case CSMI_SUCCESS:
			if(JSON == 1)
			{
				puts("[");
				for (i = 0; i < output->results_count; i++) {
					printf("    {\n");
					if(format_output == 1)
					{
						//format not support JSON atm
						csmutil_logging(warning, "%s-%d:", __FILE__, __LINE__);
						csmutil_logging(warning, "  format doesn't support JSON combination at the moment.");
						csmutil_logging(warning, "  please contribute to open source and improve this feature.");
					}
					//check if statement for null strings. print out literal null for json standard.
					if(output->results[i]->switch_name[0] == '\0'){ printf("        \"switch_name\": null,\n"); }else{ printf("        \"switch_name\": \"%s\",\n", output->results[i]->switch_name); }
					if(output->results[i]->serial_number[0] == '\0'){ printf("        \"serial_number\": null,\n"); }else{ printf("        \"serial_number\": \"%s\",\n", output->results[i]->serial_number); }
					if(output->results[i]->discovery_time[0] == '\0'){ printf("        \"discovery_time\": null,\n"); }else{ printf("        \"discovery_time\": \"%s\",\n", output->results[i]->discovery_time); }
					if(output->results[i]->collection_time[0] == '\0'){ printf("        \"collection_time\": null,\n"); }else{ printf("        \"collection_time\": \"%s\",\n", output->results[i]->collection_time); }
					if(output->results[i]->comment[0] == '\0'){ printf("        \"comment\": null,\n"); }else{ printf("        \"comment\": \"%s\",\n", output->results[i]->comment); }
					if(output->results[i]->description[0] == '\0'){ printf("        \"description\": null,\n"); }else{ printf("        \"description\": \"%s\",\n", output->results[i]->description); }
					if(output->results[i]->fw_version[0] == '\0'){ printf("        \"fw_version\": null,\n"); }else{ printf("        \"fw_version\": \"%s\",\n", output->results[i]->fw_version); }
					if(output->results[i]->gu_id[0] == '\0'){ printf("        \"gu_id\": null,\n"); }else{ printf("        \"gu_id\": \"%s\",\n", output->results[i]->gu_id); }
					printf("        \"has_ufm_agent\": %c\n", csm_print_bool_custom(output->results[i]->has_ufm_agent,'t','f')); 
					if(output->results[i]->hw_version[0] == '\0'){ printf("        \"hw_version\": null,\n"); }else{ printf("        \"hw_version\": \"%s\",\n", output->results[i]->hw_version); }
					if(output->results[i]->ip[0] == '\0'){ printf("        \"ip\": null,\n"); }else{ printf("        \"ip\": \"%s\",\n", output->results[i]->ip); }
					if(output->results[i]->model[0] == '\0'){ printf("        \"model\": null,\n"); }else{ printf("        \"model\": \"%s\",\n", output->results[i]->model); }
					printf("        \"num_modules\": %"PRId32"\n", output->results[i]->num_modules); 
					if(output->results[i]->physical_frame_location[0] == '\0'){ printf("        \"physical_frame_location\": null,\n"); }else{ printf("        \"physical_frame_location\": \"%s\",\n", output->results[i]->physical_frame_location); }
					if(output->results[i]->physical_u_location[0] == '\0'){ printf("        \"physical_u_location\": null,\n"); }else{ printf("        \"physical_u_location\": \"%s\",\n", output->results[i]->physical_u_location); }
					if(output->results[i]->ps_id[0] == '\0'){ printf("        \"ps_id\": null,\n"); }else{ printf("        \"ps_id\": \"%s\",\n", output->results[i]->ps_id); }
					if(output->results[i]->role[0] == '\0'){ printf("        \"role\": null,\n"); }else{ printf("        \"role\": \"%s\",\n", output->results[i]->role); }
					if(output->results[i]->server_operation_mode[0] == '\0'){ printf("        \"server_operation_mode\": null,\n"); }else{ printf("        \"server_operation_mode\": \"%s\",\n", output->results[i]->server_operation_mode); }
					if(output->results[i]->sm_mode[0] == '\0'){ printf("        \"sm_mode\": null,\n"); }else{ printf("        \"sm_mode\": \"%s\",\n", output->results[i]->sm_mode); }
					if(output->results[i]->state[0] == '\0'){ printf("        \"state\": null,\n"); }else{ printf("        \"state\": \"%s\",\n", output->results[i]->state); }
					if(output->results[i]->sw_version[0] == '\0'){ printf("        \"sw_version\": null,\n"); }else{ printf("        \"sw_version\": \"%s\",\n", output->results[i]->sw_version); }
					if(output->results[i]->system_guid[0] == '\0'){ printf("        \"system_guid\": null,\n"); }else{ printf("        \"system_guid\": \"%s\",\n", output->results[i]->system_guid); }
					if(output->results[i]->system_name[0] == '\0'){ printf("        \"system_name\": null,\n"); }else{ printf("        \"system_name\": \"%s\",\n", output->results[i]->system_name); }
					printf("        \"total_alarms\": %"PRId32"\n", output->results[i]->total_alarms); 
					if(output->results[i]->type[0] == '\0'){ printf("        \"type\": null,\n"); }else{ printf("        \"type\": \"%s\",\n", output->results[i]->type); }
					if(output->results[i]->vendor[0] == '\0'){ printf("        \"vendor\": null\n"); }else{ printf("        \"vendor\": \"%s\"\n", output->results[i]->vendor); }
					//if someone adds a new last field above make sure to update the commas here correctly so the JSON still validates. 
					printf("    }");
					if(i+1 != output->results_count)
					{
						printf(",");
					}
					printf("\n");
				}
				puts("]");
			}
			else
			{
				puts("---");
				if(format_output == 1)
				{
					csmi_printer(CSM_YAML,format,output,CSM_STRUCT_MAP(API_PARAMETER_OUTPUT_TYPE));
				}
				else
				{
					printf("Total_Records: %i\n", output->results_count);
					for (i = 0; i < output->results_count; i++) {
						printf("RECORD_%i:\n", i+1);
						printf("  switch_name:             %s\n", output->results[i]->switch_name);
						printf("  serial_number:           %s\n", output->results[i]->serial_number);
						printf("  discovery_time:          %s\n", output->results[i]->discovery_time);
						printf("  collection_time:         %s\n", output->results[i]->collection_time);
						printf("  comment:                 %s\n", output->results[i]->comment);
						printf("  description:             %s\n", output->results[i]->description);
						printf("  fw_version:              %s\n", output->results[i]->fw_version);
						printf("  gu_id:                   %s\n", output->results[i]->gu_id);
						printf("  has_ufm_agent:           %c\n", csm_print_bool_custom(output->results[i]->has_ufm_agent,'t','f'));
						printf("  hw_version:              %s\n", output->results[i]->hw_version);
						printf("  ip:                      %s\n", output->results[i]->ip);
						printf("  model:                   %s\n", output->results[i]->model);
						printf("  num_modules:             %"PRId32"\n", output->results[i]->num_modules);
						printf("  physical_frame_location: %s\n", output->results[i]->physical_frame_location);
						printf("  physical_u_location:     %s\n", output->results[i]->physical_u_location);
						printf("  ps_id:                   %s\n", output->results[i]->ps_id);
						printf("  role:                    %s\n", output->results[i]->role);
						printf("  server_operation_mode:   %s\n", output->results[i]->server_operation_mode);
						printf("  sm_mode:                 %s\n", output->results[i]->sm_mode);
						printf("  state:                   %s\n", output->results[i]->state);
						printf("  sw_version:              %s\n", output->results[i]->sw_version);
						printf("  system_guid:             %s\n", output->results[i]->system_guid);
						printf("  system_name:             %s\n", output->results[i]->system_name);
						printf("  total_alarms:            %"PRId32"\n", output->results[i]->total_alarms);
						printf("  type:                    %s\n", output->results[i]->type);
						printf("  vendor:                  %s\n", output->results[i]->vendor);
					}
				}
				puts("...");
			}
            break;

        case CSMI_NO_RESULTS:
			if(JSON == 1)
			{
				
			}
			else
			{
				puts("---");
				printf("Total_Records: 0\n");
				puts("# No matching records found.");
				puts("...");
			}
            break;
        
        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n",
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }
	
    csm_api_object_destroy(csm_obj);
    
    // Cleanup the library and print the error.
    int lib_return_value = csm_term_lib();
    if( lib_return_value != 0 )
    {
        csmutil_logging(error, "csm_term_lib rc= %d, Initialization failed. Success "
            "is required to be able to communicate between library and daemon. Are the "
            "daemons running?", lib_return_value);
        return lib_return_value;
    }
	
	return return_value;
}
