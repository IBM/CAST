/*================================================================================

    csmi/src/inv/cmd/switch_attributes_update.c

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

/* Should we do this? */
// #define API_PARAMETER_INPUT_TYPE  = csm_switch_attributes_update_input_t
// #define API_PARAMETER_OUTPUT_TYPE = csm_switch_attributes_update_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(csm_switch_attributes_update_input_t, input); help

void help(){
	puts("_____CSM_SWITCH_ATTRIBUTES_UPDATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_switch_attributes_update ARGUMENTS [OPTIONS]");
	puts("  csm_switch_attributes_update -s switch_names [-c comment] [-f physical_frame_location] [-u physical_u_location] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_switch_history' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_switch_attributes_update requires 1 mandatory argument.");
	puts("    Argument          | Example value   | Description  ");                                                 
	puts("    ------------------|-----------------|--------------");
	puts("    -s, --switch_name | \"abc123,xyz789\" | (STRING) This is a csv field of valid switch names. Identifies which switches will be updated.");
	puts("                      |                 | ");
	puts("  OPTIONAL:");
	puts("    csm_switch_attributes_update can have 3 optional arguments and requires at least 1");
	puts("    Argument                      | Example value        | Description  ");                                                 
	puts("    ------------------------------|----------------------|--------------");
	puts("    -c, --comment                 | \"My awesome comment\" | (STRING) Comment field for system administrators. Can be reset to NULL in CSM DB via \"#CSM_NULL\".");
	puts("    -f, --physical_frame_location | \"X12,Y24\"            | (STRING) Physical frame number where the switch is located.");
	puts("    -u, --physical_u_location     | \"U02\"                | (STRING) Physical u location (position in the frame) where the switch is located.");
	puts("                                  |                      | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_switch_attributes_update -s \"abc123\" -f \"X12,Y24\" -u \"U02\" ");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",                    no_argument,       0, 'h'},
	{"verbose",                 required_argument, 0, 'v'},
	//api arguments
	{"comment",                 required_argument, 0, 'c'},
	{"physical_frame_location", required_argument, 0, 'f'},
	{"switch_names",            required_argument, 0, 's'},
	{"physical_u_location",     required_argument, 0, 'u'},
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
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 1;
	/*Variables for checking cmd line args*/
	int opt;
	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	uint32_t i = 0;

	/*Set up data to call API*/
	csm_switch_attributes_update_input_t* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(csm_switch_attributes_update_input_t, input);
	csm_switch_attributes_update_output_t* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:c:f:s:u:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'c':
                input->comment = optarg && optarg[0] ? strdup(optarg): strdup(" ");
				optionalParameterCounter++;
				break;
			case 'f':
                csm_optarg_test( "-f, --physical_frame_location", optarg, USAGE )
				input->physical_frame_location = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 's':
			{
                csm_optarg_test( "-s, --switch_name", optarg, USAGE )
                csm_parse_csv( optarg, input->switch_names, input->switch_names_count, char*,
                        csm_str_to_char, NULL, "-s, --switch_name", USAGE );
				requiredParameterCounter++;
				break;
			}
			case 'u':
                csm_optarg_test( "-u, --physical_u_location", optarg, USAGE )
				input->physical_u_location = strdup(optarg);
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
        csm_free_struct_ptr(csm_switch_attributes_update_input_t, input);
		return return_value;           
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:         %p", input);
	csmutil_logging(debug, "  address of input:       %p", &input);
	csmutil_logging(debug, "  csm_switch_attributes_update_input_t contains the following:");
	csmutil_logging(debug, "    switch_names_count:      %i", input->switch_names_count);
	csmutil_logging(debug, "    switch_names:            %p", input->switch_names);
	for(i = 0; i < input->switch_names_count; i++){
		csmutil_logging(debug, "      switch_names[%i]: %s", i, input->switch_names[i]);
	}
	csmutil_logging(debug, "    comment:                 %s", input->comment);
	csmutil_logging(debug, "    physical_frame_location: %s", input->physical_frame_location);
	csmutil_logging(debug, "    physical_u_location:     %s", input->physical_u_location);
	csmutil_logging(debug, "  value of output:        %p", output);
	csmutil_logging(debug, "  address of output:      %p", &output);
	
	/* Call the actual CSM API */
	return_value = csm_switch_attributes_update(&csm_obj, input, &output);
	
    switch( return_value )
    {
        case CSMI_SUCCESS:
            printf( "---\n# All %i record(s) successfully updated.\n...\n",
                input->switch_names_count);
            break;

        case CSMERR_UPDATE_MISMATCH:
            printf( "---\n# Could not update %i of the %i record(s):\n", 
                output->failure_count,
                input->switch_names_count);
            printf("total_failures: %i\nswitch_names:\n", output->failure_count );

            for( i = 0; i < output->failure_count; i++ )
            {
                printf( " - %s\n",  output->failure_switches[i] );
            }

            printf("...\n");
            break;

        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n",
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }
	
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(csm_switch_attributes_update_input_t, input);
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
