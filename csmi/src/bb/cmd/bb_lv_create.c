/*================================================================================

    csmi/src/bb/cmd/bb_lv_create.c

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

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
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
/*CORAL includes*/
#include "utilities/include/string_tools.h"
/*CSM Include*/
#include "csmi/include/csm_api_burst_buffer.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"

/* Should we do this? */
#define API_PARAMETER_INPUT_TYPE csm_bb_lv_create_input_t
//#define API_PARAMETER_OUTPUT_TYPE csm_bb_lv_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help() {
	puts("_____CSM_BB_LV_CREATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_bb_lv_create ARGUMENTS [OPTIONS]");
	puts("  csm_bb_lv_create -a allocation_id -c current_size -f file_system_mount -F file_system_type -l logical_volume_name -n node_name -s state -V vg_name [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used by to create an LV.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_bb_lv_create expects 8 mandatory argument");
	puts("    Argument                  | Example value | Description  ");                                                 
	puts("    --------------------------|---------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -a, --allocation_id       | 1             | (INT64) Unique identifier for this allocation.");
	puts("    -c, --current_size        | 500           | (INT64) Current size (in bytes).");
	puts("    -f, --file_system_mount   | \"mount\"       | (STRING) Identifies the file system and mount point.");
	puts("    -F, --file_system_type    | \"type\"        | (STRING) Identifies the file system and its partition.");
	puts("    -l, --logical_volume_name | \"lv_01\"       | (STRING) Unique identifier for this ssd partition.");
	puts("    -n, --node_name           | \"node_01\"     | (STRING) Node a part of this group.");
	puts("    -s, --state               | 'C'           | (CHAR) *C*reated, *M*ounted, *S*hrinking, or *R*emoved ");
	puts("    -V, --vg_name             | \"vg_01\"       | (STRING) Volume group name. ");
	
	puts("GENERAL OPTIONS:");
	puts("[-h]                  | Help.");
	puts("[-v verbose_level]    | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_bb_lv_create -a 1 -c 500 -f \"mount\" -F \"type\" -l \"lv_01\" -n \"node_01\" -s C -V \"vg_01\" ");
	puts("____________________");
}

static struct option long_options[] =
{
	//general options
	{"help",                no_argument,       0, 'h'},
	{"verbose",             required_argument, 0, 'v'},
	//api arguments
	{"allocation_id",       required_argument, 0, 'a'},
	{"current_size",        required_argument, 0, 'c'},
	{"file_system_mount",   required_argument, 0, 'f'},
	{"file_system_type",    required_argument, 0, 'F'},
	{"logical_volume_name", required_argument, 0, 'l'},
	{"node_name",           required_argument, 0, 'n'},
	{"state",               required_argument, 0, 's'},
	{"vg_name",             required_argument, 0, 'V'},
	{0, 0, 0, 0}
};

/*
* Summary: Simple command line interface for the CSM API 'bb lv delete'. 
*			Works as interface between a system administrator and the CSM DB.
* 			Takes in the logical volume name and allocation_id via command line parameters, and deletes the matching data in the CSM database.
*/
int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int returnValue = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 8;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	//uint32_t i = 0;

    char *arg_check = NULL; ///< Used in verifying the long arg values.
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	//API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while((opt = getopt_long (argc, argv, "hv:a:c:f:F:l:n:s:V:", long_options, &indexptr)) != -1){
		switch(opt){
			/*Single char common options. */
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
                csm_optarg_test( "-a, --allocation_id", optarg, USAGE )
                csm_str_to_int64( input->allocation_id, optarg, arg_check, "-a, --allocation_id", USAGE )
				requiredParameterCounter++;
				break;
			case 'c':
                csm_optarg_test( "-c, --current_size", optarg, USAGE )
                csm_str_to_int64( input->current_size, optarg, arg_check, "-c, --current_size", USAGE )
				requiredParameterCounter++;
				break;
			case 'f':
                csm_optarg_test( "-f, --file_system_mount", optarg, USAGE )
                input->file_system_mount = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 'F':
                csm_optarg_test( "-F, --file_system_type", optarg, USAGE )
                input->file_system_type = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 'l':
				/*logical_volume_name*/
                csm_optarg_test( "-l, --logical_volume_name", optarg, USAGE )
                input->logical_volume_name = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 'n':
				/*node_name*/
                csm_optarg_test( "-n, --node_name", optarg, USAGE )
                input->node_name = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 's':
                csm_optarg_test( "-s, --state", optarg, USAGE )
                input->state = optarg[0];
				requiredParameterCounter++;
				break;
			case 'V':
                csm_optarg_test( "-V, --vg_name", optarg, USAGE )
                input->vg_name = strdup(optarg);
				requiredParameterCounter++;
				break;
			default:
				fprintf(stderr, "unknown arg: '%c'\n", opt);
                USAGE();
				return CSMERR_INVALID_PARAM;
		}
	}
	
	/*Handle command line args*/
	argc -= optind;
	argv += optind;
	
	/*Collect mandatory args*/
	/*Check to see if expected number of arguments is correct.*/
	if(requiredParameterCounter < NUMBER_OF_REQUIRED_ARGUMENTS || optionalParameterCounter < MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS){
		/*We don't have the correct number of needed arguments passed in.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s).");
		csmutil_logging(error, "    Encountered %i required parameter(s). Expected %i required parameter(s).", requiredParameterCounter, NUMBER_OF_REQUIRED_ARGUMENTS);
		csmutil_logging(error, "    Encountered %i optional parameter(s). Expected at least %i optional parameter(s).", optionalParameterCounter, MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS);
        USAGE();
		return CSMERR_MISSING_PARAM;
	}
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	returnValue = csm_init_lib();
	if( returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", returnValue);
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return returnValue;            
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:    %p", input);
	csmutil_logging(debug, "  address of input:  %p", &input);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    allocation_id:       %"PRId64, input->allocation_id);
	csmutil_logging(debug, "    current_size:        %"PRId64, input->current_size);
	csmutil_logging(debug, "    file_system_mount:   %s", input->file_system_mount);
	csmutil_logging(debug, "    file_system_type:    %s", input->file_system_type);
	csmutil_logging(debug, "    logical_volume_name: %s", input->logical_volume_name);
	csmutil_logging(debug, "    node_name:           %s", input->node_name);
	csmutil_logging(debug, "    state:               %c", input->state);
	csmutil_logging(debug, "    vg_name:             %s", input->vg_name);
	
	/*All that just to call the api.*/
	returnValue = csm_bb_lv_create(&csm_obj, input);
	
	switch( returnValue )
    {
		case CSMI_SUCCESS:
            printf("---\n# The lv: \"%s\" has been successfully created on node: \"%s\" for allocation_id: %"PRId64".\n...\n", input->logical_volume_name, input->node_name, input->allocation_id);
            break;
		default:
            printf("%s FAILED: errcode: %d errmsg: %s\n", argv[0], returnValue,  csm_api_object_errmsg_get(csm_obj));
			csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
			csmutil_logging(error, "  Encountered an error in the api.");
			csmutil_logging(error, "  returnValue: %i", returnValue);
			csmutil_logging(error, "  errcode:     %d", csm_api_object_errcode_get(csm_obj));
			csmutil_logging(error, "  errmsg:      \"%s\"", csm_api_object_errmsg_get(csm_obj));
			break;
	}
	
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
    csm_api_object_destroy(csm_obj);
	
	/* Does the cleanup needed after calling csm_init_lib */
	int lib_returnValue = csm_term_lib();
	if(lib_returnValue != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_term_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", lib_returnValue);
		return lib_returnValue;
	}
	
	return returnValue;
}