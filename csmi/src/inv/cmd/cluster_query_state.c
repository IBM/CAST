/*================================================================================

    csmi/src/inv/cmd/cluster_query_state.c

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
#include "csmi/include/csm_api_inventory.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Define API types to make life easier. */
#define API_PARAMETER_INPUT_TYPE csm_cluster_query_state_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_cluster_query_state_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_CLUSTER_QUERY_STATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_cluster_query_state ARGUMENTS [OPTIONS]");
	puts("  csm_cluster_query_state [-l limit] [-n num_allocs] [-o offset] [-O order_by] [-s state] [-t type] [-h] [-v verbose_level] [-Y]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_node' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_cluster_query_state can have 3 optional argument");
	puts("    Argument         | Example value | Description  ");                                                 
	puts("    -----------------|---------------|--------------");
	puts("    -n, --num_allocs | 0             | (INT) Filter results by the number of allocations a node is apart of. API will ignore values less than 0.");
	puts("    -s, --state      | \"IN_SERVICE\"  | (CSM_NODE_STATE) Filter results by the state of a node. API will ignore NULL values.");
	puts("                     |               | Valid values: \"undefined\", \"DISCOVERED\", \"IN_SERVICE\", \"OUT_OF_SERVICE\", \"ADMIN_RESERVED\", or \"SOFT_FAILURE\" ");
	puts("    -t, --type       | \"compute\"     | (STRING) Filter results by the type field in the CSM database. API will ignore NULL values.");
	puts("");
	puts("  FILTERS:");
	puts("    csm_cluster_query_state can have 2 optional filters.");
	puts("    Argument       | Example value | Description  ");                                                 
	puts("    ---------------|---------------|--------------");
	puts("    -l, --limit    | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset   | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("    -O, --order_by | a             | (CHAR) SQL 'ORDER BY' numeric value. Default Value: 'a'");
	puts("                                   | Valid Values: [a] = 'ORDER BY node_name ASC NULLS LAST'"); 
	puts("                                   |               [b] = 'ORDER BY node_name DESC NULLS LAST'");
	puts("                                   |               [c] = 'ORDER BY state ASC NULLS LAST'"); 
	puts("                                   |               [d] = 'ORDER BY state DESC NULLS LAST'");
	puts("                                   |               [e] = 'ORDER BY type ASC NULLS LAST'");
	puts("                                   |               [f] = 'ORDER BY type DESC NULLS LAST'");
	puts("                                   |               [g] = 'ORDER BY num_allocs ASC NULLS LAST'");
	puts("                                   |               [h] = 'ORDER BY num_allocs DESC NULLS LAST'");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("[-Y, --YAML]                  | Set output to YAML. By default for this API, we have a custom output for ease of reading the long transaction history.");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_cluster_query_state ");
	puts("");
	puts("OUTPUT OF THIS COMMAND CAN BE DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",       no_argument,       0, 'h'},
	{"verbose",    required_argument, 0, 'v'},
	{"YAML",       no_argument,       0, 'Y'},
	//api arguments
	{"num_allocs", required_argument, 0, 'n'},
	{"state",      required_argument, 0, 's'},
	{"type",       required_argument, 0, 't'},
	//filters
	{"limit",      required_argument, 0, 'l'},
	{"offset",     required_argument, 0, 'o'},
	{"order_by",   required_argument, 0, 'O'},
	{0,0,0,0}
};

