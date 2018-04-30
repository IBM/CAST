/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_query_details.c

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

#include <assert.h>

struct option longopts[] = {
  {"help", no_argument, 0, 'h'} 
};

static void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s id\n", argv0);
  fprintf(stderr, "       where:\n");
  fprintf(stderr, "              id is the id of the allocation to query (required)\n");
  // exit(EXIT_FAILURE);
  exit(EXIT_SUCCESS);
}

int csmi_client(int argc, char *argv[])
{
  int                i;
  int                ch;
  int                indexptr = 0;
  int                retval;
  csm_api_object    *csm_obj = NULL;
  csm_allocation_query_details_input_t input; 
  csm_allocation_query_details_output_t *output;
  input.allocation_id = 0;

  assert (csm_init_lib() == 0);

  while ((ch = getopt_long(argc, argv, "h", longopts, &indexptr)) != -1) {
    switch (ch) {
      case 'h':
        usage(argv[0]);
        break;
      default:
        usage(argv[0]);
        break;
    }
  }

  if (argc != 2)
    usage(argv[0]);

  for (i = optind; i < argc; i++) {
    input.allocation_id = (uint64_t)atol(argv[i]);
  }

  retval = csm_allocation_query_details(&csm_obj, &input, &output);

  if (retval == 0) {
    csmi_allocation_details_t *ad = output->allocation_details;
    printf("num_steps: %" PRIu32 "\n", ad->num_steps);
    if (ad->num_steps > 0) {
      printf("steps:\n");
      for (i = 0; i < ad->num_steps; i++) {
        printf(" - step_id: %" PRIu64 "\n", ad->steps[i]->step_id);
        printf("   num_nodes: %" PRIu32 "\n", ad->steps[i]->num_nodes);
        if (ad->steps[i]->num_nodes > 0) {
          printf("   compute_nodes:\n");
          char *nodeStr = strtok(ad->steps[i]->compute_nodes, ",");
          while (nodeStr != NULL) {
            printf("    - %s\n", nodeStr);
            nodeStr = strtok(NULL, ",");
          }
        }
      }
    }
  }
  else {
    printf("%s FAILED %d %s\n", argv[0], csm_api_object_errcode_get(csm_obj),
           csm_api_object_errmsg_get(csm_obj));
  }

  // it's the csmi library's responsibility to free internal space
  csm_api_object_destroy(csm_obj);

  assert( csm_term_lib() == 0);

  return 0;
}
