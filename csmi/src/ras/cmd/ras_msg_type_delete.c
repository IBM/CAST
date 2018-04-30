/*================================================================================

    csmi/src/ras/cmd/ras_msg_type_delete.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
 * Author: Nick Buonarota
 * Email:  nbuonar@us.ibm.com
 */
 
/* C includes*/
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
/*CORAL includes*/
#include "utilities/include/string_tools.h"
/* CSM includes */
#include "csmi/include/csm_api_ras.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Defines to make API easier */
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_delete_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_delete_output_t

///< For use as the usage variable in the input parsers.
#define USAGE  csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_RAS_MSG_TYPE_DELETE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_ras_msg_type_delete ARGUMENTS [OPTIONS]");
	puts("  csm_ras_msg_type_delete -m msg_ids [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to delete an existing RAS message type and remove its record from the 'csm_ras_type' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_ras_msg_type_delete expects 1 mandatory arguments");
	puts("    Argument      | Example value                           | Description  ");                                                 
	puts("    --------------|-----------------------------------------|--------------");
	puts("    -m, --msg_ids | \"category.component.action,test.msg.id\" | (STRING) CSV list of msg ids to delete. Identifier string for this RAS msg type.");
	puts("                  |                                         | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_ras_msg_type_delete -m \"test.msg.id\" ");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",    no_argument,       0, 'h'},
	{"verbose", required_argument, 0, 'v'},
	//arguments
	{"msg_ids", required_argument, 0, 'm'},
	{0,0,0,0}
};

int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 1;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
	/* getopt_long stores the option index here. */
    int option_index = 0;
	int i = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;

	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:m:", longopts, &option_index)) != -1) {
		switch (opt) {
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			/*Specific API parameters.*/
			/*Populate with data received from cmd line parameters.*/
			case 'm':
			{
                csm_optarg_test( "-m, --msg_id", optarg, USAGE );
                csm_parse_csv( optarg, input->msg_ids, input->msg_ids_count, char*,
                            csm_str_to_char, NULL, "-m, --msg_id", USAGE );
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
	if( requiredParameterCounter < NUMBER_OF_REQUIRED_ARGUMENTS || 
        optionalParameterCounter < MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS)
    {
		/*We don't have the correct number of needed arguments passed in.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s).");
		csmutil_logging(error, "    Encountered %i required parameter(s). Expected %i required parameter(s).", requiredParameterCounter, NUMBER_OF_REQUIRED_ARGUMENTS);
		csmutil_logging(error, "    Encountered %i optional parameter(s). Expected at least %i optional parameter(s).", optionalParameterCounter, MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS);
		USAGE();
        return CSMERR_INVALID_PARAM;
	}
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0)
    {
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
	    csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    msg_ids_count: %i", input->msg_ids_count);
	csmutil_logging(debug, "    msg_ids:       %p", input->msg_ids);
	for(i = 0; i < input->msg_ids_count; i++){
		csmutil_logging(debug, "      msg_ids[%i]: %s", i, input->msg_ids[i]);
	}

	/* Call the C API. */
    return_value = csm_ras_msg_type_delete(&csm_obj, input, &output);

    switch( return_value )
    {
        case CSMI_SUCCESS:
            printf("csm_ras_msg_type_delete has completed successfully!\n");
            printf("  The following msg ids have been deleted:\n");
            for(i = 0; i < output->deleted_msg_ids_count; i++)
            {
                printf( "    %s\n", output->deleted_msg_ids[i]);
            }
            break;

        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n",
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));

            // TODO do we really need this?
            if ( output && output->not_deleted_msg_ids_count > 0 )
            {
                csmutil_logging(error, "  msg ids not deleted:\n");
			    for(i = 0; i < output->not_deleted_msg_ids_count; i++)
                {
			        csmutil_logging(error, "    %s\n", output->not_deleted_msg_ids[i]);
			    }
            }
    }

	//Call internal CSM API clean up.
    csm_api_object_destroy(csm_obj);
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

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
