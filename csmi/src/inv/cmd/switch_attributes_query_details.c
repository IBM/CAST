/*================================================================================

    csmi/src/inv/cmd/switch_attributes_query_details.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

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
#include <inttypes.h>
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

/* Define API types to make life easier. */
#define API_PARAMETER_INPUT_TYPE csm_switch_attributes_query_details_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_switch_attributes_query_details_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_NODE_ATTRIBUTES_QUERY_DETAILS_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_switch_attributes_query_details ARGUMENTS [OPTIONS]");
	puts("  csm_switch_attributes_query_details -s switch_name [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_switch' and related children tables to get everything there is to know about a switch in the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_switch_attributes_query_details expects 1 mandatory argument");
	puts("    Argument          | Example value | Description  ");                                                 
	puts("    ------------------|---------------|--------------");
	puts("    -s, --switch_name | \"switch_01\"   | (STRING) This is the switch name to query. The switch name is a unique identification for a switch.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_switch_attributes_query_details -s \"switch_01\"");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",       no_argument,       0, 'h'},
	{"verbose",    required_argument, 0, 'v'},
	//api arguments
	{"switch_name", required_argument, 0, 's'},
	{0,0,0,0}
};

/*
* Summary: Simple command line interface for the CSM API 'node attributes query details'. 
* 			Takes in the name of a single node via command line parameters, queries the 
*			database for this node, and prints all the attributes of that node.
*/
int main(int argc, char *argv[])
{	
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 1;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    //char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	uint32_t i = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:s:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 's':
            {
				/*checks if NULL*/
                csm_optarg_test( "-s, --switch_name", optarg, USAGE );
				input->switch_name = strdup(optarg);
				
				/* Increment requiredParameterCounter so later we can check if arguments were correctly set before calling API. */
				requiredParameterCounter++;
				break;
			}
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
	if(return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:    %p", input);
	csmutil_logging(debug, "  address of input:  %p", &input);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    switch_name:       %s", input->switch_name);
	
    /* Call the C API. */
	return_value = csm_switch_attributes_query_details(&csm_obj, input, &output);
	/* Use CSM API free to release arguments. We no longer need them. */
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    switch(return_value)
    {
        case CSMI_SUCCESS:
			puts("---");
			printf("Total_Records: %i\n", output->result_count);
	    	printf("Record_%i:\n", output->result_count);
			printf("  switch_name:                    %s\n", output->result[0]->switch_data->switch_name);
			printf("  switch_serial_number:           %s\n", output->result[0]->switch_data->serial_number);
			printf("  switch_discovery_time:          %s\n", output->result[0]->switch_data->discovery_time);
			printf("  switch_collection_time:         %s\n", output->result[0]->switch_data->collection_time);
			printf("  switch_comment:                 %s\n", output->result[0]->switch_data->comment);
			printf("  switch_description:             %s\n", output->result[0]->switch_data->description);
			printf("  switch_fw_version:              %s\n", output->result[0]->switch_data->fw_version);
			printf("  switch_gu_id:                   %s\n", output->result[0]->switch_data->gu_id);
			printf("  switch_has_ufm_agent:           %c\n", csm_print_bool_custom(output->result[0]->switch_data->has_ufm_agent,'t','f'));
			printf("  switch_hw_version:              %s\n", output->result[0]->switch_data->hw_version);
			printf("  switch_ip:                      %s\n", output->result[0]->switch_data->ip);
			printf("  switch_model:                   %s\n", output->result[0]->switch_data->model);
			printf("  switch_num_modules:             %"PRId32"\n", output->result[0]->switch_data->num_modules);
			printf("  switch_physical_frame_location: %s\n", output->result[0]->switch_data->physical_frame_location);
			printf("  switch_physical_u_location:     %s\n", output->result[0]->switch_data->physical_u_location);
			printf("  switch_ps_id:                   %s\n", output->result[0]->switch_data->ps_id);
			printf("  switch_role:                    %s\n", output->result[0]->switch_data->role);
			printf("  switch_server_operation_mode:   %s\n", output->result[0]->switch_data->server_operation_mode);
			printf("  switch_sm_mode:                 %s\n", output->result[0]->switch_data->sm_mode);
			printf("  switch_state:                   %s\n", output->result[0]->switch_data->state);
			printf("  switch_sw_version:              %s\n", output->result[0]->switch_data->sw_version);
			printf("  switch_system_guid:             %s\n", output->result[0]->switch_data->system_guid);
			printf("  switch_system_name:             %s\n", output->result[0]->switch_data->system_name);
			printf("  switch_total_alarms:            %"PRId32"\n", output->result[0]->switch_data->total_alarms);
			printf("  switch_type:                    %s\n", output->result[0]->switch_data->type);
			printf("  switch_vendor:                  %s\n", output->result[0]->switch_data->vendor);
			printf("  inventory_count:                %i\n", output->result[0]->inventory_count);
			if(output->result[0]->inventory_count > 0)
			{
				printf("  inventory:\n");
				for(i = 0; i < output->result[0]->inventory_count; i++)
				{
					printf("    - name:             %s\n", output->result[0]->inventory[i]->name);
					printf("      host_system_guid: %s\n", output->result[0]->inventory[i]->host_system_guid);
					printf("      discovery_time:   %s\n", output->result[0]->inventory[i]->discovery_time);
					printf("      collection_time:  %s\n", output->result[0]->inventory[i]->collection_time);
					printf("      comment:          %s\n", output->result[0]->inventory[i]->comment);
					printf("      description:      %s\n", output->result[0]->inventory[i]->description);
					printf("      device_name:      %s\n", output->result[0]->inventory[i]->device_name);
					printf("      device_type:      %s\n", output->result[0]->inventory[i]->device_type);
					printf("      max_ib_ports:     %"PRId32"\n", output->result[0]->inventory[i]->max_ib_ports);
					printf("      module_index:     %"PRId32"\n", output->result[0]->inventory[i]->module_index);
					printf("      number_of_chips:  %"PRId32"\n", output->result[0]->inventory[i]->number_of_chips);
					printf("      path:             %s\n", output->result[0]->inventory[i]->path);
					printf("      serial_number:    %s\n", output->result[0]->inventory[i]->serial_number);
					printf("      severity:         %s\n", output->result[0]->inventory[i]->severity);
					printf("      status:           %s\n", output->result[0]->inventory[i]->status);
				}
			}
			puts("...");
            break;

        case CSMI_NO_RESULTS:
            puts("---");
            printf("Total_Records: 0\n");
            puts("# No matching records found.");
            puts("...");
            break;
    
        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n", argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }
		
    /* Call internal CSM API clean up. */
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
