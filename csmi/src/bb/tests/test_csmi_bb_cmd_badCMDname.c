/*================================================================================

    csmi/src/bb/tests/test_csmi_bb_cmd_badCMDname.c

  © Copyright IBM Corporation 2015-2018`. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

/* CSM Includes */
#include "csmi/include/csm_api_burst_buffer.h"

/* CSM logging includes */
#include "csmutil/include/csmutil_logging.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  Entered test_csmi_bb_cmd_badCMDname.");
	
	//Quck Change for debug level of test case
	csmutil_logging_level_set("trace");
	//csmutil_logging_level_set("debug");
	//csmutil_logging_level_set("info");
	
	/*Helper Variables*/
	int returnValue = 0;
	/*For for loops*/
	int i = 0;

	/*Variables*/
	csm_api_object *csm_obj = NULL;
	csm_bb_cmd_input_t* input = NULL;
	/* CSM API initalise and malloc function*/
	csm_init_struct_ptr(csm_bb_cmd_input_t, input);
	csm_bb_cmd_output_t* output = NULL;
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of input:   %p", input);
	csmutil_logging(debug, "  address of input: %p", &input);
	csmutil_logging(debug, "  values in input:");
	csmutil_logging(debug, "    command_arguments: %s", input->command_arguments);
	csmutil_logging(debug, "    node_names_count:  %i", input->node_names_count);
	csmutil_logging(debug, "    node_names:        %p", input->node_names);
	for(i = 0; i < input->node_names_count; i++){
		csmutil_logging(debug, "      node_names[%i]: %s", i, input->node_names[i]);
	}
	csmutil_logging(debug, "    value of output:   %p", output);
	csmutil_logging(debug, "    address of output: %p", &output);
	
	//Set values.
	input->command_arguments = NULL;
	input->node_names_count = 1;
	input->node_names = (char**)calloc(input->node_names_count, sizeof(char*));
	input->node_names[0] = strdup("c650f03p31-vm03");
	
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  value of input:   %p", input);
	csmutil_logging(debug, "  address of input: %p", &input);
	csmutil_logging(debug, "  values in input:");
	csmutil_logging(debug, "    command_arguments: %s", input->command_arguments);
	csmutil_logging(debug, "    node_names_count:  %i", input->node_names_count);
	csmutil_logging(debug, "    node_names:        %p", input->node_names);
	for(i = 0; i < input->node_names_count; i++){
		csmutil_logging(debug, "      node_names[%i]: %s", i, input->node_names[i]);
	}
	csmutil_logging(debug, "    value of output:   %p", output);
	csmutil_logging(debug, "    address of output: %p", &output);

	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	returnValue = csm_init_lib();
	if( returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", returnValue);
		return returnValue;            
	}
	
	/*Call the API*/
	returnValue = csm_bb_cmd(&csm_obj, input, &output);
	//Use CSM API free to release the input. We no longer need it.
	csm_free_struct_ptr(csm_bb_cmd_input_t, input);
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
		csmutil_logging(debug, "  csm_bb_cmd has completed successfully!");
		
		csmutil_logging(debug, "  value of output:   %p", output);
		csmutil_logging(debug, "  address of output: %p", &output);
		csmutil_logging(debug, "  values in output:");
		csmutil_logging(debug, "    command_output:   %s", output->command_output);
		
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
	puts("#End code inside 'csmi_client' of 'test_csmi_bb_cmd_badCMDname.c'");
	puts("############");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	puts("#");
	
	return 0;
}
