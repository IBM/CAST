/*================================================================================

    csmi/src/wm/cmd/allocation_query_details.c

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

#define API_PARAMETER_INPUT_TYPE  csm_allocation_query_details_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_allocation_query_details_output_t

///< For use as the usage variable in the input parsers.
#define USAGE help

struct option longopts[] = {
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	{"allocation_id", required_argument, 0, 'a'}, 
	{0,0,0,0}
};

static void help()
{
	puts("_____CSM_ALLOCATION_QUERY_DETAILS_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_query_details ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_query_details -a allocation_id [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to get detailed information about a specific allocation based off a supplied id.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_allocation_query_details expects 1 mandatory argument");
	puts("    Argument                | Example value | Description  ");                                                 
	puts("    ------------------------|---------------|--------------");
	puts("    -a, --allocation_id     | 1             | (LONG INTEGER) Unique identifier for an allocation.");
	puts("                            |               | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_query_details -a 1");
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
    	input.allocation_id = 0;
	
	while ((opt = getopt_long(argc, argv, "hv:a:", longopts, &indexptr)) != -1) {
		switch (opt) {
		case 'h':
            USAGE();
            return CSMI_HELP;
		case 'v':
            csm_set_verbosity( optarg, USAGE )
			break;
		case 'a':
            csm_optarg_test( "-a, --allocation", optarg, USAGE )
            csm_str_to_int64(input.allocation_id, optarg, arg_check, "-a, --allocation", USAGE)
			break;
		default:
            csmutil_logging(error, "unknown arg: '%c'\n", opt);
            USAGE();
            return CSMERR_INVALID_PARAM;
		}
	}

	if (input.allocation_id == 0)
    {
        USAGE();
		return CSMERR_INVALID_PARAM;
	}

	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
		return return_value;            
	}

	return_value = csm_allocation_query_details(&csm_obj, &input, &output);

    switch(return_value)
    {
        case CSMI_SUCCESS:
        {
            csmi_allocation_t *a = output->allocation;
            csmi_allocation_details_t *ad = output->allocation_details;
	    	// first, print the basic allocation data
	    	printf("---\n");
	    	printf("allocation_id:                  %" PRId64 "\n", a->allocation_id);
	    	printf("primary_job_id:                 %" PRId64 "\n", a->primary_job_id);
	    	printf("secondary_job_id:               %" PRId32 "\n", a->secondary_job_id);
	    	printf("num_nodes:                      %" PRId32 "\n", a->num_nodes);
	    	if (a->num_nodes > 0) {
	    		printf("compute_nodes:\n");
	    		for (i = 0; i < a->num_nodes; i++) 
                {
	    			printf("  %s:\n", a->compute_nodes[i]);
                    
                    // Only print non negative accounting data.
                    if( ad->node_accounting[i] )
                    {
                        printf("   ib_rx:                       %"PRId64 "\n",
                            ad->node_accounting[i]->ib_rx);

                        printf("   ib_tx:                       %"PRId64 "\n",
                            ad->node_accounting[i]->ib_tx);

                        // ======================================================

                        printf("   gpfs_read:                   %"PRId64 "\n", 
                            ad->node_accounting[i]->gpfs_read);

                        printf("   gpfs_write:                  %"PRId64 "\n",
                            ad->node_accounting[i]->gpfs_write);

                        printf("   energy_consumed:             %"PRId64 "\n",
                            ad->node_accounting[i]->energy_consumed);

                        printf("   power_cap:                   %"PRId32 "\n",
                            ad->node_accounting[i]->power_cap);

                        printf("   power_shifting_ratio:        %"PRId32 "\n",
                            ad->node_accounting[i]->power_shifting_ratio);

                        printf("   power_cap_hit:               %"PRId64 "\n",
                            ad->node_accounting[i]->power_cap_hit );

                        printf("   gpu_energy:                  %"PRId64 "\n",
                            ad->node_accounting[i]->gpu_energy);

                        printf("   gpu_usage:                   %"PRId64 "\n",
                            ad->node_accounting[i]->gpu_usage);

                        printf("   cpu_usage:                   %"PRId64 "\n",
                            ad->node_accounting[i]->cpu_usage);

                        printf("   memory_usage_max:            %"PRId64 "\n",
                            ad->node_accounting[i]->memory_usage_max);
                    }
	    		}
	    	}

            printf("num_transitions:                %" PRId32 "\n", ad->num_transitions);
            if ( ad->num_transitions > 0 )
            {
	    		printf("state_transitions:\n");
                for ( i=0; i < ad->num_transitions; ++i)
                {
                    if( ad->state_transitions[i] )
                    {
                        printf("  - history_time:            %s\n",
                            ad->state_transitions[i]->history_time);
                        printf("    state:                   %s\n", 
                            csm_get_string_from_enum(csmi_state_t, ad->state_transitions[i]->state));
                    }
                }
            }

	    	printf("ssd_file_system_name:           %s\n", a->ssd_file_system_name);
	    	printf("launch_node_name:               %s\n", a->launch_node_name);
	    	printf("user_flags:                     %s\n", a->user_flags);
	    	printf("system_flags:                   %s\n", a->system_flags);
	    	printf("smt_mode:                       %" PRId16 "\n", a->smt_mode);
	    	printf("ssd_min:                        %" PRId64 "\n", a->ssd_min);
	    	printf("ssd_max:                        %" PRId64 "\n", a->ssd_max);
	    	printf("num_processors:                 %" PRId32 "\n", a->num_processors);
	    	printf("num_gpus:                       %" PRId32 "\n", a->num_gpus);
	    	printf("projected_memory:               %" PRId32 "\n", a->projected_memory);
            printf("state:                          %s\n", csm_get_string_from_enum(csmi_state_t, a->state));
	    	printf("type:                           %s\n", csm_get_string_from_enum(csmi_allocation_type_t, a->type));
	    	printf("job_type:                       %s\n", csm_get_string_from_enum(csmi_job_type_t, a->job_type));
	    	printf("user_name:                      %s\n", a->user_name);
	    	printf("user_id:                        %" PRId32 "\n", a->user_id);
	    	printf("user_group_id:                  %" PRId32 "\n", a->user_group_id);
	    	printf("user_script:                    %s\n", a->user_script);
	    	printf("begin_time:                     %s\n", a->begin_time);
	    	printf("account:                        %s\n", a->account);
	    	printf("comment:                        %s\n", a->comment);
	    	printf("job_name:                       %s\n", a->job_name);
	    	printf("job_submit_time:                %s\n", a->job_submit_time);
	    	printf("queue:                          %s\n", a->queue);
	    	printf("requeue:                        %s\n", a->requeue);
	    	printf("time_limit:                     %" PRId64 "\n", a->time_limit);
	    	printf("wc_key:                         %s\n", a->wc_key);
	    	printf("isolated_cores:                 %d\n", a->isolated_cores);
	    	if (a->history) {
	    		printf("history:\n");
	    		printf("   end_time:                %s\n", a->history->end_time);
	    		printf("   exit_status:             %" PRId32 "\n", a->history->exit_status);
	    		printf("   archive_history_time:    %s\n", a->history->archive_history_time);
	    	}

	    	// next, print the detailed allocation data
	    	printf("num_steps:      %" PRId32 "\n", ad->num_steps);
	    	if (ad->num_steps > 0) 
            {
	    		printf("steps:\n");
	    		for (i = 0; i < ad->num_steps; i++) 
                {
	    			printf(" - step_id:         %" PRId64 "\n", ad->steps[i]->step_id);

                    // Only print the end time if it was set.
                    if( ad->steps[i]->end_time != NULL  && ad->steps[i]->end_time[0] != 0)
                        printf("   end_time:        %s\n",  ad->steps[i]->end_time);
                    
	    			printf("   num_nodes:       %" PRId32 "\n", ad->steps[i]->num_nodes);
	    			if (ad->steps[i]->num_nodes > 0) 
                    {
	    				printf("   compute_nodes:\n");
	    				char *nodeStr = strtok(ad->steps[i]->compute_nodes, ",");
	    				while (nodeStr != NULL) 
                        {
	    					printf("    - %s\n", nodeStr);
	    					nodeStr = strtok(NULL, ",");
	    				}
	    			}
	    		}
	    	}
	    	printf("...\n");
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
                argv[0], return_value, csm_api_object_errmsg_get(csm_obj));
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
