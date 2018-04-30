/*================================================================================

    csmi/src/wm/cmd/allocation_update_history.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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
#include "csmi/include/csm_api_workload_manager.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

#define API_PARAMETER_INPUT_TYPE csm_allocation_update_history_input_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_NODE_ATTRIBUTES_UPDATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_update_history ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_update_history -n node_names [-c comment] [-f physical_frame_location] [-r ready] [-s state] [-t type] [-u physical_u_location] [-1 feature_1] [-2 feature_2] [-3 feature_3] [-4 feature_4] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to update a record in the 'csm_allocation_history' table of the csm database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_allocation_update_history expects 1 mandatory parameter");
	puts("    Argument                | Example value      | Description  ");                                                 
	puts("    ------------------------|--------------------|--------------");
	puts("    -a, --allocation_id     | 1                  | (LONG INTEGER) Unique identifier for an allocation.");
    puts("");
	puts("  OPTIONAL:");
	puts("    csm_allocation_update_history has 7 optional arguments (1 required)");
	puts("    Argument                      | Example value            | Description  ");                                                 
	puts("    ------------------------------|--------------------------|--------------");
	puts("    -u, --user_name               | jdunham                  | (STRING) The new Linux user name to assign the allocation to.");
	puts("    -U, --user_id                 | 9999137                  | (INTEGER) The new user id to assign this allocation to.");
	puts("    -g, --user_group_id           | 1                        | (INTEGER) The new user group id to assign this allocation to.");
	puts("    -A, --account                 | \"sys dev\"              | (STRING) Account responsible for this allocation.");
	puts("    -c, --comment                 | \"Allocation notes\"     | (STRING) Comment field for notes about the allocation (destroys old comment).");
	puts("    -j, --job_name                | \"usage_predictor\"      | (STRING) The name of the job which spawned the allocation");
	puts("    -r, --reservation             | \"nrg_reserv_120\"       | (STRING) The name of the reservation for the allocation."); // TODO what?
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_update_history -a 12 --account \"wxyz\" ");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	//api arguments
	{"allocation_id", required_argument, 0, 'a'},
	{"use_name",      required_argument, 0, 'u'},
	{"user_id",       required_argument, 0, 'U'},
	{"user_group_id", required_argument, 0, 'g'},
	{"account",       required_argument, 0, 'A'},
	{"comment",       required_argument, 0, 'c'},
	{"job_name",      required_argument, 0, 'j'},
	{"reservation",   required_argument, 0, 'r'},
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
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:a:u:U:g:A:c:j:r:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
				csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
                csm_optarg_test( "-a, --allocation_id", optarg, USAGE )
                csm_str_to_int64( input->allocation_id, optarg, arg_check,  "-a, --allocation_id", USAGE );
				requiredParameterCounter++;
                break;
            case 'u':
                csm_optarg_test( "-u, --user_name", optarg, USAGE )
				input->user_name = strdup(optarg);
				optionalParameterCounter++;
                break;
			case 'U':
                csm_optarg_test( "-u, --user_id", optarg, USAGE )
                csm_str_to_int32( input->user_id, optarg, arg_check, "-u, --user_id", USAGE );
				optionalParameterCounter++;
				break;
			case 'g':
                csm_optarg_test( "-g, --user_group_id", optarg, USAGE )
                csm_str_to_int32( input->user_group_id, optarg, arg_check, "-g, --user_group_id", USAGE );
				optionalParameterCounter++;
				break;

            case 'A':
                csm_optarg_test( "-A, --account", optarg, USAGE )
				input->account = strdup(optarg);
				optionalParameterCounter++;
                break;
			case 'c':
				input->comment = optarg && optarg[0] ? strdup(optarg) : strdup(" ");
				optionalParameterCounter++;
				break;
            case 'j':
                csm_optarg_test( "-j, --job_name", optarg, USAGE )
				input->job_name = strdup(optarg);
				optionalParameterCounter++;
                break;
			case 'r':
                csm_optarg_test( "-r, --reservation", optarg, USAGE )
				input->reservation = strdup(optarg);
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
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;           
	}
	
	/* Call the actual CSM API */
	return_value = csm_allocation_update_history(&csm_obj, input);

    if ( return_value == CSMI_SUCCESS )
    {
        printf( "---\n# Successfully updated allocation %"PRId64"\n...\n",
            input->allocation_id);
    }
    else
    {
        printf("---\n# FAILED: errcode: %d errmsg: %s\n...\n",
            return_value,  csm_api_object_errmsg_get(csm_obj));
    }
	
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	//Call internal CSM API clean up.
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
