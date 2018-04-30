/*================================================================================

    csmi/src/wm/cmd/allocation_update_state.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <inttypes.h>
#include "csmi/include/csm_api_workload_manager.h"
/*Needed for CSM logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

#include <assert.h>

#define EXPECTED_NUMBER_OF_ARGUMENTS 2

///< For use as the usage variable in the input parsers.
#define USAGE help

struct option longopts[] = {
  {"help",          no_argument,       0, 'h'},
  {"verbose",       required_argument, 0, 'v'},
  {"allocation_id", required_argument, 0, 'a'}, 
  {"state",         required_argument, 0, 's'},
  {0,0,0,0}
};

static void help()
{
    char* argv0= "csm_allocation_update_state";
	puts("_____CSM_ALLOCATION_UPDATE_STATE_CMD_HELP_____");
	puts("USAGE:");
	printf("  %s ARGUMENTS [OPTIONS]\n", argv0);
	printf("  %s -a allocation_id -s state [-h] [-v verbose_level]\n", argv0);
	puts("");
	puts("SUMMARY: Used to modify the state field of an existing allocation.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	printf("    %s expects 2 mandatory arguments\n", argv0);
	puts("    Argument                | Example value      | Description  ");                                                 
	puts("    ------------------------|--------------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -a, --allocation_id     | 1                  | (LONG INTEGER) Unique identifier for an allocation.");
	puts("    -s, --state             | \"running\"          | (STRING) State of allocation.");
	puts("                            |                    | Valid values: \"running\" or \"staging-out\"");
	puts("                            |                    | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	printf("  %s -a 1 -s \"running\"\n", argv0);
	puts("____________________");
}

int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
    int return_value = 0;
	char *arg_check = NULL; ///< Used in verifying the long arg values.
	/*Variables for checking cmd line args*/
	int opt;
	int indexptr = 0;
	int parameterCounter = 0;
	/*api parameters*/
    csm_allocation_update_state_input_t input;
    csm_init_struct_versioning(&input);
    input.allocation_id = 0;
    input.new_state     = csm_enum_max(csmi_state_t);

    while ((opt = getopt_long(argc, argv, "hv:a:s:", longopts, &indexptr)) != -1) {
        switch (opt) {
			case 'h':
                USAGE();
                return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
            {
                csm_optarg_test( "-a, --allocation", optarg, USAGE )
                csm_str_to_int64(input.allocation_id, optarg, arg_check, "-a, --allocation", USAGE)
				parameterCounter++;
				break;
			}
            case 's':
            {
                csm_optarg_test( "-s, --state", optarg, USAGE )
                int temp_state = csm_enum_from_string(optarg, csmi_state_t_strs);
                input.new_state = temp_state != -1 ? 
                    (csmi_state_t) temp_state : csm_enum_max(csmi_state_t);
				parameterCounter++;
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
	if(parameterCounter < EXPECTED_NUMBER_OF_ARGUMENTS){
		/*We don't have the correct number of arguments passed in. We expecting EXPECTED_NUMBER_OF_ARGUMENTS.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s). Encountered %i parameter(s). Expected %i mandatory parameter(s).", parameterCounter, EXPECTED_NUMBER_OF_ARGUMENTS);
		csmutil_logging(error, "  Run with '-h' for help and information.");
		return CSMERR_INVALID_PARAM;
	}

    // If the input was invalid display the help and return an invalid parameter.
    if ((input.allocation_id <= 0) || input.new_state == csm_enum_max(csmi_state_t))
    {
        // TODO error message.
        USAGE();
        return CSMERR_INVALID_PARAM;
    }

    

    /* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
    if( return_value != 0){
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required"
            " to be able to communicate between library and daemon. Are the daemons running?", 
            return_value);
		return return_value;            
    }

    return_value = csm_allocation_update_state(&csm_obj, &input);

    if ( return_value == CSMI_SUCCESS )
    {
        printf("Allocation ID %" PRId64 " was updated to %s successfully\n",
            input.allocation_id, csm_get_string_from_enum(csmi_state_t,input.new_state));
    }
    else 
    {
        printf("%s FAILED: errcode: %d errmsg: %s\n",
            argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }

    // it's the csmi library's responsibility to free internal space
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
