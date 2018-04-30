/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_step_begin.c

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

#include "csmi/include/csm_api_workload_manager.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
	puts("############");
	puts("#Begin code inside 'csmi_client' of 'test_csmi_allocation_step_begin.c'");
	
	csm_api_object *csm_obj = NULL;
	assert(csm_init_lib() == 0);
	
	csmi_allocation_step_t myStep;
	/*for the csm_step table*/
	myStep.step_id = 1;
	myStep.allocation_id = 1;
	myStep.status= CSM_STEP_RUNNING;
	myStep.executable = "my executable";
	myStep.working_directory = "my working_directory";
	myStep.argument = "my argument";
	myStep.environment_variable = "my environment_variable";
	myStep.num_nodes = 1;
	myStep.num_processors = 1;
	myStep.num_gpus = 1;
	myStep.projected_memory = 512;
	myStep.num_tasks = 1;
	
	myStep.compute_nodes = (char**)calloc(myStep.num_nodes, sizeof(char*));
	/*for csm_step_node table*/
	myStep.compute_nodes[0] = "n09";
	
	puts("#about to call csm_allocation_step_begin");
	puts("#####");
	/*Calling the actual API*/
	int retval = 0;
	retval = csm_allocation_step_begin(&csm_obj, &myStep);
	if(retval != 0){
		printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
	}
		
	puts("#####");
	puts("#called csm_allocation_step_begin");
	
	assert( csm_term_lib() == 0);
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_allocation_step_begin.c'");
	puts("############");
	
	return 0;
}
