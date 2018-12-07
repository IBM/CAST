/*================================================================================

    csmi/src/inv/cmd/node_attributes_query.c

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
#include "csmi/include/csm_api_inventory.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Define API types to make life easier. */
#define API_PARAMETER_INPUT_TYPE csm_node_attributes_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_attributes_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_NODE_ATTRIBUTES_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_node_attributes_query ARGUMENTS [OPTIONS]");
	puts("  csm_node_attributes_query [-c comment] [-l limit] [-n node_names] [-o offset] [-s state] [-t type] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_node' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_node_attributes_query can have 4 optional arguments and requires at least 1");
	puts("    Argument         | Example value    | Description  ");                                                 
	puts("    -----------------|------------------|--------------");
	puts("    -c, --comment    | \"csm.reserved.%\" | (STRING) Filter results by the comment field in the CSM database. This field will query by 'LIKE' so '%' are accepted. API will ignore NULL values.");
	puts("    -n, --node_names | \"node01,node02\"  | (STRING) This is a csv field of node names to query. Filter results to only include records that have a matching node names. The node name is a unique identification for a node.");
	puts("                     |                  | Valid formats: \"node01\", \"node01,node02\", or \"node[01-09]\"");
	puts("    -s, --state      | \"IN_SERVICE\"     | (CSM_NODE_STATE) Filter results by the state field in the CSM database. API will ignore NULL values.");
	puts("                     |                  | Valid values: \"undefined\", \"DISCOVERED\", \"IN_SERVICE\", \"OUT_OF_SERVICE\", \"ADMIN_RESERVED\", \"SOFT_FAILURE\", or \"HARD_FAILURE\".");
	puts("    -t, --type       | \"compute\"        | (STRING) Filter results by the type field in the CSM database. API will ignore NULL values.");
	puts("");
	puts("  FILTERS:");
	puts("    csm_node_attributes_query can have 2 optional filters.");
	puts("    Argument     | Example value | Description  ");                                                 
	puts("    -------------|---------------|--------------");
	puts("    -l, --limit  | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_node_attributes_query -n \"node01\"");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",       no_argument,       0, 'h'},
	{"verbose",    required_argument, 0, 'v'},
	//api arguments
	{"comment",    required_argument, 0, 'c'},
	{"node_names", required_argument, 0, 'n'},
	{"state",      required_argument, 0, 's'},
	{"type",       required_argument, 0, 't'},
	//filters
	{"limit",      required_argument, 0, 'l'},
	{"offset",     required_argument, 0, 'o'},
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
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 1;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	uint32_t i = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:c:l:n:o:s:t:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'c':
				csm_optarg_test( "-c, --comment", optarg, USAGE );
				input->comment = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'l':
				csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
			case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
				break;
			case 'n':
            {
                csm_optarg_test( "-n, --node_names", optarg, USAGE );
				
				int return_node_range = 0;	
				
				return_node_range = CORAL_stringTools_nodeRangeParser(optarg, &(input->node_names_count), &(input->node_names));
				
				if(return_node_range == 3)
				{
					//function returned 3: 
					//noderange not properly formatted. 
					//node range digits precision don't match. ie: 003 VS 05 ie: node[005-98]
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  node range digits precision don't match.");
					csmutil_logging(error, "  ie: 3 precision places VS 2 precision places, ie: node[005-98].");
					csmutil_logging(error, "  node range digits precision MUST match.");
					csmutil_logging(error, "  %s is not a valid value for node range.", optarg);
					csmutil_logging(error, "  determined noderange was not properly formatted.");
					USAGE();
					return CSMERR_INVALID_PARAM;
				}
				else if(return_node_range != 0)
				{
					//No range was found
					csm_parse_csv( optarg, input->node_names, input->node_names_count, char*, csm_str_to_char, NULL, "-n, --node_names", USAGE );
				}
				
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
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
	csmutil_logging(debug, "    comment:          %s", input->comment);
	csmutil_logging(debug, "    limit:            %i", input->limit);
	csmutil_logging(debug, "    node_names_count: %i", input->node_names_count);
	csmutil_logging(debug, "    node_names:       %p", input->node_names);
	for(i = 0; i < input->node_names_count; i++){
		csmutil_logging(debug, "      node_names[%i]: %s", i, input->node_names[i]);
	}
	csmutil_logging(debug, "    offset:           %i", input->offset);
	csmutil_logging(debug, "    type:             %s", csm_get_string_from_enum(csmi_node_type_t, input->type) );
	
    /* Call the C API. */
	return_value = csm_node_attributes_query(&csm_obj, input, &output);
	/* Use CSM API free to release arguments. We no longer need them. */
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    switch ( return_value )
    {
        case CSMI_SUCCESS:
			puts("---");
			printf("Total_Records: %i\n", output->results_count);
	    	for(i = 0; i < output->results_count; i++){
	    		printf("Record_%i:\n", i+1);
	    		printf("  node_name:               %s\n", output->results[i]->node_name);
	    		printf("  collection_time:         %s\n", output->results[i]->collection_time);
				printf("  update_time:             %s\n", output->results[i]->update_time);
	    		printf("  comment:                 \"%s\"\n", output->results[i]->comment);
				printf("  discovered_cores:        %i\n", output->results[i]->discovered_cores);
				printf("  discovered_dimms:        %i\n", output->results[i]->discovered_dimms);
	    		printf("  discovered_gpus:         %i\n", output->results[i]->discovered_gpus);
				printf("  discovered_hcas:         %i\n", output->results[i]->discovered_hcas);
	    		printf("  discovered_sockets:      %i\n", output->results[i]->discovered_sockets);
				printf("  discovered_ssds:         %i\n", output->results[i]->discovered_ssds);
				printf("  feature_1:               %s\n", output->results[i]->feature_1);
	    		printf("  feature_2:               %s\n", output->results[i]->feature_2);
	    		printf("  feature_3:               %s\n", output->results[i]->feature_3);
	    		printf("  feature_4:               %s\n", output->results[i]->feature_4);
	    		printf("  hard_power_cap:          %i\n", output->results[i]->hard_power_cap);
	    		printf("  installed_memory:        %" PRId64 "\n", output->results[i]->installed_memory);
	    		printf("  installed_swap:          %" PRId64 "\n", output->results[i]->installed_swap);
	    		printf("  kernel_release:          %s\n", output->results[i]->kernel_release);
	    		printf("  kernel_version:          %s\n", output->results[i]->kernel_version);
	    		printf("  machine_model:           %s\n", output->results[i]->machine_model);
	    		printf("  os_image_name:           %s\n", output->results[i]->os_image_name);
				printf("  os_image_uuid:           %s\n", output->results[i]->os_image_uuid);
	    		printf("  physical_frame_location: %s\n", output->results[i]->physical_frame_location);
	    		printf("  physical_u_location:     %s\n", output->results[i]->physical_u_location);
	    		printf("  primary_agg:             %s\n", output->results[i]->primary_agg);
	    		printf("  secondary_agg:           %s\n", output->results[i]->secondary_agg);
	    		printf("  serial_number:           %s\n", output->results[i]->serial_number);
	    		printf("  state:                   %s\n", csm_get_string_from_enum(csmi_node_state_t, output->results[i]->state));
	    		printf("  type:                    %s\n", csm_get_string_from_enum(csmi_node_type_t, output->results[i]->type));
	    	}
			puts("...");
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
