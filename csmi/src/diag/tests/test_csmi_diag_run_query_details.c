/*================================================================================

    csmi/src/diag/tests/test_csmi_diag_run_query_details.c

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

#include "csmi/include/csm_api_diagnostics.h"

//for logging
#include "csmutil/include/csmutil_logging.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
	puts("############");
	puts("#Begin code inside 'csmi_client' of 'test_csmi_diag_run_query_details.c'");
	
	csmutil_logging_level_set("trace");
	//csmutil_logging_level_set("debug");
	
	/*create csm object*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int returnValue = 0;

	/*establish connection*/
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	returnValue = csm_init_lib();
	if( returnValue != 0){
		fprintf(stderr, "# ERROR: csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?\n", returnValue);
		return returnValue;            
	}
	
	/*Set up test data*/
    csm_diag_run_query_details_input_t args;
    args.run_id = 0;
    csm_diag_run_query_details_output_t * output = NULL;
	
	printf("# SQL Query Argument Data:\n");
	printf("#   run_id: %lu\n",              args.run_id);
	//printf("#   run_id: %s\n",              arguments->run_id);
	
	puts("# pre-call:");
	printf("#   address of runData:      %p\n", &output);
	printf("#   value of resultData:     %p\n", output);
	
	
	puts("#about to call csm_diag_run_query_details");
	puts("#####");
	
	
	/*Calling the actual API*/
	returnValue = csm_diag_run_query_details( &csm_obj, &args, &output );
	//release_args(arguments);
	if(returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  errcode=%d errmsg=\"%s\"", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
		//CLEAN UP MEMORY
		// for(i = 0; i < dataCount; i++){
			// release_result(results[i]);
		// }
		// free(results);
		return returnValue;
	}else{

        csmi_diag_run_t *runData = output->run_data;
        csmi_diag_run_query_details_result_t **resultData = output->details;
	    
	    puts("---");
	    if(output) {
	    	if(runData){
	    		printf("runData:\n");
	    		printf("  history_time:     %s \n", runData->history_time);
	    		printf("  run_id:           %"PRId64" \n", runData->run_id);
	    		printf("  allocation_id:    %"PRId64" \n", runData->allocation_id);
	    		printf("  begin_time:       %s \n", runData->begin_time);
	    		printf("  end_time:         %s \n", runData->end_time);
	    		printf("  diag_status:      %s \n", runData->diag_status);
	    		printf("  inserted_ras:     %c \n", runData->inserted_ras);
	    		printf("  log_dir:          %s \n", runData->log_dir);
	    		printf("  cmd_line:         %s \n", runData->cmd_line);
	    		printf("Total_Result_Records: %"PRId64"\n", output->num_details);
	    		uint32_t i = 0;
	    		for(i = 0; i < output->num_details; i++){
	    			printf("Result_Record_%u:\n", i+1);
	    			printf("  history_time:     %s\n",  resultData[i]->history_time);
	    			printf("  run_id:           %"PRId64" \n",  resultData[i]->run_id);
	    			printf("  test_name:        %s\n",  resultData[i]->test_name);
	    			printf("  node_name:        %s\n",  resultData[i]->node_name);
	    			printf("  serial_number:    %s\n",  resultData[i]->serial_number);
	    			printf("  begin_time:       %s\n",  resultData[i]->begin_time);
	    			printf("  end_time:         %s\n",  resultData[i]->end_time);
	    			printf("  status:           %s\n",  resultData[i]->status);
	    			printf("  log_file:         %s\n",  resultData[i]->log_file);
	    		}
	    	}
	    }
        else {
            puts("# No records found.");
        }
        puts("...");
    }
	
	puts("#####");
	puts("#called csm_diag_run_query_details");
	
	//CLEAN UP MEMORY
	// for(i = 0; i < dataCount; i++){
		// release_result(results[i]);
	// }
	// free(results);
    csm_api_object_destroy(csm_obj);
	
	/* terminate connection */
	/* does the cleanup needed after calling csm_init_lib */
	returnValue = csm_term_lib();
	if(returnValue != 0){
		fprintf(stderr, "# ERROR: csm_term_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?\n", returnValue);
		return returnValue;
	}
	
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_diag_run_query.c'");
	puts("############");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	
	return 0;
}
