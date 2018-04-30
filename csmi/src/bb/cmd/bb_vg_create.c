/*================================================================================

    csmi/src/bb/cmd/bb_vg_create.c

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
/*CORAL includes*/
#include "utilities/include/string_tools.h"
/*CSM Include*/
#include "csmi/include/csm_api_burst_buffer.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"

/* Should we do this? */
#define API_PARAMETER_INPUT_TYPE csm_bb_vg_create_input_t
//#define API_PARAMETER_OUTPUT_TYPE csm_BbVgCreate_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help() {
	puts("_____CSM_BB_VG_CREATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_bb_vg_create ARGUMENTS [OPTIONS]");
	puts("  csm_bb_vg_create -a available_size -n node_name -s ssd_serial_number -S ssd_allocation -t total_size -V vg_name [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY:  Intended to be used by a system administrator to manually create a VG. ");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_bb_vg_create expects 7 mandatory argument");
	puts("    Argument                | Example value | Description  ");                                                 
	puts("    ------------------------|---------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -a, --available_size    | 256             | (INT64) Available size remaining in this volume group. Can not be greater than 'total_size'. Values less than zero are not valid.");
	puts("    -c, --scheduler         | t               | (CHAR) Tells CSM whether or not this is the volume group for the scheduler. TRUE or FALSE.");
	puts("                            |                 | Valid values: 't' or 'f'");
	puts("    -n, --node_name         | \"node_01\"       | (STRING) This volume group is attached to this node. Can not be NULL. 'node_name' must exist in 'csm_node' table.");
	puts("    -s, --ssd_serial_number | \"abc123,xyz456\" | (STRING) This is a csv field of ssd serial numbers in this volume group. ");
	puts("    -S, --ssd_allocation    | 500,500           | (INT64) This is a csv field of ssd allocations values for each ssd. Order must match serial number order above. Can not be NULL. Must contain at least one entry.");
	puts("    -t, --total_size        | 1000            | (INT64) Total size of this volume group. Values less than zero are not valid. All ssd_allocation values must add up to this field.");
	puts("    -V, --vg_name           | \"vg_01\"         | (STRING) Unique identifier for this volume group. Can not be NULL.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h]                  | Help.");
	puts("[-v verbose_level]    | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_bb_vg_create -a 256 -n \"node_01\" -s \"abc123\" -S 500 -t 500 -V \"vg_01\" -c t");
	puts("____________________");
}

static struct option long_options[] =
{
	//general options
	{"help",              no_argument,       0, 'h'},
	{"verbose",           required_argument, 0, 'v'},
	//api arguments
	{"available_size",    required_argument, 0, 'a'},
	{"scheduler",         required_argument, 0, 'c'},
	{"node_name",         required_argument, 0, 'n'},
	{"ssd_serial_number", required_argument, 0, 's'},
	{"ssd_allocation",    required_argument, 0, 'S'},
	{"total_size",        required_argument, 0, 't'},
	{"vg_name",           required_argument, 0, 'V'},
	{0, 0, 0, 0}
};

