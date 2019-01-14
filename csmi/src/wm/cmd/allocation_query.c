/*================================================================================

    csmi/src/wm/cmd/allocation_query.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

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

#define API_PARAMETER_INPUT_TYPE  csm_allocation_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_allocation_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE  help

struct option longopts[] = {
  {"help",             no_argument,       0, 'h'},
  {"verbose",          required_argument, 0, 'v'},
  {"allocation_id",    required_argument, 0, 'a'}, 
  {"primary_job_id",   required_argument, 0, 'j'}, 
  {"secondary_job_id", required_argument, 0, 'J'}, 
  {0,0,0,0}
};

static void help()
{
	puts("_____CSM_ALLOCATION_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_query ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_query [-a id] [-j id] [-J id] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to get information about a specific allocation based off supplied id(s).");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_allocation_query can take 3 optional arguments");
	puts("    At least one must be provided.");
	puts("    The combination of input ids must result in a unique allocation, or else an error is returned (ambiguous query).");
    puts("    A unique allocation is defined by an allocation id or a primary job id paired with a secondary job id.");
	puts("    Argument                  | Example value | Description  ");                                                 
	puts("    --------------------------|---------------|--------------");
	puts("    -a, --allocation_id       | 1             | (LONG INTEGER) Unique identifier for an allocation.");
	puts("    -j, --primary_job_id      | 1             | (LONG INTEGER) Primary job id (for lsf this will be the lsf job id).");
	puts("    -J, --secondary_job_id    | 0             | (INTEGER)      Secondary job id (for lsf this will be the lsf job index for job arrays).");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_query -a 1");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

int main(int argc, char *argv[])
{
	/*Helper Variables*/
	uint32_t i;
	int opt;
	int indexptr = 0;
	int return_value = 0;
	char *arg_check = NULL; ///< Used in verifying the long arg values.
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*API Variables*/
    API_PARAMETER_INPUT_TYPE input;
    API_PARAMETER_OUTPUT_TYPE *output = NULL;

    csm_init_struct(API_PARAMETER_INPUT_TYPE, input);


	while ((opt = getopt_long(argc, argv, "hv:a:j:J:", longopts, &indexptr)) != -1) {
		switch (opt) {
			case 'h':
                USAGE();
                return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
                csm_optarg_test( "-a, --allocation", optarg, USAGE )
                csm_str_to_int64(input.allocation_id, optarg, arg_check, "-a, --allocation", help)
				break;
			case 'j':
                csm_optarg_test( "-j, --primary_job_id", optarg, USAGE )
                csm_str_to_int64(input.primary_job_id, optarg, 
                    arg_check, "-j, --primary_job_id", help)
				break;
			case 'J':
                csm_optarg_test( "-J, --secondary_job_id", optarg, USAGE )
                csm_str_to_int32(input.secondary_job_id, optarg, 
                    arg_check, "-J, --secondary_job_id", help)
				break;
			default:
				csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
                return CSMERR_INVALID_PARAM;
		}
	}


	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
		return return_value;
	}

	return_value = csm_allocation_query(&csm_obj, &input, &output);

    switch(return_value)
    {
        case CSMI_SUCCESS:
        { 
            // Grab the allocation struct for convenience.
            csmi_allocation_t *allocation = output->allocation;

	    	puts("---");
	    	printf("allocation_id:                  %" PRId64 "\n", allocation->allocation_id);
	    	printf("primary_job_id:                 %" PRId64 "\n", allocation->primary_job_id);
	    	printf("secondary_job_id:               %" PRId32 "\n", allocation->secondary_job_id);
	    	printf("num_nodes:                      %" PRId32 "\n", allocation->num_nodes);

	    	if (allocation->num_nodes > 0) {
	    		printf("compute_nodes:\n");
	    		for (i = 0; i < allocation->num_nodes; i++) {
	    			printf(" - %s\n", allocation->compute_nodes[i]);
	    		}
	    	}
	    	printf("ssd_file_system_name:           %s\n", allocation->ssd_file_system_name);
	    	printf("launch_node_name:               %s\n", allocation->launch_node_name);
	    	printf("user_flags:                     %s\n", allocation->user_flags);
	    	printf("system_flags:                   %s\n", allocation->system_flags);
	    	printf("smt_mode:                       %" PRId16 "\n", allocation->smt_mode);
	    	printf("ssd_min:                        %" PRId64 "\n", allocation->ssd_min);
	    	printf("ssd_max:                        %" PRId64 "\n", allocation->ssd_max);
	    	printf("num_processors:                 %" PRId32 "\n", allocation->num_processors);
	    	printf("num_gpus:                       %" PRId32 "\n", allocation->num_gpus);
	    	printf("projected_memory:               %" PRId32 "\n", allocation->projected_memory);
	    	printf("state:                          %s\n", csm_get_string_from_enum(csmi_state_t, allocation->state));
	    	printf("type:                           %s\n", csm_get_string_from_enum(csmi_allocation_type_t,allocation->type));
	    	printf("job_type:                       %s\n", csm_get_string_from_enum(csmi_job_type_t,allocation->job_type));
	    	printf("user_name:                      %s\n", allocation->user_name);
	    	printf("user_id:                        %" PRId32 "\n", allocation->user_id);
	    	printf("user_group_id:                  %" PRId32 "\n", allocation->user_group_id);
	    	printf("user_script:                    %s\n", allocation->user_script);
	    	printf("begin_time:                     %s\n", allocation->begin_time);
	    	printf("account:                        %s\n",allocation->account);
	    	printf("comment:                        %s\n", allocation->comment);
	    	printf("job_name:                       %s\n", allocation->job_name);
	    	printf("job_submit_time:                %s\n", allocation->job_submit_time);
	    	printf("queue:                          %s\n", allocation->queue);
	    	printf("requeue:                        %s\n", allocation->requeue);
	    	printf("time_limit:                     %" PRId64 "\n", allocation->time_limit);
	    	printf("wc_key:                         %s\n", allocation->wc_key);
	    	printf("isolated_cores:                 %d\n", allocation->isolated_cores);
	    	if (allocation->history) {
	    		printf("history:\n");
	    		printf("   end_time:                %s\n", allocation->history->end_time);
	    		printf("   exit_status:             %" PRId32 "\n", allocation->history->exit_status);
	    		printf("   archive_history_time:    %s\n", allocation->history->archive_history_time);
	    	}
	    	puts("...");
            break;
        }

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