/*
* Summary: Simple command line interface for the CSM API 'node attributes query'. 
* 			Takes in the name of a single node via command line parameters, queries the 
*			database for this node, and prints all the attributes of that node.
*/
int main(int argc, char *argv[])
{	
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 0;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	int i = 0;
	/*For format printing later. */
	char YAML = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:l:n:o:O:s:t:Y", longopts, &indexptr)) != -1) {
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
			case 'n':
            {
				int32_t check_int = -1;
				csm_str_to_int32( check_int, optarg, arg_check, "-n, --num_allocs", USAGE );
                if(check_int >= 0)
				{
					input->num_allocs = check_int;
				}
				else
				{
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  %s is not a valid value for num_allocs.", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
				break;
			case 'O':
				if(strlen(optarg) == 1 && 
                    (  optarg[0] == 'a' 
					|| optarg[0] == 'b' 
					|| optarg[0] == 'c' 
					|| optarg[0] == 'd' 
					|| optarg[0] == 'e' 
					|| optarg[0] == 'f' 
					|| optarg[0] == 'g' 
					|| optarg[0] == 'h' 
					)
				)
                {
					input->order_by = optarg[0];
				}else{
					csmutil_logging(error, "Invalid parameter for -O: optarg , encountered: %s", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				break;
			case 's':
			{
                csm_optarg_test( "-s, --state", optarg, USAGE )
				int temp_state = csm_enum_from_string(optarg, csmi_node_state_t_strs);
                input->state = temp_state != -1 ? (csmi_node_state_t) temp_state : csm_enum_max(csmi_node_state_t);
				if(input->state == csm_enum_max(csmi_node_state_t))
				{
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  %s is not a valid value for state.", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				optionalParameterCounter++;	
				break;
			}
			case 't':
            {
				csm_optarg_test( "-t, --type", optarg, USAGE );
                int temp_type = csm_get_enum_from_string(csmi_node_type_t, optarg);
                input->type = temp_type != -1 ? (csmi_node_type_t) temp_type : csm_enum_max(csmi_node_type_t);
				if(input->type == csm_enum_max(csmi_node_type_t))
				{
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  %s is not a valid value for type.", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				optionalParameterCounter++;
				break;
            }
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
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if(return_value != 0){
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
	csmutil_logging(debug, "    limit:            %"PRId32"\n", input->limit);
	csmutil_logging(debug, "    num_allocs:       %"PRId32"\n", input->num_allocs);
	csmutil_logging(debug, "    offset:           %"PRId32"\n", input->offset);
	csmutil_logging(debug, "    order_by:         %c", input->order_by);
	csmutil_logging(debug, "    state:            %s", csm_get_string_from_enum(csmi_node_state_t, input->state) );
	csmutil_logging(debug, "    type:             %s", csm_get_string_from_enum(csmi_node_type_t, input->type) );
	
    /* Call the C API. */
	return_value = csm_cluster_query_state(&csm_obj, input, &output);
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
					printf("  node_name:       %s\n", output->results[i]->node_name);
					printf("  collection_time: %s\n", output->results[i]->collection_time);
					printf("  update_time:     %s\n", output->results[i]->update_time);
					printf("  state:           %s\n", csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state));
					printf("  type:            %s\n", csm_get_string_from_enum(csmi_node_type_t, output->results[i]->type));
					printf("  num_allocs:      %" PRIu32 "\n", output->results[i]->num_allocs);
					if(output->results[i]->num_allocs > 0)
					{
						printf("  allocs:\n");
						uint32_t j = 0;
						for(j = 0; j < output->results[i]->num_allocs; j++)
						{
							printf("    - alloc_id: %" PRId64 "\n", output->results[i]->allocs[j]);
						}
						printf("  states:\n");
						j = 0;
						for(j = 0; j < output->results[i]->num_allocs; j++)
						{
							printf("    - state: %s\n", output->results[i]->states[j]);
						}
						printf("  shared:\n");
						j = 0;
						for(j = 0; j < output->results[i]->num_allocs; j++)
						{
							printf("    - shared: %s\n", output->results[i]->shared[j]);
						}
					}
				}
				puts("...");
			}
			else
			{
				// char* temp_allocs = NULL;
				// char* temp_states = NULL;
				// char* temp_shared = NULL;
				
				// temp_allocs = strdup("temp");
				// temp_states = strdup("temp");
				// temp_shared = strdup("temp");
				int largest_nodename_length = 0;
				int largest_state_length = 0;
				int largest_type_length = 0;
				
				// 10 = largest 
				for(i = 0; i < output->results_count; i++)
				{
					int temp_nodeNameSize = strlen(output->results[i]->node_name);
					int temp_stateSize = strlen(csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state));
					int temp_typeSize = strlen(csm_get_string_from_enum(csmi_node_type_t, output->results[i]->type));
					
					if(temp_nodeNameSize > largest_nodename_length){
						largest_nodename_length = temp_nodeNameSize;
					}
					if(temp_stateSize > largest_state_length){
						largest_state_length = temp_stateSize;
					}
					if(temp_typeSize > largest_type_length){
						largest_type_length = temp_typeSize;
					}
				}
				
				
				puts("---");
				//printf("node_name: %s\n", output->results[i]->node_name);
				printf("# %-*s | %-26s | %-26s | %-*s | %-*s | %s\n", largest_nodename_length , "node_name", "collection_time", "update_time", largest_state_length, "state", largest_type_length, "type", "num_allocs - allocs - states - shared");
				printf("# %-*s + %-26s + %-26s + %-*s + %-*s + %s\n", largest_nodename_length , " ", " ", " ", largest_state_length, " ", largest_type_length, " ", " ");
				
				//printf("#    node_name    |      collection_time       |        update_time         |   state    |    type    | num_allocs | allocs |        states        | shared \n");
				//printf("#-----------------+----------------------------+----------------------------+------------+------------+------------+--------+----------------------+--------\n");
				for(i = 0; i < output->results_count; i++){
					//format the string for nice padding
					printf("# %-*s | %-26s | %-26s | %-*s | %-*s | ", largest_nodename_length , output->results[i]->node_name, output->results[i]->collection_time, output->results[i]->update_time, largest_state_length, csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state), largest_type_length, csm_get_string_from_enum(csmi_node_type_t, output->results[i]->type));
					printf(" %" PRIu32, output->results[i]->num_allocs);
					if(output->results[i]->num_allocs > 0)
					{
						printf(" - ");
						uint32_t j = 0;
						for(j = 0; j < output->results[i]->num_allocs; j++)
						{
							if(j > 0)
							{
								printf(",");
							}
							printf("%" PRId64, output->results[i]->allocs[j]);
						}
						printf(" - ");
						j = 0;
						for(j = 0; j < output->results[i]->num_allocs; j++)
						{
							if(j > 0)
							{
								printf(",");
							}
							printf("%s", output->results[i]->states[j]);
						}
						printf(" - ");
						j = 0;
						for(j = 0; j < output->results[i]->num_allocs; j++)
						{
							if(j > 0)
							{
								printf(",");
							}
							printf("%s", output->results[i]->shared[j]);
						}
						
					}
					printf("\n");
					
					
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
