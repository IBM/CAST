/*================================================================================

    csmi/src/bb/tests/test_csmi_bb_vg_create.c

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

/* C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* CSM API includes */
#include "csmi/include/csm_api_burst_buffer.h"

/* CSM logging includes */
#include "csmutil/include/csmutil_logging.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  Entered test_csmi_bb_vg_create.");
	
	//Quck Change for debug level of test case
	csmutil_logging_level_set("trace");
	//csmutil_logging_level_set("debug");
	//csmutil_logging_level_set("info");
	
	//Helper Variables*/
	uint32_t i = 0;

	/*Variables*/
	csm_api_object *csm_obj = NULL;
	csm_bb_vg_create_input_t* input = NULL;
	/* CSM API initalise and malloc function*/
	csm_init_struct_ptr(csm_bb_vg_create_input_t, input);
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of input:   %p", input);
	csmutil_logging(debug, "  address of input: %p", &input);
	csmutil_logging(debug, "  values in input:");
	csmutil_logging(debug, "    available_size: %lld", input->available_size);
	csmutil_logging(debug, "    node_name:      %s", input->node_name);
	csmutil_logging(debug, "    ssd_info_count: %ld", input->ssd_info_count);
	csmutil_logging(debug, "    ssd_info:       %p", input->ssd_info);
	for(i = 0; i < input->ssd_info_count; i++){
	csmutil_logging(debug, "      ssd_info[i]: %p", input->ssd_info[i]);
	csmutil_logging(debug, "        ssd_serial_number: %s", input->ssd_info[i]->ssd_serial_number);
	csmutil_logging(debug, "        ssd_allocation:    %lld", input->ssd_info[i]->ssd_allocation);
	}
	csmutil_logging(debug, "    total_size:     %lld", input->total_size);
	csmutil_logging(debug, "    vg_name:        %s", input->vg_name);
	
	//Set values.
	input->available_size = 256;
	input->node_name = strdup("n02");
	input->ssd_info_count = 3;
	input->ssd_info = (csmi_bb_vg_ssd_info_t**)calloc(input->ssd_info_count, sizeof(csmi_bb_vg_ssd_info_t*));
	for(i = 0; i < input->ssd_info_count; i++){
		//set up
		input->ssd_info[i] = NULL;
		csm_init_struct_ptr(csmi_bb_vg_ssd_info_t , input->ssd_info[i]);
	}
	//fill with test data
	input->ssd_info[0]->ssd_serial_number = strdup("abc456");
	input->ssd_info[0]->ssd_allocation = 500;
	input->ssd_info[1]->ssd_serial_number = strdup("abc789");
	input->ssd_info[1]->ssd_allocation = 500;
	input->ssd_info[2]->ssd_serial_number = strdup("xyz123");
	input->ssd_info[2]->ssd_allocation = 500;
	
	input->total_size = 1500;
	input->vg_name = strdup("volume_group_02");
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of input:   %p", input);
	csmutil_logging(debug, "  address of input: %p", &input);
	csmutil_logging(debug, "  values in input:");
	csmutil_logging(debug, "    available_size: %lld", input->available_size);
	csmutil_logging(debug, "    node_name:      %s", input->node_name);
	csmutil_logging(debug, "    ssd_info_count: %ld", input->ssd_info_count);
	csmutil_logging(debug, "    ssd_info:       %p", input->ssd_info);
	for(i = 0; i < input->ssd_info_count; i++){
	csmutil_logging(debug, "      ssd_info[i]: %p", input->ssd_info[i]);
	csmutil_logging(debug, "        ssd_serial_number: %s", input->ssd_info[i]->ssd_serial_number);
	csmutil_logging(debug, "        ssd_allocation:    %lld", input->ssd_info[i]->ssd_allocation);
	}
	csmutil_logging(debug, "    total_size:     %lld", input->total_size);
	csmutil_logging(debug, "    vg_name:        %s", input->vg_name);
	
	/*Helper Variables*/
	int returnValue = 0;
	/*For for loops*/
	//uint32_t i = 0;
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	returnValue = csm_init_lib();
	if( returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", returnValue);
        csm_free_struct_ptr(csm_bb_vg_create_input_t, input);
		return returnValue;            
	}
	
	/*Call the API*/
	returnValue = csm_bb_vg_create(&csm_obj, input);
	//Use CSM API free to release the input. We no longer need it.
	csm_free_struct_ptr(csm_bb_vg_create_input_t, input);
	if(returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Encountered an error in the api.");
		csmutil_logging(error, "  returnValue: %i", returnValue);
		csmutil_logging(error, "  errcode:     %d", csm_api_object_errcode_get(csm_obj));
		csmutil_logging(error, "  errmsg:      \"%s\"", csm_api_object_errmsg_get(csm_obj));
		//ToDO: CLEAN UP MEMORY
		return returnValue;
	}else if(returnValue == 0){
		csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(debug, "  csm_bb_vg_create has completed successfully!");
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
	puts("#End code inside 'csmi_client' of 'test_csmi_bb_vg_create.c'");
	puts("############");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	
	return 0;
}
