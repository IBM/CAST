/*================================================================================

    csmi/src/ras/cmd/csm_ras_event_query.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "csmi/include/csm_api_ras.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Defines to make API easier */
#define API_PARAMETER_INPUT_TYPE csm_ras_event_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_event_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE help

void help() {
	puts("_____CSM_RAS_EVENT_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_ras_event_query ARGUMENTS [OPTIONS]");
	puts("  csm_ras_event_query [-b begin_timestamp] [-c control_action] [-e end_timestamp] [-l location_name] [-m message_id] [-M message] [-n limit] [-o offset] [-O order_by] [-s severity] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the \"csm_ras_event\" table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_ras_event_query can have 10 optional arguments and requires at least 1");
	puts("    Argument                             | Example value                | Description  ");                                                 
	puts("    -------------------------------------|------------------------------|--------------");
	puts("    -b, --begin_timestamp                | \"1999-01-15 12:00:00.123456\" | (STRING) A time used to filter results of the SQL query and only include records with a 'time_stamp' at or after (ie: '>=' ) this time.");
	puts("    -B, --master_time_stamp_search_begin | \"1999-01-15 12:00:00.123456\" | (STRING) A time used to filter results of the SQL query and only include records with a 'master_time_stamp' at or after (ie: '>=' ) this time.");
	puts("    -c, --control_action                 | \"NONE\"                       | (STRING) 'control_action' search string. The 'control_action' is the name of control action script to invoke for this event.");
	puts("    -e, --end_timestamp                  | \"1999-01-15 13:00:00.123456\" | (STRING) A time used to filter results of the SQL query and only include records with a 'time_stamp' at or before (ie: '<=' ) this time.");
	puts("    -E, --master_time_stamp_search_end   | \"1999-01-15 13:00:00.123456\" | (STRING) A time used to filter results of the SQL query and only include records with a 'master_time_stamp' at or before (ie: '<=' ) this time.");
	puts("    -l, --location_name                  | \"node_01\"                    | (STRING) 'location_name' search string. The node name or location name for this RAS event.");
	puts("    -m, --msg_id                         | \"csm.%%\"                     | (STRING) The 'msg_id' search string. The 'msg_id' is the identifier string for this RAS event.");
	puts("    -M, --message                        | \"Message for RAS system\"     | (STRING) 'message' search string. 'message' is the RAS message to display to the user (pre-variable substitution).");
	puts("    -r, --rec_id                         | 13                           | (INT 64) 'rec_id' search string. 'rec_id' is the unique identifier for this specific ras event.");
	puts("    -s, --severity                       | \"INFO\"                       | (STRING) 'severity' search string. 'severity' is the severity of the RAS event.");
	puts("                                         |                              | Valid Values: \"INFO\", \"WARNING\", or \"FATAL\"");
	puts("  FILTERS:");
	puts("    csm_ras_event_query can have 3 optional filters.");
	puts("    Argument       | Example value | Description  ");                                                 
	puts("    ---------------|---------------|--------------");
	puts("    -n, --limit    | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
	puts("    -o, --offset   | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("    -O, --order_by | a             | (CHAR) SQL 'ORDER BY' numeric value. Default Value: 'a'");
	puts("                                   | Valid Values: [a] = 'ORDER BY rec_id ASC NULLS LAST'"); 
	puts("                                   |               [b] = 'ORDER BY rec_id DESC NULLS LAST'");
	puts("                                   |               [c] = 'ORDER BY time_stamp ASC NULLS LAST'"); 
	puts("                                   |               [d] = 'ORDER BY time_stamp DESC NULLS LAST'");
	puts("                                   |               [e] = 'ORDER BY master_time_stamp ASC NULLS LAST'");
	puts("                                   |               [f] = 'ORDER BY master_time_stamp DESC NULLS LAST'");
	puts("                                   |               [g] = 'ORDER BY location_name ASC NULLS LAST'");
	puts("                                   |               [h] = 'ORDER BY location_name DESC NULLS LAST'");
	puts("                                   |               [i] = 'ORDER BY msg_id ASC NULLS LAST'");
	puts("                                   |               [j] = 'ORDER BY msg_id DESC NULLS LAST'");
	puts("                                   |               [k] = 'ORDER BY severity ASC NULLS LAST'");
	puts("                                   |               [l] = 'ORDER BY severity DESC NULLS LAST'");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_ras_event_query -m \"csm.%%\" ");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",             no_argument,       0, 'h'},
	{"verbose",          required_argument, 0, 'v'},
	//arguments
	{"begin_timestamp",  required_argument, 0, 'b'},
	{"control_action",   required_argument, 0, 'c'},
	{"end_timestamp",    required_argument, 0, 'e'},
	{"location_name",    required_argument, 0, 'l'},
	{"message",          required_argument, 0, 'M'},
	{"msg_id",           required_argument, 0, 'm'},
	{"rec_id",           required_argument, 0, 'r'},
	{"severity",         required_argument, 0, 's'},
	//filters
	{"limit",            required_argument, 0, 'n'},
	{"offset",           required_argument, 0, 'o'},
	{"order_by",         required_argument, 0, 'O'},
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
    while ((opt = getopt_long(argc, argv, "hv:b:B:c:e:E:M:l:m:n:o:O:r:s:", longopts, &option_index)) != -1) {

        switch (opt) {
			case 'h':
	            USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;	
			/*Specific API parameters.*/
			/*Populate with data received from cmd line parameters.*/
			case 'b':
				input->start_time_stamp = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'B':
				input->master_time_stamp_search_begin = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'c':
				input->control_action = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'e':
				input->end_time_stamp = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'E':
				input->master_time_stamp_search_end = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'm':
				input->msg_id = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'l':
				input->location_name = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'M':
				input->message = strdup(optarg);
				optionalParameterCounter++;
				break;
            case 'n':
                csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
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
					|| optarg[0] == 'i' 
					|| optarg[0] == 'j' 
					|| optarg[0] == 'k' 
					|| optarg[0] == 'l' 
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
			case 'r':
				csm_optarg_test( "-r, --rec_id", optarg, USAGE );
				csm_str_to_int64( input->rec_id, optarg, arg_check, "-r, --rec_id", USAGE );
				optionalParameterCounter++;
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
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    start_time_stamp: %s", input->start_time_stamp);
	csmutil_logging(debug, "    control_action:   %s", input->control_action);
	csmutil_logging(debug, "    end_time_stamp:   %s", input->end_time_stamp);
	csmutil_logging(debug, "    location_name:    %s", input->location_name);
	csmutil_logging(debug, "    msg_id:           %s", input->msg_id);
	csmutil_logging(debug, "    message:          %s", input->message);
	csmutil_logging(debug, "    severity:         %s", input->severity);
	csmutil_logging(debug, "    limit:            %"PRId32"\n", input->limit);
	csmutil_logging(debug, "    offset:           %"PRId32"\n", input->offset);
	csmutil_logging(debug, "    order_by:         %c", input->order_by);
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
	    csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}

	/* Call the actual CSM API */
    return_value = csm_ras_event_query(&csm_obj, input, &output);
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    switch(return_value)
    {
        case CSMI_SUCCESS:
	    	puts("---");
	    	printf("Total_Records: %i\n", output->results_count);
	    	for (i = 0; i < output->results_count; i++) {
	    		printf("RECORD_%i:\n", i+1);
	    		printf("  rec_id:            %"PRId64"\n", output->results[i]->rec_id);
				printf("  msg_id:            %s\n", output->results[i]->msg_id);
	    		printf("  severity:          %s\n", output->results[i]->severity);
	    		printf("  time_stamp:        %s\n", output->results[i]->time_stamp);
				printf("  master_time_stamp: %s\n", output->results[i]->master_time_stamp);
	    		printf("  location_name:     %s\n", output->results[i]->location_name);
	    		printf("  count:             %d\n", output->results[i]->count);
	    		printf("  control_action:    %s\n", output->results[i]->control_action);
	    		printf("  message:           %s\n", output->results[i]->message);
	    		printf("  raw_data:          %s\n", output->results[i]->raw_data);
				printf("  kvcsv:             %s\n", output->results[i]->kvcsv);
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
