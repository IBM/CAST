/*================================================================================

    csmi/src/bb/cmd/bb_lv_delete.c

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
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
/*CSM Include*/
#include "csmi/include/csm_api_burst_buffer.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"


#define API_PARAMETER_INPUT_TYPE csm_bb_lv_delete_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_BbLvDelete_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help() {
	puts("_____CSM_BB_LV_DELETE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_bb_lv_delete ARGUMENTS [OPTIONS]");
	puts("  csm_bb_lv_delete -a allocation_id -l logical_volume_name -n node_name -r num_bytes_read  -w num_bytes_written [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Intended to be used by a system administrator to manually remove an entry from the 'csm_bb_lv' table in the CSM database. This cmd line interface should be used in the event of an error occur causing an lv entry to not get deleted properly. This cmd line interface should not be used as the main way of removing entries from 'csm_bb_lv'.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_bb_lv_delete expects 5 mandatory arguments");
	puts("    Argument                  | Example value              | Description  ");                                                 
	puts("    --------------------------|----------------------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -a, --allocation_id       | 1                          | (INT64) Uniquely identify this allocation.");
	puts("    -l, --logical_volume_name | \"my logical volume name\"   | (STRING) Unique identifier for this ssd partition.");
	puts("    -n, --node_name           | \"node_01\"                  | (STRING) Name of the node where this LV is located.");
	puts("    -r, --num_bytes_read      | 1                          | (INT64) Number of bytes read during the life of this partition.");
	puts("    -w, --num_bytes_written   | 1                          | (INT64) Number of bytes written during the life of this partition.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h]                  | Help.");
	puts("[-v verbose_level]    | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_bb_lv_delete -a 1 -l \"my logical volume name\" -n \"node_01\" -r 1 -w 1");
	puts("____________________");
}

static struct option long_options[] =
{
	//general options
	{"help",                no_argument,       0, 'h'},
	{"verbose",             required_argument, 0, 'v'},
	//api arguments
	{"allocation_id",       required_argument, 0, 'a'},
	{"logical_volume_name", required_argument, 0, 'l'},
	{"node_name",           required_argument, 0, 'n'},
	{"num_bytes_read",      required_argument, 0, 'r'},
	{"num_bytes_written",   required_argument, 0, 'w'},
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
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 5;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
	/* getopt_long stores the option index here. */
	int indexptr = 0;

    char *arg_check = NULL; ///< Used in verifying the long arg values.
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	/*check optional args*/
	/*Only include 'hv:' to limit single char parameters*/
	while((opt = getopt_long (argc, argv, "hv:a:l:n:r:w:", long_options, &indexptr)) != -1){
		switch(opt){
			/*Single char common options. */
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
				break;
			/*Specific API parameters.*/
			/*Populate with data received from cmd line parameters.*/
			case 'a':
				/*allocation_id*/
                csm_optarg_test( "-a, --allocation_id", optarg, USAGE )
                csm_str_to_int64( input->allocation_id, optarg, arg_check,
                                "-a, --allocation_id", USAGE )
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
			case 'r':
                csm_optarg_test( "-r, --num_bytes_read", optarg, USAGE )
                csm_str_to_int64( input->num_bytes_read, optarg, arg_check, "-r, --num_bytes_read", USAGE )
				requiredParameterCounter++;
				break;
			case 'w':
                csm_optarg_test( "-w, --num_bytes_written", optarg, USAGE )
                csm_str_to_int64( input->num_bytes_written, optarg, arg_check, "-w, --num_bytes_written", USAGE )
				requiredParameterCounter++;
				break;
			default:
				csmutil_logging(error, "unknown arg: '%c'\n", opt);
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
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;            
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:    %p", input);
	csmutil_logging(debug, "  address of input:  %p", &input);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    allocation_id:       %li", input->allocation_id);
	csmutil_logging(debug, "    logical_volume_name: %s",  input->logical_volume_name);
	csmutil_logging(debug, "    node_name:           %s",  input->node_name);
	csmutil_logging(debug, "    num_bytes_read:      %"PRId64,  input->num_bytes_read);
	csmutil_logging(debug, "    num_bytes_written:   %"PRId64,  input->num_bytes_written);
	
	/*Call the api.*/
	return_value = csm_bb_lv_delete(&csm_obj, input);

    if ( return_value == CSMI_SUCCESS )
    {
        printf("---\n# Logical volume %s was successfully deleted\n...\n", 
            input->logical_volume_name);
    }
    else
    {
        printf("%s FAILED: errcode: %d errmsg: %s\n",
            argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }

	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
    csm_api_object_destroy(csm_obj);

    // Cleanup the library and print the error.
	int lib_return_value = csm_term_lib();
	if(lib_return_value != 0)
    {
		csmutil_logging(error, "  csm_term_lib rc= %d, Initialization failed. Success "
            "is required to be able to communicate between library and daemon. Are the "
            "daemons running?", lib_return_value);
		return lib_return_value;
	}

	return return_value;
}
