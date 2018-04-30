/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_step_query.c

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

/*For printing through the infrastructure*/
#include "csmutil/include/csmutil_logging.h"

#include <assert.h>
#if 0
void release_args(csm_allocation_step_query_args_t* arguments)
{
	if(arguments){
		//if(arguments->step_id){free(arguments->step_id);}
		//if(arguments->allocation_id){free(arguments->allocation_id);}
		free(arguments);
	}
}

void release_result(csm_allocation_step_query_result_t* resultRecord)
{
	if(resultRecord){
		if(resultRecord->history_time){free(resultRecord->history_time);}
		if(resultRecord->begin_time){free(resultRecord->begin_time);}
		if(resultRecord->end_time){free(resultRecord->end_time);}
		if(resultRecord->executable){free(resultRecord->executable);}
		if(resultRecord->working_directory){free(resultRecord->working_directory);}
		if(resultRecord->argument){free(resultRecord->argument);}
		if(resultRecord->environment_variable){free(resultRecord->environment_variable);}
		if(resultRecord->level_gpu_usage){free(resultRecord->level_gpu_usage);}
		if(resultRecord->err_text){free(resultRecord->err_text);}
		if(resultRecord->network_bandwidth){free(resultRecord->network_bandwidth);}
		if(resultRecord->cpu_stats){free(resultRecord->cpu_stats);}
		if(resultRecord->total_u_time){free(resultRecord->total_u_time);}
		if(resultRecord->total_s_time){free(resultRecord->total_s_time);}
		if(resultRecord->total_num_threads){free(resultRecord->total_num_threads);}
		if(resultRecord->gpu_stats){free(resultRecord->gpu_stats);}
		if(resultRecord->memory_stats){free(resultRecord->memory_stats);}
		if(resultRecord->max_memory){free(resultRecord->max_memory);}
		if(resultRecord->max_swap){free(resultRecord->max_swap);}
		if(resultRecord->io_stats){free(resultRecord->io_stats);}
		free(resultRecord);
	}
}
#endif
int csmi_client(int argc, char *argv[])
{
#if 0
	csmutil_logging_level_set("trace");
	//csmutil_logging_level_set("debug");
	
	puts("############");
	puts("#Begin code inside 'csmi_client' of 'test_csmi_allocation_step_query.c'");
	
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
	csm_allocation_step_query_args_t* arguments = NULL;
	arguments = malloc(sizeof(csm_allocation_step_query_args_t));
	//arguments->step_id = "1";
	arguments->allocation_id = "2";
	
	printf("# SQL Query Argument Data:\n");
	printf("#  step_id: %s\n",              arguments->step_id);
	printf("#  allocation_id: %s\n",        arguments->allocation_id);
	
	csm_allocation_step_query_result_t **results = NULL;
	uint32_t dataCount = 0;
	
	puts("#pre-call:");
	printf("# value of results: %p\n", results);
	printf("# address of results: %p\n", &results);
	
	puts("#about to call csm_allocation_step_query");
	puts("#####");
	
	returnValue = csm_allocation_step_query(&csm_obj, arguments, &results, &dataCount);
	release_args(arguments);
	if(returnValue != 0){
		csmutil_logging(error, "%s-%d: errcode=%d errmsg=\"%s\"", __FILE__, __LINE__, csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
		//CLEAN UP MEMORY
		for(i = 0; i < dataCount; i++){
			release_result(results[i]);
		}
		free(results);
		return returnValue;
	}
		
	puts("#####");
	puts("#called csm_allocation_step_query");
	
	
	puts("#returned:");
	printf("# value of results: %p\n", results);
	printf("# address of results: %p\n", &results);
	
	printf("Total_Records: %u\n", dataCount);
	if(dataCount > 0){
		for(i = 0; i < dataCount; i++){
			printf("Record_%u:\n", i+1);
			printf("  history_time:         %s\n",  results[i]->history_time);
			printf("  step_id:              %lu\n", results[i]->step_id);
			printf("  allocation_id:        %lu\n", results[i]->allocation_id);
			printf("  begin_time:           %s\n",  results[i]->begin_time);
			printf("  end_time:             %s\n",  results[i]->end_time);
			printf("  state:                %c\n",  results[i]->state);
			printf("  executable:           %s\n",  results[i]->executable);
			printf("  working_directory:    %s\n",  results[i]->working_directory);
			printf("  argument:             %s\n",  results[i]->argument);
			printf("  environment_variable: %s\n",  results[i]->environment_variable);
			printf("  seq_id:               %lu\n", results[i]->seq_id);
			printf("  num_nodes:            %u\n",  results[i]->num_nodes);
			printf("  num_processors:       %u\n",  results[i]->num_processors);
			printf("  num_gpus:             %u\n",  results[i]->num_gpus);
			printf("  num_memory:           %u\n",  results[i]->num_memory);
			printf("  num_tasks:            %u\n",  results[i]->num_tasks);
			printf("  level_gpu_usage:      %s\n",  results[i]->level_gpu_usage);
			printf("  exit_status:          %u\n",  results[i]->exit_status);
			printf("  err_text:             %s\n",  results[i]->err_text);
			printf("  network_bandwidth:    %s\n",  results[i]->network_bandwidth);
			printf("  cpu_stats:            %s\n",  results[i]->cpu_stats);
			printf("  total_u_time:         %s\n",  results[i]->total_u_time);
			printf("  total_s_time:         %s\n",  results[i]->total_s_time);
			printf("  total_num_threads:    %s\n",  results[i]->total_num_threads);
			printf("  gpu_stats:            %s\n",  results[i]->gpu_stats);
			printf("  memory_stats:         %s\n",  results[i]->memory_stats);
			printf("  max_memory:           %s\n",  results[i]->max_memory);
			printf("  max_swap:             %s\n",  results[i]->max_swap);
			printf("  io_stats:             %s\n",  results[i]->io_stats);
		}
	}else if(dataCount == 0){
		puts("# No records found.");
	}else{
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "*dataCount value expected to be >= 0");
		csmutil_logging(error, "*dataCount value: %u", dataCount);
		csmutil_logging(error, "errcode=%d errmsg=\"%s\"", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
		return -1;
	}
	
	
	
	//CLEAN UP MEMORY
	for(i = 0; i < dataCount; i++){
		release_result(results[i]);
	}
	free(results);
	
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
