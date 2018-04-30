/*================================================================================

    csmi/src/ras/cmd/ras_msg_type_create.c

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
#include <time.h>
#include <sys/time.h>
/* CSM includes */
#include "csmi/include/csm_api_ras.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Defines to make API easier */
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_create_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_create_output_t

///< For use as the usage variable in the input parsers.
#define USAGE  csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_RAS_MSG_TYPE_CREATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_ras_msg_type_create ARGUMENTS [OPTIONS]");
	puts("  csm_ras_msg_type_create -m msg_id -M message [-c control_action] [-d description] [-e enabled] [-s severity] [-S set_state] [-t threshold_count] [-T threshold_period] [-V visible_to_users]  [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to create a new RAS message type and insert a record for the new type into the 'csm_ras_type' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  >0 if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_ras_msg_type_create expects 2 mandatory arguments");
	puts("    Argument      | Example value          | Description  ");                                                 
	puts("    --------------|------------------------|--------------");
	puts("    -m, --msg_id  | \"system.component.id\"    | (STRING) The identifier string for this RAS event. It must be unique.  Typically it consists of three parts separated by periods \"system.component.id\".");
	puts("    -M, --message | \"Message for RAS system\" | (STRING) RAS message to display to the user (pre-variable substitution).");	
	puts("                  |                        | ");
	puts("  OPTIONAL:");
	puts("    csm_ras_msg_type_create can have 9 optional argument");
	puts("    Argument               | Example value            | Description  ");                                                 
	puts("    -----------------------|--------------------------|--------------");
	puts("    -c, --control_action   | \"node_not_ready\"         | (STRING) Name of control action script to invoke for this event.");
	puts("                           |                          | (default = \"NONE\")");
	puts("    -d, --description      | \"This is a description.\" | (STRING) Description of the RAS event.");
	puts("                           |                          | (default = \"NONE\")");
	puts("    -e, --enabled          | 't'                      | (CHAR) Event will be processed by RAS when enabled = 't'.");
	puts("                           |                          | Valid Values: 't' or '1' for true, 'f' or '0' for false");
	puts("                           |                          | (default = 't')");
	puts("    -s, --severity         | \"FATAL\"                  | (STRING) Severity of the RAS event.");
	puts("                           |                          | Valid Values: \"INFO\", \"WARNING\", or \"FATAL\"");
	puts("                           |                          | (default = \"INFO\")");
	puts("    -S, --set_state        | \"SOFT_FAILURE\"           | (STRING) resources associated with the event will be set to this node state when the event hits threshold. setting 'undefined' or not setting this flag will have the node keep its current state.");
	puts("                           |                          | Valid Values: \"undefined\", \"SOFT_FAILURE\" ");
	puts("                           |                          | (default = \"undefined\")");
	puts("    -t, --threshold_count  | 1                        | (INTEGER) Number of times this event has to occur during the 'threshold_period' before taking action on the RAS event. Values less than 0 will be set to 0.");
	puts("                           |                          | (default = 1)");
	puts("    -T, --threshold_period | 5                        | (INTEGER) Period in seconds over which to count the 'threshold_count'. Values less than 0 will be set to 0.");
	puts("                           |                          | (default = 5)");
	puts("    -V, --visible_to_users | 't'                      | (CHAR) If visible_to_users = 't' is configured, then these events will be returned in the output of csm_ras_event_query_allocation.");
	puts("                           |                          | Valid Values: 't' or '1' for true, 'f' or '0' for false");
	puts("                           |                          | (default = 't')");
	puts("                           |                          | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_ras_msg_type_create -m \"test.msg.id\" -M \"Message for RAS system\" ");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",             no_argument,       0, 'h'},
	{"verbose",          required_argument, 0, 'v'},
	//arguments
	{"control_action",   required_argument, 0, 'c'},
	{"description",      required_argument, 0, 'd'},
	{"enabled",          required_argument, 0, 'e'},
	{"msg_id",           required_argument, 0, 'm'},
	{"message",          required_argument, 0, 'M'},
	{"severity",         required_argument, 0, 's'},
	{"set_state",        required_argument, 0, 'S'},
	{"threshold_count",  required_argument, 0, 't'},
	{"threshold_period", required_argument, 0, 'T'},
	{"visible_to_users", required_argument, 0, 'V'},
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
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 2;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
    int option_index = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/* TEMP WORK AROUND for: no default value in csm_init_struct_ptr for strings */
	input->control_action = strdup("NONE");
	input->description    = strdup("NONE");
	input->set_state      = CSM_NODE_NO_DEF;
	input->severity       = CSM_RAS_INFO;

	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:c:d:e:m:M:s:S:t:T:V:", longopts, &option_index)) != -1) {
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
			case 'd':
                csm_optarg_test( "-d, --description", optarg, USAGE )
				input->description = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'e':
                csm_optarg_test( "-e, --enabled", optarg, USAGE )
				if( optarg[0] == 't' || optarg[0] == '1' )
                {
					input->enabled = CSM_TRUE;
				}
                else if ( optarg[0] == 'f' || optarg[0] == '0' )
                {
                    input->enabled = CSM_FALSE;
                }
                else
                {
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  Parameter [enabled] expected to be"
                        " either 't', 'f', '0', or '1'. ");
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				optionalParameterCounter++;
				break;
			case 'm':
                csm_optarg_test( "-m, --msg_id", optarg, USAGE )
				input->msg_id = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 'M':
                csm_optarg_test( "-M, --message", optarg, USAGE )
				input->message = strdup(optarg);
				requiredParameterCounter++;
				break;
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
			/*set_state*/
			{
                csm_optarg_test( "-S, --set_state", optarg, USAGE )
				if( strcmp(optarg,"undefined")    == 0 || 
                    strcmp(optarg,"SOFT_FAILURE") == 0  
                )
                {
					int temp_state = csm_get_enum_from_string(csmi_node_state_t, optarg);
					input->set_state = temp_state != -1 ? (csmi_node_state_t) temp_state : csm_enum_max(csmi_node_state_t);
				}
				else
                {
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  %s is not a valid value for set_state.", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				optionalParameterCounter++;
				break;
            }
			case 't':
                csm_optarg_test( "-t, --threshold_count", optarg, USAGE )
                csm_str_to_int32( input->threshold_count, optarg, arg_check,
                                "-t, --threshold_count", USAGE );
                
                if ( input->threshold_count < 0 ) input->threshold_count = 0;

				optionalParameterCounter++;
				break;
			case 'T':
                csm_optarg_test( "-T, --threshold_period", optarg, USAGE )
                csm_str_to_int32( input->threshold_period, optarg, arg_check,
                                "-T, --threshold_period", USAGE );
                
                if ( input->threshold_period < 0 ) input->threshold_period = 0;

				optionalParameterCounter++;
				break;
			case 'V':
                csm_optarg_test( "-V, --visible_to_users", optarg, USAGE )
				if( optarg[0] == 't' || optarg[0] == '1' )
                {
					input->visible_to_users = CSM_TRUE;
				}
                else if ( optarg[0] == 'f' || optarg[0] == '0' )
                {
                    input->visible_to_users = CSM_FALSE;
                }
				else{
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  Parameter [visible_to_users] expected to be "
                        "either 't', 'f', '0', or '1'.");
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				optionalParameterCounter++;
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
	if( requiredParameterCounter < NUMBER_OF_REQUIRED_ARGUMENTS || 
        optionalParameterCounter < MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS)
    {
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
	if( return_value != 0)
    {
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. "
            "Success is required to be able to communicate between library and daemon. "
            "Are the daemons running?", return_value);
	    csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    control_action:   %s", input->control_action);
	csmutil_logging(debug, "    description:      %s", input->description);
	csmutil_logging(debug, "    enabled:          %c", clean_char(input->enabled));
	csmutil_logging(debug, "    msg_id:           %s", input->msg_id);
	csmutil_logging(debug, "    message:          %s", input->message);
	csmutil_logging(debug, "    set_state:        %s", csm_get_string_from_enum(csmi_node_state_t, input->set_state));
	csmutil_logging(debug, "    severity:         %s", csm_get_string_from_enum(csmi_ras_severity_t, input->severity));
	csmutil_logging(debug, "    threshold_count:  %d", input->threshold_count);
	csmutil_logging(debug, "    threshold_period: %d", input->threshold_period);
	csmutil_logging(debug, "    visible_to_users: %c", clean_char(input->visible_to_users));

    return_value = csm_ras_msg_type_create(&csm_obj, input, &output);
	//Use CSM API free to release input. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    switch( return_value )
    {
        case CSMI_SUCCESS:
            printf( "csm_ras_msg_type_create has completed successfully!\n"
                    "    insert_successful: %c\n"
                    "    msg_id:            %s\n", 
                    (output->insert_successful ? 'y' : 'n'), 
                    output->msg_id );
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
