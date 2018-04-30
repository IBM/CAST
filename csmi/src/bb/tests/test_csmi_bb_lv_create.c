/*================================================================================

    csmi/src/bb/tests/test_csmi_bb_lv_create.c

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
/*C Include*/
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
/*CSM Include*/
#include "csmi/include/csm_api_burst_buffer.h"
/*For printing through the infrastructure*/
#include "csmutil/include/csmutil_logging.h"

#define API_PARAMETER_INPUT_TYPE csm_bb_lv_create_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_BbLvCreate_output_t

/*
* Program to test the csm_bb_lv_create CSM API. 
*/
int csmi_client(int argc, char *argv[])
{
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  Entered test_csmi_bb_lv_create.");
	
	//Quck Change for debug level of test case
	csmutil_logging_level_set("trace");
	//csmutil_logging_level_set("debug");
	//csmutil_logging_level_set("info");
	
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int returnValue = 0;
	/*For for loops*/
	//uint32_t i = 0;

	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of input:   %p", input);
	csmutil_logging(debug, "  address of input: %p", &input);
	
	/* CSM API initalise and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of input:   %p", input);
	csmutil_logging(debug, "  address of input: %p", &input);
	csmutil_logging(debug, "  input default values:");
	csmutil_logging(debug, "    allocation_id:       %lli", input->allocation_id);
	csmutil_logging(debug, "    current_size:        %lli", input->current_size);
	csmutil_logging(debug, "    file_system_mount:   %s",   input->file_system_mount);
	csmutil_logging(debug, "    file_system_type:    %s",   input->file_system_type);
	csmutil_logging(debug, "    logical_volume_name: %s",   input->logical_volume_name);
	csmutil_logging(debug, "    node_name:           %s",   input->node_name);
	csmutil_logging(debug, "    state:               %c",   input->state);
	csmutil_logging(debug, "    vg_name:             %s",   input->vg_name);
	
	/*Input the values we wat for our test. */
	input->allocation_id = 1;
	input->current_size = 512;
	free(input->file_system_mount);
	input->file_system_mount = strdup("No file system mount.");
	free(input->file_system_type);
	input->file_system_type = strdup("No file system type.");
	free(input->logical_volume_name);
	input->logical_volume_name = strdup("lv_01");
	free(input->node_name);
	input->node_name = strdup("n01");
	input->state = 'C';
	free(input->vg_name);
	input->vg_name = strdup("vg_01");
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	returnValue = csm_init_lib();
	if( returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", returnValue);
		csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return returnValue;            
	}
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:   %p", input);
	csmutil_logging(debug, "  address of input: %p", &input);
	csmutil_logging(debug, "  input default values:");
	csmutil_logging(debug, "    allocation_id:       %lli", input->allocation_id);
	csmutil_logging(debug, "    current_size:        %lli", input->current_size);
	csmutil_logging(debug, "    file_system_mount:   %s",   input->file_system_mount);
	csmutil_logging(debug, "    file_system_type:    %s",   input->file_system_type);
	csmutil_logging(debug, "    logical_volume_name: %s",   input->logical_volume_name);
	csmutil_logging(debug, "    node_name:           %s",   input->node_name);
	csmutil_logging(debug, "    state:               %c",   input->state);
	csmutil_logging(debug, "    vg_name:             %s",   input->vg_name);
	
	/*Call the API*/
	returnValue = csm_bb_lv_create(&csm_obj, input);
	/*Use CSM API helper free to release input. We no longer need it.*/
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	if(returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Encountered an error in the api.");
		csmutil_logging(error, "  returnValue: %i", returnValue);
		csmutil_logging(error, "  errcode:     %d", csm_api_object_errcode_get(csm_obj));
		csmutil_logging(error, "  errmsg:      \"%s\"", csm_api_object_errmsg_get(csm_obj));
		return returnValue;
	}else if(returnValue == 0){
		csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(debug, "  csm_bb_lv_create has completed successfully!");
	}else{
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Encountered a critical error. returnValue corrupted.");
	}
	
	/* does the cleanup needed after calling csm_init_lib */
	returnValue = csm_term_lib();
	if(returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_term_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", returnValue);
		return returnValue;
	}
	
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_bb_lv_create.c'");
	puts("############");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	
	return 0;
}