/*
* Summary: Simple command line interface for the CSM API 'bb vg create'. 
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
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 7;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	int i = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	input->ssd_info_count = 0;
	input->ssd_info = NULL;
	
	/*check optional args*/
	while((opt = getopt_long (argc, argv, "hv:a:c:n:s:S:t:V:", long_options, &indexptr)) != -1){
		switch(opt){
			/*Single char common options. */
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
				/*available_size*/
                csm_optarg_test( "-a, --available_size", optarg, USAGE )
                csm_str_to_int64( input->available_size, optarg, arg_check, "-a, --available_size", USAGE )
				requiredParameterCounter++;
				break;
			case 'c':
				if(strlen(optarg) == 1 && 
                        ( optarg[0] == 't' || optarg[0] == 'f' ) )
                {
				    input->scheduler = optarg[0] == 't' ? CSM_TRUE : CSM_FALSE;
				    requiredParameterCounter++;
				}
                else
                {
					csmutil_logging(error, "Invalid parameter for -c: optarg , encountered: %s", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				break;
			case 'n':
				/*node_name*/
                csm_optarg_test( "-n, --node_name", optarg, USAGE )
                input->node_name = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 's':
			{
				/*ssd_serial_number*/
                csm_optarg_test( "-s, --ssd_serial_number", optarg, USAGE )
				
				//helper variables
				char** ssd_names = NULL;
				uint32_t ssd_names_count = 0;
				
				//parse the opt arg into temp names.
				csm_parse_csv( optarg, ssd_names, ssd_names_count, char *, csm_str_to_char, NULL, "-s, --ssd_serial_number", USAGE )
				
				//check if already set
				if(input->ssd_info_count == 0){
					//not yet set
					input->ssd_info_count = ssd_names_count;
					//allocate the space
					input->ssd_info = (csmi_bb_vg_ssd_info_t**)calloc(input->ssd_info_count, sizeof(csmi_bb_vg_ssd_info_t*));
					for(i = 0; i < input->ssd_info_count; i++){
						//set up
						input->ssd_info[i] = NULL;
						csm_init_struct_ptr(csmi_bb_vg_ssd_info_t , input->ssd_info[i]);
					}
				}else{
					//already set by -S ssd_allocation
					//check to make sure the numbers match
					if(input->ssd_info_count != ssd_names_count)
					{
						//then we have a problem
						csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
						csmutil_logging(error, "  ssd_info_count does not match");
						uint32_t j = 0;
						for(j = 0; j < ssd_names_count; j++)
						{
							free(ssd_names[j]);
						}
						free(ssd_names);
						break;
					}
				}
				//everything is ready and set up.
				uint32_t j = 0;
				//copy temp names into the structs
				for(j = 0; j < input->ssd_info_count; j++)
				{
					input->ssd_info[j]->ssd_serial_number = strdup(ssd_names[j]);
					//free temp
					free(ssd_names[j]);
				}
				//free the list
				free(ssd_names);
				
				requiredParameterCounter++;
				break;
			}
			case 'S':
			{
				/*ssd_serial_number*/
                csm_optarg_test( "-S, --ssd_allocation", optarg, USAGE )
				
				//helper variables
				int64_t* ssd_allocations = NULL;
				uint32_t ssd_allocations_count = 0;
				
				//parse the opt arg into temp names.
				csm_parse_csv( optarg, ssd_allocations, ssd_allocations_count, int64_t, csm_str_to_int64, arg_check, "-S, --ssd_allocation", USAGE );
				
				//check if already set
				if(input->ssd_info_count == 0){
					//not yet set
					input->ssd_info_count = ssd_allocations_count;
					//allocate the space
					input->ssd_info = (csmi_bb_vg_ssd_info_t**)calloc(input->ssd_info_count, sizeof(csmi_bb_vg_ssd_info_t*));
					for(i = 0; i < input->ssd_info_count; i++){
						//set up
						input->ssd_info[i] = NULL;
						csm_init_struct_ptr(csmi_bb_vg_ssd_info_t , input->ssd_info[i]);
					}
				}else{
					//already set by -s ssd_serial_number
					//check to make sure the numbers match
					if(input->ssd_info_count != ssd_allocations_count)
					{
						//then we have a problem
						csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
						csmutil_logging(error, "  ssd_info_count does not match");
						free(ssd_allocations);
						break;
					}
				}
				//everything is ready and set up.
				uint32_t j = 0;
				//copy temp names into the structs
				for(j = 0; j < input->ssd_info_count; j++)
				{
					input->ssd_info[j]->ssd_allocation = ssd_allocations[j];
				}
				//free the list
				free(ssd_allocations);
				
				requiredParameterCounter++;
				break;
			}
			case 't':
				/*total_size*/
                csm_optarg_test( "-t, --total_size", optarg, USAGE )
                //csm_str_to_int64( input->total_size, optarg, arg_check, "-t, --total_size", USAGE )
				input->total_size = strtoll(optarg, &arg_check, 10);
				requiredParameterCounter++;
				break;
			case 'V':
			{
                csm_optarg_test( "-V, --vg_name", optarg, USAGE )
                input->vg_name = strdup(optarg);
				requiredParameterCounter++;
				break;
			}
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
	csmutil_logging(debug, "    available_size: %"PRId64, input->available_size);
	csmutil_logging(debug, "    node_name:      %s", input->node_name);
	csmutil_logging(debug, "    ssd_info_count: %"PRId32, input->ssd_info_count);
	csmutil_logging(debug, "    ssd_info:       %p", input->ssd_info);
	for(i = 0; i < input->ssd_info_count; i++){
	csmutil_logging(debug, "      ssd_info[%i]->ssd_serial_number: %s", i, input->ssd_info[i]->ssd_serial_number);
	csmutil_logging(debug, "      ssd_info[%i]->ssd_allocation:    %"PRId64, i, input->ssd_info[i]->ssd_allocation);
	}
	csmutil_logging(debug, "    total_size:     %"PRId64, input->total_size);
	csmutil_logging(debug, "    vg_name:        %s", input->vg_name);
	
	/*All that just to call the api.*/
	returnValue = csm_bb_vg_create(&csm_obj, input);
	
	switch( returnValue )
    {
		case CSMI_SUCCESS:
            printf("---\n# The vg: \"%s\" has been successfully created on node: \"%s\".\n...\n", input->vg_name, input->node_name);
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
