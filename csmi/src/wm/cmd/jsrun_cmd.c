/*================================================================================

    csmi/src/wm/cmd/jsrun_cmd.c

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

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

#define API_PARAMETER_INPUT_TYPE csm_jsrun_cmd_input_t
#define API_PARAMETER_OUTPUT_TYPE

struct option longopts[] = {
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	{"allocation_id", required_argument, 0, 'a'},
	{"kv_pairs",      required_argument, 0, 'k'},
	{"jsm_path",      required_argument, 0, 'p'},
	{0,0,0,0}
};

static void help()
{
	puts("USAGE:");
	puts("  jsrun_cmd ARGUMENTS [OPTIONS]");
	puts("  jsrun_cmd -a allocation_id -k \"key=value,\" [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to start jsm with a set of key value pairs on a compute daemon.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    jsrun_cmd requires an allocation_id to determine where to start the daemon.");
	puts("    Argument                | Example value | Description  "); 
	puts("    ------------------------|---------------|--------------");
	puts("    -a, --allocation_id     | 1             | (LONG INTEGER) Allocation identifier [>0].");
	puts("                            |               | ");
    puts("  OPTIONAL:");
    puts("    jsrun_cmd may be run with optional arguments.");

    puts("    Argument               | Example value       | Description  ");
    puts("    -----------------------|---------------------|--------------");
    puts("    -k, --kv_pairs         | \"gpus=0,mem=1024\" | (String) A comma separated list of alphanumeric ");
    puts("                                                   key value pairs (indicated by equals signs).");
    puts("    -p, --jsm_path         | \"/path/to/jsm\"    | (String) A linux path to the jsm executable.");
    puts("                           |                     | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  jsm_cmd  -a 1 -k \"gpus=0,mem=1024\"");
	puts("____________________");
}

int main(int argc, char *argv[])
{
	int               opt;
	int               return_value;
	int               indexptr = 0;
    
    /*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    input->allocation_id = 0;
    input->kv_pairs      = NULL;
    input->jsm_path      = NULL;

	csm_api_object   *csm_obj = NULL;
	char             *arg_check = NULL; ///< Used in verifying the long arg values.

	while ((opt = getopt_long(argc, argv, "hv:a:k:p:", longopts, &indexptr)) != -1) {
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
                csm_str_to_int64(input->allocation_id, optarg, arg_check, "-a, --allocation", USAGE)
				break;      
            }
            case 'k':
                csm_optarg_test( "-k, --kv_pairs", optarg, USAGE );
                input->kv_pairs = strdup(optarg);
                break;
            case 'p':
                csm_optarg_test( "-p, --jsm_path", optarg, USAGE );
                input->jsm_path = strdup(optarg);
                break;
			default:      
                csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
                return CSMERR_INVALID_PARAM;
		}
	}

	if (input->allocation_id <= 0)
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

	return_value = csm_jsrun_cmd(&csm_obj, input);

	if (return_value == CSMI_SUCCESS ) 
    {
	    printf("---\n# Allocation Id: %ld successfully started\n...\n", input->allocation_id);
	}
	else 
    {
		printf("# %s FAILED: returned: %d, errcode: %d errmsg: %s\n", argv[0], return_value, 
            csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));

        csm_print_node_errors(csm_obj)
	}
	// Free after because the SUCESS case uses input-allocation_id
	/* Use CSM API free to release arguments. We no longer need them. */
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

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
