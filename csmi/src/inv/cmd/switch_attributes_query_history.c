/*================================================================================

    csmi/src/inv/cmd/switch_attributes_query_history.c

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

/* Should we do this? */
// #define API_PARAMETER_INPUT_TYPE  = csm_switch_attributes_query_history_input_t
// #define API_PARAMETER_OUTPUT_TYPE = csm_switch_attributes_query_history_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(csm_switch_attributes_query_history_input_t, input); help

void help(){
	puts("_____CSM_SWITCH_ATTRIBUTES_QUERY_HISTORY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_switch_attributes_query_history ARGUMENTS [OPTIONS]");
	puts("  csm_switch_attributes_query_history [-s switch_name] [-l limit] [-o offset] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_switch_history' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_switch_attributes_query_history requires 1 mandatory argument.");
	puts("    Argument          | Example value | Description  ");                                                 
	puts("    ------------------|---------------|--------------");
	puts("    -s, --switch_name | \"abc123\"      | (STRING) Filter results to only include records that have a matching switch name. The switch name is a unique identification switch name for a switch.");
	puts("                      |               | ");
	puts("  FILTERS:");
	puts("    csm_switch_attributes_query can have 2 optional filters.");
	puts("    Argument      | Example value | Description  ");                                                 
	puts("    --------------|---------------|--------------");
	puts("    -l, --limit   | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset  | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_switch_attributes_query_history -s \"abc123\"");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",        no_argument,       0, 'h'},
	{"verbose",     required_argument, 0, 'v'},
	//api arguments
	{"switch_name", required_argument, 0, 's'},
	//filters
	{"limit",       required_argument, 0, 'l'},
	{"offset",      required_argument, 0, 'o'},
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
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 1;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	int i = 0;

	/*Set up data to call API*/
	csm_switch_attributes_query_history_input_t* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(csm_switch_attributes_query_history_input_t, input);
	csm_switch_attributes_query_history_output_t* output = NULL;
	//csm_init_struct_ptr(csm_switch_attributes_query_history_output_t, output);
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:l:o:s:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'l':
				csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
			case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
				break;
			case 's':
                csm_optarg_test( "-s, --switch_name", optarg, USAGE );
				input->switch_name = strdup(optarg);
				requiredParameterCounter++;
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
        csm_free_struct_ptr(csm_switch_attributes_query_history_input_t, input);
		return return_value;           
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:         %p", input);
	csmutil_logging(debug, "  address of input:       %p", &input);
	csmutil_logging(debug, "  csm_switch_attributes_query_history_input_t contains the following:");
	csmutil_logging(debug, "    limit:         %i", input->limit);
	csmutil_logging(debug, "    offset:        %i", input->offset);
	csmutil_logging(debug, "    switch_name: %s", input->switch_name);
	csmutil_logging(debug, "  value of output:        %p", output);
	csmutil_logging(debug, "  address of output:      %p", &output);
	
	/* Call the actual CSM API */
	return_value = csm_switch_attributes_query_history(&csm_obj, input, &output);
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(csm_switch_attributes_query_history_input_t, input);
	
    switch( return_value )
    {
        case CSMI_SUCCESS:
	    	puts("---");
	    	printf("Total_Records: %i\n", output->results_count);
	    	for (i = 0; i < output->results_count; i++) {
	    		printf("RECORD_%i:\n", i+1);
	    		printf("  history_time:            %s\n", output->results[i]->history_time);
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
				printf("  operation:               %s\n", output->results[i]->operation);
				printf("  archive_history_time:    %s\n", output->results[i]->archive_history_time);
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
