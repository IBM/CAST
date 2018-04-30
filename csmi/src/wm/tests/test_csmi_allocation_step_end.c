/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_step_end.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota
* Email: nbuonar@us.ibm.com
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>     // Provides gettimeofday()

#include "csmi/include/csm_api_workload_manager.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
	puts("############");
	puts("#Begin code inside 'csmi_client' of 'test_csmi_allocation_step_end.c'");
	
	csm_api_object *csm_obj = NULL;
	assert(csm_init_lib() == 0);
	
	struct timeval now_tv;
	gettimeofday(&now_tv, NULL);
	
	/*SET UP VALUES FOR TEST*/
    csm_allocation_step_end_input_t input;
    csmi_allocation_step_history_t hist;
	input.step_id = 1;
	input.allocation_id = 1;
	hist.exit_status = 1;
	hist.error_message = "PRE-ALPHA HARDCODED TEXT: error";
	hist.cpu_stats = "PRE-ALPHA HARDCODED TEXT: cpu_good";
	hist.total_u_time = now_tv.tv_sec;
	hist.total_s_time = 0;
	hist.omp_thread_limit = "PRE-ALPHA HARDCODED TEXT: t_num_threads_good";
	hist.gpu_stats = "PRE-ALPHA HARDCODED TEXT: gpu_s_good";
	hist.memory_stats = "PRE-ALPHA HARDCODED TEXT: mem_sts_good";
	hist.max_memory = 1;
	hist.io_stats = "PRE-ALPHA HARDCODED TEXT: io_sts_good";
	
    input.history = &hist;
	
	puts("#about to call csm_allocation_step_end");
	puts("#####");
	/*Calling the actual API*/
	int retval = 0;
	retval = csm_allocation_step_end(&csm_obj, &input);
	if(retval != 0){
		printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
	}

	
	puts("#####");
	puts("#called csm_allocation_step_end");
	
	assert( csm_term_lib() == 0);
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_allocation_step_end.c'");
	puts("############");
	
	return 0;	
}
