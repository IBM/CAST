/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_step_query_active_all.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

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

/*For printing through the infrastructure*/
#include "csmutil/include/csmutil_logging.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
#if 0 
	csmutil_logging_level_set("trace");
	//csmutil_logging_level_set("debug");
	
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  inside 'csmi_client' of 'test_csmi_allocation_step_query_active_all.c'");
	
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int returnValue = 0;
	/*For for loops*/
	uint32_t i = 0;
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	returnValue = csm_init_lib();
	if( returnValue != 0){
		fprintf(stderr, "# ERROR: csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?\n", returnValue);
		return returnValue;            
	}
	
	/*test data*/
	csm_allocationStepQueryActiveAll_input_t* input = NULL;
	input = malloc(sizeof(csm_allocationStepQueryActiveAll_input_t));
	//set all data to 0
	//get rid of garbage
	memset(input, 0, sizeof(csm_allocationStepQueryActiveAll_input_t));
	input->allocation_id = 1;
	csm_allocationStepQueryActiveAll_output_t **output = NULL;
	uint32_t dataCount = 0;
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of input:       %p", input);
	csmutil_logging(debug, "  address of input:     %p", &input);
	csmutil_logging(debug, "  input->allocation_id: %lu", input->allocation_id);
	csmutil_logging(debug, "  value of output:      %p", output);
	csmutil_logging(debug, "  address of output:    %p", &output);
	csmutil_logging(debug, "  value of dataCount:   %u", dataCount);
	csmutil_logging(debug, "  address of dataCount: %p", &dataCount);
	
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  about to call allocation_step_query_active_all");
	
	returnValue = csm_allocation_step_query_active_all(&csm_obj, input, &output, &dataCount);
	//ToDo: release memory of input struct
	//release_args(input);
	if(returnValue != 0){
		csmutil_logging(error, "%s-%d: errcode=%d errmsg=\"%s\"", __FILE__, __LINE__, csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
		//CLEAN UP MEMORY
		return returnValue;
	}
	
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  returned from allocation_step_query_active_all");
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of output:      %p", output);
	csmutil_logging(debug, "  address of output:    %p", &output);
	csmutil_logging(debug, "  value of dataCount:   %u", dataCount);
	csmutil_logging(debug, "  address of dataCount: %p", &dataCount);
	
	puts("---");
	printf("Total_Records: %u\n", dataCount);
	if(dataCount > 0){
		for(i = 0; i < dataCount; i++){
			printf("Record_%u:\n", i+1);
			printf("  allocation_id:        %lu\n", output[i]->allocation_id);
			printf("  step_id:              %lu\n", output[i]->step_id);
			printf("  begin_time:           %s\n",  output[i]->begin_time);
			printf("  state:                %c\n",  output[i]->state);
			printf("  executable:           %s\n",  output[i]->executable);
			printf("  working_directory:    %s\n",  output[i]->working_directory);
			printf("  argument:             %s\n",  output[i]->argument);
			printf("  environment_variable: %s\n",  output[i]->environment_variable);
			printf("  seq_id:               %lu\n", output[i]->seq_id);
			printf("  num_nodes:            %u\n",  output[i]->num_nodes);
			printf("  num_processors:       %u\n",  output[i]->num_processors);
			printf("  num_gpus:             %u\n",  output[i]->num_gpus);
			printf("  num_memory:           %u\n",  output[i]->num_memory);
			printf("  num_tasks:            %u\n",  output[i]->num_tasks);
			printf("  user_flags:           %s\n",  output[i]->user_flags);
			printf("  system_flags:         %s\n",  output[i]->system_flags);
		}
	}else if(dataCount == 0){
		puts("# No records found.");
	}else{
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  *dataCount value expected to be >= 0");
		csmutil_logging(error, "  *dataCount value: %u", dataCount);
		csmutil_logging(error, "  errcode=%d errmsg=\"%s\"", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
		return 1;
	}
	puts("...");
	
	
	
	//CLEAN UP MEMORY
	for(i = 0; i < dataCount; i++){
		//release_result(results[i]);
	}
	free(output);
	
	/* does the cleanup needed after calling csm_init_lib */
	returnValue = csm_term_lib();
	if(returnValue != 0){
		fprintf(stderr, "# ERROR: csm_term_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?\n", returnValue);
		return returnValue;
	}
	
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_allocation_step_query.c'");
	puts("############");
#endif
	return 0;
}
