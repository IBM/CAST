/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_create.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "csmi/include/csm_api_workload_manager.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
  int                i;
  int                retval;
  csmi_allocation_t   allocation;
  char             **compute_nodes;
  csm_api_object    *csm_obj = NULL;
  time_t             t;
  struct tm         *tm;
  char               tbuf[32];

  assert (csm_init_lib() == 0);

  memset(&allocation, 0, sizeof(csmi_allocation_t));

  printf("\ncsm_allocation_create:\n");

  allocation.primary_job_id = 1;
  allocation.secondary_job_id = 2;
  allocation.ssd_file_system_name = strdup("file system 1");
  allocation.launch_node_name = strdup("launch node 1");
  allocation.user_flags = strdup("alloc check cmd");
  allocation.system_flags = strdup("dealloc check cmd");
  allocation.ssd_min = 100000000;
  allocation.ssd_max = 100000000;
  allocation.num_nodes = 2;
  allocation.num_processors = 2;
  allocation.num_gpus = 4;
  allocation.projected_memory = 32000000;
  allocation.state = CSM_RUNNING;
  allocation.type = CSM_USER_MANAGED;
  allocation.job_type = CSM_BATCH;
  allocation.user_name = strdup("user1");
  allocation.user_id = 3;
  allocation.user_group_id = 6;
  allocation.user_script = strdup("user script 1");
  allocation.account = strdup("account 1");
  allocation.comment = strdup("comment 1");
  time(&t);
  tm = localtime(&t);
  strftime(tbuf, sizeof(tbuf), "%F %T", tm);
  allocation.job_name = strdup("job 1");
  allocation.job_submit_time = strdup(tbuf);
  allocation.queue = strdup("queue 1");
  allocation.requeue = strdup("requeue 1");
  allocation.time_limit = 1234;
  allocation.wc_key = strdup("wc_key 1");
  compute_nodes = (char **)malloc(sizeof(char *) * allocation.num_nodes);
  compute_nodes[0] = strdup("c931f03p18-vm02");
  compute_nodes[1] = strdup("c931f03p18-vm06");
  allocation.compute_nodes = compute_nodes;

  retval = csm_allocation_create(&csm_obj, &allocation);

  if (retval == 0) {
    printf("\tallocation_id: %" PRIu64 "\n", allocation.allocation_id);
  }
  else {
    printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", argv[0], csm_api_object_errcode_get(csm_obj),
           csm_api_object_errmsg_get(csm_obj));
  }

  // it's the csmi library's responsibility to free internal space
  csm_api_object_destroy(csm_obj);

  // it's the csmi client's responsibility to free compute_nodes
  for (i = 0; i < allocation.num_nodes; i++)
    free(compute_nodes[i]);
  free(compute_nodes);

  assert( csm_term_lib() == 0);

  return 0;
}
