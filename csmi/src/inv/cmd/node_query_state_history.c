/*================================================================================

    csmi/src/inv/cmd/node_query_state_history.c

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
#include "csmi/include/csm_api_inventory.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Define API types to make life easier. */
#define API_PARAMETER_INPUT_TYPE csm_node_query_state_history_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_query_state_history_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_NODE_QUERY_STATE_HISTORY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_node_query_state_history ARGUMENTS [OPTIONS]");
	puts("  csm_node_query_state_history -n node_name [-l limit] [-o offset] [-O order_by] [-h] [-v verbose_level] [-Y]");
	puts("");
	puts("SUMMARY: Used to retrieve the state history of a node over its lifetime in the CSM system.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_node_query_state_history expects 1 mandatory parameter");
	puts("    Argument        | Example value | Description  ");                                                 
	puts("    ----------------|---------------|--------------");
	puts("    -n, --node_name | \"node01\"      | (STRING) Identifies which node this information is for.");
	puts("");
	puts("  FILTERS:");
	puts("    csm_node_query_state_history can have 3 optional filters.");
	puts("    Argument       | Example value | Description  ");                                                 
	puts("    ---------------|---------------|--------------");
	puts("    -l, --limit    | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset   | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("    -O, --order_by | d             | (CHAR) SQL 'ORDER BY' numeric value. Default Value: 'a'");
	puts("                                   | Valid Values: [a] = 'ORDER BY history_time ASC NULLS LAST'");
	puts("                                   |               [d] = 'ORDER BY history_time DESC NULLS LAST'");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("[-Y, --YAML]                  | Set output to YAML. By default for this API, we have a custom output for ease of reading the long transaction history.");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_node_query_state_history -n \"node01\"");
	puts("");
	puts("OUTPUT OF THIS COMMAND CAN BE DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",      no_argument,       0, 'h'},
	{"verbose",   required_argument, 0, 'v'},
	{"YAML",      no_argument,       0, 'Y'},
	//api arguments
	{"node_name", required_argument, 0, 'n'},
	//filters
	{"limit",     required_argument, 0, 'l'},
	{"offset",    required_argument, 0, 'o'},
	{"order_by",  required_argument, 0, 'O'},
	{0,0,0,0}
};

/*
* Summary: Simple command line interface for the CSM API 'node attributes query history'. 
* 			Takes in the name of a single node via command line parameters, queries the 
*			database for this node, and prints all the entries of that node in the csm_node_history table of the CSM DB.
*/
int main(int argc, char *argv[])
{	
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 1;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	uint32_t i = 0;
	/*For format printing later. */
	char YAML = 0;

	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:l:n:o:O:Y", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'l':
				csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
			case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
				break;
			case 'O':
				if(strlen(optarg) == 1 && 
                    ( optarg[0] == 'a' || optarg[0] == 'd' ) )
                {
					input->order_by = optarg[0];
				}else{
					csmutil_logging(error, "Invalid parameter for -O: optarg , encountered: %s", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				break;
			case 'n':
                csm_optarg_test( "-n, --node_name", optarg, USAGE );
				input->node_name = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 'Y':
				YAML = 1;
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
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:    %p", input);
	csmutil_logging(debug, "  address of input:  %p", &input);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    limit:     %i", input->limit);
	csmutil_logging(debug, "    node_name: %s", input->node_name);
	csmutil_logging(debug, "    offset:    %i", input->offset);
	csmutil_logging(debug, "    order_by:  %c", input->order_by);
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if(return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}
	
	/*All that just to call the api.*/
	return_value = csm_node_query_state_history(&csm_obj, input, &output);
	/* Use CSM API free to release arguments. We no longer need them. */
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
    switch ( return_value )
    {
        case CSMI_SUCCESS:
			if(YAML == 1)
			{
				puts("---");
				printf("Total_Records: %i\n", output->results_count);
				for(i = 0; i < output->results_count; i++){
					printf("Record_%i:\n", i+1);
					printf("  history_time: %s\n", output->results[i]->history_time);
					printf("  node_name:    %s\n", output->node_name);
					printf("  state:        %s\n", csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state));
					printf("  alteration:   %s\n", csm_get_string_from_enum(csmi_node_alteration_t, output->results[i]->alteration));
					printf("  RAS_rec_id:   %s\n", output->results[i]->ras_rec_id);
					printf("  RAS_msg_id:   %s\n", output->results[i]->ras_msg_id);
				}
				puts("...");
			}
			else
			{
				puts("---");
				printf("node_name: %s\n", output->node_name);
				printf("#         history_time        |      state     |      alteration      | RAS_rec_id, RAS_msg_id \n");
				printf("# ----------------------------+----------------+----------------------+------------------------\n");
				for(i = 0; i < output->results_count; i++){
					if(output->results[i]->state == CSM_NODE_SOFT_FAILURE)
					{
						printf("#  %s | %-15s| %-21s| %s, %s\n", output->results[i]->history_time, csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state), csm_get_string_from_enum(csmi_node_alteration_t, output->results[i]->alteration), output->results[i]->ras_rec_id, output->results[i]->ras_msg_id);
					}else{
						printf("#  %s | %-15s| %-21s| \n", output->results[i]->history_time, csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state), csm_get_string_from_enum(csmi_node_alteration_t, output->results[i]->alteration));
					}
				}
				puts("...");
				
			}
			break;
        case CSMI_NO_RESULTS:
            puts("---");
            printf("Total_Records: 0\n");
            puts("# No matching records found.");
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
    if( lib_return_value != 0 )
    {
        csmutil_logging(error, "csm_term_lib rc= %d, Initialization failed. Success "
            "is required to be able to communicate between library and daemon. Are the "
            "daemons running?", lib_return_value);
        return lib_return_value;
    }
		
	return return_value;
}
