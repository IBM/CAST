/*================================================================================

    csmi/src/wm/cmd/allocation_query_active_all.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

e===============================================================================*/
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
#include "csmi/src/wm/include/csmi_wm_internal.h"


#include <assert.h>

#define API_PARAMETER_INPUT_TYPE csm_allocation_query_active_all_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_allocation_query_active_all_output_t
#define API_FORMAT_START "num_allocations,allocations{"
#define API_FORMAT_STRING "allocation_id,primary_job_id,begin_time,state,user_name,num_nodes,launch_node_name"
#define API_FORMAT_END   "}"

///< For use as the usage variable in the input parsers.
#define USAGE  help

struct option longopts[] = {
	{"help",    no_argument,       0, 'h'},
	{"verbose", required_argument, 0, 'v'},
    {"format",  required_argument,  0, 'f'},
	{0,0,0,0}
}
;

static void help(){
	puts("_____CSM_ALLOCATION_QUERY_ACTIVE_ALL_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_query_active_all ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_query_active_all [-h] [-v verbose_level] [-f format_string]");
	puts("");
	puts("SUMMARY: Used to get information about all active allocations.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    Argument     | Example value               | Description  ");
	puts("    -------------|-----------------------------|--------------");
	puts("    -f, --format | allocation_id,compute_nodes | (STRING) Format string detailing the fields to display.");
    puts("                                   Values are comma delimited, matching the contents of the struct.");
    puts("                                   If a result has a subresult (e.g. struct pointer) those elements");
    puts("                                   may be indicated by <subresult_field>\"{\"<formatting>\"}\".");
    puts("                                   \"%\" Indicates all values should be shown for that result level and deeper.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  >0 if ERROR.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_query_active_all");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

int main(int argc, char *argv[])
{
	/*Helper Variables*/
    //int j;
	int return_value = 0;
    int opt;
    int indexptr = 0;
    char *format = NULL;
	/*CSM Variables*/
	csm_api_object    *csm_obj = NULL;
	/*API Variables*/
    API_PARAMETER_INPUT_TYPE input;
	API_PARAMETER_OUTPUT_TYPE* output = NULL;

    csm_init_struct(API_PARAMETER_INPUT_TYPE, input);

    while ((opt = getopt_long(argc, argv, "hf:v:", longopts, &indexptr)) != -1) {
        switch(opt){
			case 'h':
                USAGE();
                return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
            case 'f':
                CSM_WRAP_FORMAT_STRING( format, 
                    API_FORMAT_START,optarg,API_FORMAT_END );
                break;
			default:
				csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
				return CSMERR_INVALID_PARAM;
        }
    }

    /* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if ( return_value != 0 )
    {
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required "
            "to be able to communicate between library and daemon. Are the daemons running?", 
            return_value);
		return return_value;
	}

    return_value = csm_allocation_query_active_all( &csm_obj, &input, &output);

    switch(return_value)
    {
        case CSMI_SUCCESS:
        {
            puts("---");

            if ( !format ) 
            {
                CSM_WRAP_FORMAT_STRING( format, 
                    API_FORMAT_START,API_FORMAT_STRING,API_FORMAT_END );
            }
            csmi_printer(CSM_YAML,format,output,CSM_STRUCT_MAP(API_PARAMETER_OUTPUT_TYPE));
	        
            puts("\n...");

            break;
         }

        case CSMI_NO_RESULTS:
            puts("---");
            printf(" num_allocations: 0\n");
            puts(" # No matching records found.");
            puts("...");
            break;

        default:
	    	printf("%s FAILED: errcode: %d errmsg: %s\n", 
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
	}
	
    if ( format ) free(format);

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
