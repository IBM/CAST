/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_query_active_all.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/include/csm_api_workload_manager.h"
#include "csmutil/include/csmutil_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>


int csmi_client(int argc, char *argv[])
{
  int retval = 0;
  uint32_t i;
  uint32_t j;
  csm_api_object    *csm_obj = NULL;
  csm_allocation_query_active_all_input_t input;
  csm_allocation_query_active_all_output_t* output = NULL;
  csm_init_struct(csm_allocation_query_active_all_input_t, input);
  
  csmutil_logging_level_set("trace"); 

  csmutil_logging(trace, "%s-%d: csm_allocation_query_active_all Begin.", __FILE__, __LINE__);

  assert (csm_init_lib() == 0);

  retval = csm_allocation_query_active_all( &csm_obj, &input, &output);

  csmi_allocation_t **allocations = output->allocations;
  for ( i = 0; i < output->num_allocations; i = i + 1 )
  {
        csmutil_logging(debug, "Allocation #%d", i);
        printf("\n");
	    printf("---\n");
	    printf("allocation_id:                  %" PRIu64 "\n", allocations[i]->allocation_id);
        printf("primary_job_id:                 %" PRIu64 "\n", allocations[i]->primary_job_id);
        printf("secondary_job_id:               %" PRIu32 "\n", allocations[i]->secondary_job_id);
        printf("num_nodes:                      %" PRIu32 "\n", allocations[i]->num_nodes);
        if (allocations[i]->num_nodes > 0) {
          printf("compute_nodes:\n");
          for (j = 0; j  < allocations[i]->num_nodes; j++) {
            printf(" - %s\n", allocations[i]->compute_nodes[j]);
          }
        }
        printf("ssd_file_system_name:               %s\n", allocations[i]->ssd_file_system_name);
        printf("launch_node_name:               %s\n", allocations[i]->launch_node_name);
        printf("user_flags:      %s\n", allocations[i]->user_flags);
        printf("system_flags:    %s\n", allocations[i]->system_flags);
        printf("ssd_min:                        %" PRIu64 "\n", allocations[i]->ssd_min);
        printf("ssd_max:                        %" PRIu64 "\n", allocations[i]->ssd_max);
        printf("num_processors:                 %" PRIu32 "\n", allocations[i]->num_processors);
        printf("num_gpus:                       %" PRIu32 "\n", allocations[i]->num_gpus);
        printf("projected_memory:               %" PRIu32 "\n", allocations[i]->projected_memory);
        printf("state:                          %s\n", csm_get_string_from_enum(csmi_state_t,allocations[i]->state));
        printf("type:                           %s\n", csm_get_string_from_enum(csmi_allocation_type_t, allocations[i]->type));
        printf("job_type:                       %s\n", csm_get_string_from_enum(csmi_job_type_t,allocations[i]->job_type));
        printf("user_name:                      %s\n", allocations[i]->user_name);
        printf("user_id:                        %" PRIu32 "\n", allocations[i]->user_id);
        printf("user_group_id:                  %" PRIu32 "\n", allocations[i]->user_group_id);
        printf("user_script:                    %s\n", allocations[i]->user_script);
        printf("begin_time:                     %s\n", allocations[i]->begin_time);
        printf("account:                        %s\n", allocations[i]->account);
        printf("comment:                        %s\n", allocations[i]->comment);
        printf("job_name:                       %s\n", allocations[i]->job_name);
        printf("job_submit_time:                %s\n", allocations[i]->job_submit_time);
        printf("queue:                          %s\n", allocations[i]->queue);
        printf("requeue:                        %s\n", allocations[i]->requeue);
        printf("time_limit:                     %" PRIu64 "\n", allocations[i]->time_limit);
        printf("wc_key:                         %s\n", allocations[i]->wc_key);
  }

    csm_api_object_destroy(csm_obj);

    assert( csm_term_lib() == 0);
    csmutil_logging(trace, "%s-%d: csm_allocation_query_active_all End.", __FILE__, __LINE__);

  return retval;
}
