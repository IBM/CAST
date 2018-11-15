/*================================================================================

    csmi/src/wm/cmd/soft_failure_recovery.c

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

#define API_PARAMETER_INPUT_TYPE 
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
	puts("  csm_soft_failure_recovery ARGUMENTS [OPTIONS]");
	puts("  csm_soft_failure_recovery ");
	puts("");
	puts("SUMMARY: Used to recover all nodes in the cluster from soft failure.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    Argument                | Example value | Description  "); 
	puts("    ------------------------|---------------|--------------");
	puts("                            |               | ");
    puts("  OPTIONAL:");
    puts("    Argument               | Example value       | Description  ");
    puts("    -----------------------|---------------------|--------------");
    puts("                           |                     | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_soft_failure_recovery");
	puts("____________________");
}

int main(int argc, char *argv[])
{
	int               opt;
	int               return_value;
	int               indexptr = 0;
    
    //API_PARAMETER_INPUT_TYPE input;    
    csm_soft_failure_recovery_input_t input;
    csm_soft_failure_recovery_output_t *output;

	csm_api_object   *csm_obj = NULL;

	while ((opt = getopt_long(argc, argv, "hv:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
                return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			default:      
                csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
                return CSMERR_INVALID_PARAM;
		}
	}


    return_value = csm_init_lib();
	if ( return_value != 0 )
    {
        csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
        csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?",return_value);
        return return_value;
    }        
    
    return_value = csm_soft_failure_recovery(&csm_obj, &input, &output);

	if (return_value == CSMI_SUCCESS ) 
    {
	    printf("---\n# Soft Failure cleaned up.\n");

        if ( output->error_count > 0 )
        {
            printf("# %d Nodes Failed:\n", output->error_count);
            int i=0;
            for(;i< output->error_count; i++)
            {
                csm_soft_failure_recovery_node_t* node_error = output->node_errors[i];
                printf("# %s[%d]: %s\n", node_error->source, node_error->errcode, node_error->errmsg);
            }
        }

        printf("...\n");
	}
	else 
    {
		printf("# %s FAILED: returned: %d, errcode: %d errmsg: %s\n", argv[0], return_value, 
            csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
        
        csm_print_node_errors(csm_obj)
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
