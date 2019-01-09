/*================================================================================

    csmi/src/ras/cmd/ras_msg_type_query.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
 * Author: Nick Buonarota
 * Email:  nbuonar@us.ibm.com
 */
 
/* C includes*/
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
/* CSM includes */
#include "csmi/include/csm_api_ras.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Defines to make API easier */
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE  csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_RAS_MSG_TYPE_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_ras_msg_type_query ARGUMENTS [OPTIONS]");
	puts("  csm_ras_msg_type_query [-c control_action] [-m msg_id] [-M message] [-s severity] [-S set_state] [-l limit] [-o offset] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_ras_type' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_ras_msg_type_query can have 4 optional arguments and requires at least 1");
	puts("    Argument             | Example value               | Description  ");                                                 
	puts("    ---------------------|-----------------------------|--------------");
	puts("    -c, --control_action | \"node_not_ready\"            | (STRING) 'control_action' search string. The 'control_action' is the name of control action script to invoke for this event.");
	puts("    -m, --msg_id         | \"csm.%%\"                    | (STRING) The 'msg_id' search string. The 'msg_id' is the identifier string for this RAS event.");
	puts("    -M, --message        | \"Message for RAS system\"    | (STRING) 'message' search string. 'message' is the RAS message to display to the user (pre-variable substitution).");	
	puts("    -s, --severity       | \"INFO\"                      | (STRING) 'severity' search string. 'severity' is the severity of the RAS event.");
	puts("                         |                             | Valid Values: \"INFO\", \"WARNING\", or \"FATAL\"");
	puts("    -S, --set_state      | \"SOFT_FAILURE,HARD_FAILURE\" | (STRING) This is a csv field of 'set_state' search string. 'set_state' is the state the RAS event will set a node to after the event triggers.");
	puts("  FILTERS:");
	puts("    csm_ras_msg_type_query can have 2 optional filters.");
	puts("    Argument      | Example value | Description  ");                                                 
	puts("    --------------|---------------|--------------");
	puts("    -l, --limit   | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset  | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_ras_msg_type_query -s \"INFO\"");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",             no_argument,       0, 'h'},
	{"verbose",          required_argument, 0, 'v'},
	//arguments
	{"control_action",   required_argument, 0, 'c'},
	{"msg_id",           required_argument, 0, 'm'},
	{"message",          required_argument, 0, 'M'},
	{"severity",         required_argument, 0, 's'},
	{"set_state",        required_argument, 0, 'S'},
	//filters
	{"limit",            required_argument, 0, 'l'},
	{"offset",           required_argument, 0, 'o'},
	{0,0,0,0}
};

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
    int option_index = 0;
	/* For for loops.*/
	uint32_t i = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;

	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:c:l:m:M:o:s:S:", longopts, &option_index)) != -1) {
		switch (opt) {
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;	
			/*Specific API parameters.*/
			/*Populate with data received from cmd line parameters.*/
			case 'c':
                csm_optarg_test( "-c, --control_action", optarg, USAGE )
				input->control_action = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'm':
                csm_optarg_test( "-m, --msg_id", optarg, USAGE )
				input->msg_id = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'M':
                csm_optarg_test( "-M, --message", optarg, USAGE )
				input->message = strdup(optarg);
				optionalParameterCounter++;
				break;
            case 'l':
                csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
            case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
			case 's':
				/*severity*/
                csm_optarg_test( "-s, --severity", optarg, USAGE )
				if( strcmp(optarg,"INFO")    == 0 || 
                    strcmp(optarg,"WARNING") == 0 || 
                    strcmp(optarg,"FATAL")   == 0)
                {
					int temp_severity = csm_get_enum_from_string(csmi_ras_severity_t, optarg);
					input->severity = temp_severity != -1 ? (csmi_ras_severity_t) temp_severity : csm_enum_max(csmi_ras_severity_t);
				}
                else
                {
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  %s is not a valid value for severity.", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				optionalParameterCounter++;
				break;
			case 'S':
			{
				csm_optarg_test( "-s, --set_state", optarg, USAGE );
				csm_parse_csv( optarg, input->set_states, input->set_states_count,
                            char*, csm_str_to_char, NULL, "-s, --set_state", USAGE );

				//see if those are all valid states.
				for(i = 0; i < input->set_states_count; i++){
					if( -1 == csm_get_enum_from_string(csmi_node_state_t, input->set_states[i] ))
					{
						csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
						csmutil_logging(error, "  \"%s\" is not a valid value for set_states. Must be a valid csmi_node_state_t", input->set_states[i]);
	                    USAGE();
						return CSMERR_INVALID_PARAM;
					}
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
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
	    csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:         %p", input);
	csmutil_logging(debug, "  address of input:       %p", &input);
	csmutil_logging(debug, "  API_PARAMETER_INPUT_TYPE contains the following:");
	csmutil_logging(debug, "    control_action: %s", input->control_action);
	csmutil_logging(debug, "    limit:          %i", input->limit);
	csmutil_logging(debug, "    msg_id:         %s", input->msg_id);
	csmutil_logging(debug, "    message:        %s", input->message);
	csmutil_logging(debug, "    offset:         %i", input->offset);
	csmutil_logging(debug, "    severity:       %s", csm_get_string_from_enum(csmi_ras_severity_t, input->severity));
	csmutil_logging(debug, "    set_states_count: %i", input->set_states_count);
	csmutil_logging(debug, "    set_states:       %p", input->set_states);
	for(i = 0; i < input->set_states_count; i++){
		csmutil_logging(debug, "      set_states[%i]: %s", i, input->set_states[i]);
	}
	csmutil_logging(debug, "  value of output:        %p", output);
	csmutil_logging(debug, "  address of output:      %p", &output);

	/* Call the actual CSM API */
    return_value = csm_ras_msg_type_query(&csm_obj, input, &output);
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    switch( return_value )
    {
        case CSMI_SUCCESS:
	    	puts("---");
	    	printf("Total_Records: %i\n", output->results_count);
	    	for (i = 0; i < output->results_count; i++) {
	    		printf("RECORD_%i:\n", i+1);
	    		printf("  msg_id:           %s\n", output->results[i]->msg_id);
	    		printf("  control_action:   %s\n", output->results[i]->control_action);
	    		printf("  description:      %s\n", output->results[i]->description);
				printf("  enabled:          %c\n", csm_print_bool(output->results[i]->enabled));
	    		printf("  message:          %s\n", output->results[i]->message);
				printf("  set_state:        %s\n", csm_get_string_from_enum(csmi_node_state_t, output->results[i]->set_state));
	    		printf("  severity:         %s\n", csm_get_string_from_enum(csmi_ras_severity_t, output->results[i]->severity));
	    		printf("  threshold_count:  %d\n", output->results[i]->threshold_count);
	    		printf("  threshold_period: %d\n", output->results[i]->threshold_period);
				printf("  visible_to_users: %c\n", csm_print_bool(output->results[i]->visible_to_users));
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
	
	//Call internal CSM API clean up.
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
