/*================================================================================

    csmi/src/bb/cmd/bb_vg_query.c

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
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Should we do this? */
#define API_PARAMETER_INPUT_TYPE csm_bb_vg_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_bb_vg_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help() {
	puts("_____CSM_BB_VG_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_bb_vg_query ARGUMENTS [OPTIONS]");
	puts("  csm_bb_vg_query -V vg_names [-l limit] [-o offset] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used by a system administrator to view records in the \"csm_vg\" table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_bb_vg_query has 2 optional arguments and requires at least 1");
	puts("    Argument         | Example value     | Description  ");                                                 
	puts("    -----------------|-------------------|--------------");
	puts("    -n, --node_names | \"node_01,node_02\" | (STRING) This is a csv field of node names. Filters results to only include matching node names.");
	puts("    -V, --vg_names   | \"vg_01,vg_02\"     | (STRING) This is a csv field of vg names. Filters results to only include matching vg names.");
	puts("");
	puts("  FILTERS:");
	puts("    csm_bb_vg_query can have 2 optional filters.");
	puts("    Argument      | Example value | Description  ");                                                 
	puts("    --------------|---------------|--------------");
	puts("    -l, --limit   | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset  | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("GENERAL OPTIONS:");
	puts("[-h]                  | Help.");
	puts("[-v verbose_level]    | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_bb_vg_query -V \"vg_01\"");
	puts("____________________");
}

static struct option long_options[] =
{
	//general options
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	//api arguments
	{"node_names",    required_argument, 0, 'n'},
	{"vg_names",      required_argument, 0, 'V'},
	//filters
	{"limit",         required_argument, 0, 'l'},
	{"offset",        required_argument, 0, 'o'},
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
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 0;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 1;
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
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while((opt = getopt_long (argc, argv, "hv:n:l:o:V:", long_options, &indexptr)) != -1){
		switch(opt){
			/*Single char common options. */
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'n':
				csm_optarg_test( "-n, --node_names", optarg, USAGE )
                csm_parse_csv( optarg, input->node_names, input->node_names_count, char*,
                            csm_str_to_char, NULL, "-n, --node_names", USAGE )
				/* Increment requiredParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			case 'l':
                csm_optarg_test( "-l, --limit", optarg, USAGE )
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
				break;
			case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE )
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
				break;
			case 'V':
			{
                csm_optarg_test( "-V, --vg_names", optarg, USAGE )
                csm_parse_csv( optarg, input->vg_names, input->vg_names_count, char*,
                            csm_str_to_char, NULL, "-V, --vg_names", USAGE )
				/* Increment requiredParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
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
	csmutil_logging(debug, "    node_names_count: %i", input->node_names_count);
	csmutil_logging(debug, "    node_names:       %s", input->node_names);
	csmutil_logging(debug, "    limit:            %i", input->limit);
	csmutil_logging(debug, "    offset:           %i", input->offset);
	csmutil_logging(debug, "    vg_names_count:   %i", input->vg_names_count);
	csmutil_logging(debug, "    vg_names:         %s", input->vg_names);
	csmutil_logging(debug, "  value of output:   %p", output);
	csmutil_logging(debug, "  address of output: %p", &output);
	
	/*All that just to call the api.*/
	return_value = csm_bb_vg_query(&csm_obj, input, &output);
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    switch ( return_value )
    {
        case CSMI_SUCCESS:
	    	puts("---");
	    	printf("Total_Records: %i\n", output->results_count);
	    	for (i = 0; i < output->results_count; i++) {
	    		printf("RECORD_%i:\n", i+1);
	    		printf("  vg_name:        %s\n", output->results[i]->vg_name);
				printf("  node_name:      %s\n", output->results[i]->node_name);
				printf("  total_size:     %" PRId64 "\n", output->results[i]->total_size);
	    		printf("  available_size: %" PRId64 "\n", output->results[i]->available_size);
				printf("  scheduler:      %c\n", csm_print_bool_custom(output->results[i]->scheduler, 't','f'));
	    	}
	    	puts("...");
            break;
        case CSMI_NO_RESULTS:
	    	puts("---");
	    	puts("Total_Records: 0");
	    	puts("# No matching record found.");
	    	puts("...");
            break;
        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n",
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }
	
    /* Call internal CSM API clean up. */
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
