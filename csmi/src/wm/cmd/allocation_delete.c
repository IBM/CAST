/*================================================================================

    csmi/src/wm/cmd/allocation_delete.c

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

///< For use as the usage variable in the input parsers.
#define USAGE  help

#define API_PARAMETER_INPUT_TYPE csm_allocation_delete_input_t
#define API_PARAMETER_OUTPUT_TYPE

struct option longopts[] = {
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	{"allocation_id", required_argument, 0, 'a'},
	{"primary_job_id",   required_argument, 0, 'j'},
	{"secondary_job_id", required_argument, 0, 'J'},
	{0,0,0,0}
};

static void help()
{
	puts("_____CSM_ALLOCATION_DELETE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_delete ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_delete -a allocation_id [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to delete an existing allocation.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_allocation_delete expects at least an allocation id or primary job id.");
	puts("    Argument                | Example value | Description  "); 
	puts("    ------------------------|---------------|--------------");
	puts("    -a, --allocation_id     | 1             | (LONG INTEGER) Allocation identifier [>0].");
    puts("                                               Only specify allocation_id or primary job id.");
    puts("    -j, --primary_job_id    | 1             | (LONG INTEGER) Primary job id [>0].");
    puts("                                               Only specify allocation_id or primary job id.");
	puts("                            |               | ");
    puts("  OPTIONAL:");
    puts("    csm_allocation_delete can have 1 optional argument");

    puts("    Argument               | Example value   | Description  ");
    puts("    -----------------------|-----------------|--------------");
    puts("    -J, --secondary_job_id | 0               | (INTEGER) Secondary job id (only specify with primary job id).");
    puts("                                               Defaults to zero..");
    puts("                           |                 | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_delete -a 1");
	puts("____________________");
  // csmutil_logging(error, "Usage: %s -a id\n", argv0);
  // csmutil_logging(error, "       where:\n");
  // csmutil_logging(error, "              -a, --allocation_id=LONG INTEGER\n");
}

int main(int argc, char *argv[])
{
	int               opt;
	int               return_value;
	int               indexptr = 0;
    
    API_PARAMETER_INPUT_TYPE input;    
    input.allocation_id = 0;
    input.primary_job_id =0;
    input.secondary_job_id=0;

	csm_api_object   *csm_obj = NULL;
	char             *arg_check = NULL; ///< Used in verifying the long arg values.

	while ((opt = getopt_long(argc, argv, "hv:a:j:J:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
                return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':      
            {
                csm_optarg_test( "-a, --allocation", optarg, USAGE )
                csm_str_to_int64(input.allocation_id, optarg, arg_check, "-a, --allocation", USAGE)
				break;      
            }
            case 'j':
                csm_optarg_test( "-j, --primary_job_id", optarg, USAGE );
                csm_str_to_int64( input.primary_job_id, optarg, arg_check,
                                "-j, --primary_job_id", USAGE );
                break;
            case 'J':
                csm_optarg_test( "-J, --secondary_job_id", optarg, USAGE );
                csm_str_to_int32( input.secondary_job_id, optarg, arg_check,
                                "-J, --secondary_job_id", USAGE );
    break;
			default:       
                csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
                return CSMERR_INVALID_PARAM;
		}
	}

	if (input.allocation_id <= 0 && input.primary_job_id <= 0)
    {
		USAGE();
        return CSMERR_INVALID_PARAM;
	}

	return_value = csm_init_lib();
	if( return_value != 0)
    {
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
		return return_value;
	}

	return_value = csm_allocation_delete(&csm_obj, &input);

	if (return_value == CSMI_SUCCESS ) 
    {
	    printf("---\n# Allocation Id: %ld successfully deleted\n...\n", input.allocation_id);
	}
	else 
    {
		printf("# %s FAILED: returned: %d, errcode: %d errmsg: %s\n", argv[0], return_value, 
            csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
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
