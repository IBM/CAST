/*================================================================================

    csmi/src/diag/cmd/diag_run_query.c

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
#include "csmi/include/csm_api_diagnostics.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"

#define USAGE csm_free_struct_ptr(csm_diag_run_query_input_t, input); help

void help() {
	puts("_____CSM_DIAG_RUN_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_diag_run_query ARGUMENTS [OPTIONS]");
	puts("  csm_diag_run_query [-a allocation_ids] [-b begin_time_search_begin] [-B begin_time_search_end] [-e end_time_search_begin] [-E end_time_search_end] [-r run_ids] [-s status] [-l limit] [-o offset] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_diag_run' and 'csm_diag_run_history' tables of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_diag_run_query can have 8 optional arguments and requires at least 1");
	puts("    Argument                      | Example value                | Description  ");                                                 
	puts("    ------------------------------|------------------------------|--------------");
	puts("    -a, --allocation_ids          | 1,33,123                     | (LONG INTEGER) Filter results to only include records that have a matching allocation id. This is a csv field of valid allocation ids. API will ignore values less than zero.");
	puts("    -b, --begin_time_search_begin | \"1999-01-15 12:00:00.123456\" | (STRING) A time used to filter results of the SQL query and only include records with a begin_time at or after (ie: '>=' ) this time.");
	puts("    -B, --begin_time_search_end   | \"1999-01-15 13:00:00.123456\" | (STRING) A time used to filter results of the SQL query and only include records with a begin_time at or before (ie: '<=' ) this time.");
	puts("    -e, --end_time_search_begin   | \"2000-01-15 12:34:56.123456\" | (STRING) A time used to filter results of the SQL query and only include records with an end_time at or after (ie: '>=' ) this time.");
	puts("    -E, --end_time_search_end     | \"2000-01-15 13:37:33.134317\" | (STRING) A time used to filter results of the SQL query and only include records with an end_time at or before (ie: '<=' ) this time.");
	puts("    -i, --inserted_ras            | t                            | (CHAR) Filter results to only include records that inserted RAS events or only include records that did not insert RAS events.");
	puts("                                  |                              | Valid Values: 't','f','T','F','1', or '0'");
	puts("    -r, --run_ids                 | 1,42,1337                    | (LONG INTEGER) Filter results to only include records that have a matching run id. This is a csv field of valid run ids. API will ignore values less than zero.");
	puts("    -s, --status                  | \"FAILED,RUNNING\"             | (STRING) Filter results to only include records that have a matching diagnostic status. This is a csv string of valid diagnostic status.");
	puts("                                  |                              | Valid Values: \"CANCELED\", \"COMPLETED\", \"FAILED\", \"COMPLETED_FAIL\", or \"RUNNING\"");
	puts("  FILTERS:");
	puts("    csm_diag_run_query can have 2 optional filters.");
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
	puts("  csm_diag_run_query -r 1");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",                    no_argument,       0, 'h'},
	{"verbose",                 required_argument, 0, 'v'},
	//arguments
	{"allocation_ids",          required_argument, 0, 'a'},
	{"begin_time_search_begin", required_argument, 0, 'b'},
	{"begin_time_search_end",   required_argument, 0, 'B'},
	{"end_time_search_begin",   required_argument, 0, 'e'},
	{"end_time_search_end",     required_argument, 0, 'E'},
	{"inserted_ras",            required_argument, 0, 'i'},
	{"run_ids",                 required_argument, 0, 'r'},
	{"status",                  required_argument, 0, 's'},
	//filters
	{"limit",                   required_argument, 0, 'l'},
	{"offset",                  required_argument, 0, 'o'},
	{0,0,0,0}
};

/*
* Summary: Simple command line interface for the CSM API 'diag run query'. 
*			Works as interface between HCDIAG and the CSM DB.
* 			Takes in the run_id via command line parameters, and queries the data in the CSM database.
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
	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/* For for loops.*/
	int i = 0;
	char *arg_check = NULL; ///< Used in verifying the long arg values.
	
	/*Set up data to call API*/
	csm_diag_run_query_input_t* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(csm_diag_run_query_input_t, input);
	csm_diag_run_query_output_t* output = NULL;

	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:a:b:B:e:E:i:r:s:l:o:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
			{
                csm_optarg_test( "-a, --allocation_ids", optarg, USAGE );
                csm_parse_csv( optarg, input->allocation_ids, input->allocation_ids_count,
                            int64_t, csm_str_to_int64, arg_check, "-a, --allocation_ids", USAGE);
				optionalParameterCounter++;
				break;
			}
			case 'b':
                csm_optarg_test( "-b, --begin_time_search_begin", optarg, USAGE );
				input->begin_time_search_begin = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'B':
                csm_optarg_test( "-B, --begin_time_search_end", optarg, USAGE );
				input->begin_time_search_end = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'e':
                csm_optarg_test( "-e, --end_time_search_begin", optarg, USAGE );
				input->end_time_search_begin = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'E':
                csm_optarg_test( "E, --end_time_search_end", optarg, USAGE );
				input->end_time_search_end = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'i':
            {
                csm_optarg_test( "-i, --inserted_ras", optarg, USAGE );
				if( optarg[0] == 't' || optarg[0] == 'T' || optarg[0] == '1' || 
                    optarg[0] == 'f' ||optarg[0] == 'F' || optarg[0] == '0')
                {
					input->inserted_ras = optarg[0];
				}
                else
                {
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  Parameter [inserted_ras] expected to be 't','f','T','F','1', or '0'.");
					csmutil_logging(error, "  Encountered: %s", optarg);
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
                optionalParameterCounter++;
				break;
            }
            case 'l':
                csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
            case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
			case 'r':
			{
                csm_optarg_test( "-r, --run_ids", optarg, USAGE );
                csm_parse_csv( optarg, input->run_ids, input->run_ids_count,
                            int64_t, csm_str_to_int64, arg_check, "-r, --run_ids", USAGE)
				optionalParameterCounter++;
				break;
			}
			case 's':
			{
                csm_optarg_test( "-s, --status", optarg, USAGE );

                char *saveptr;
                char *statStr = strtok_r(optarg, ",", &saveptr);
                int stat_val;

                while( statStr != NULL )
                {
                    // Get the bit flag
                    csm_get_enum_bit_flag( csmi_diag_run_status_t, statStr, stat_val)
    
                    // If the value greater than NONE this is a valid field. 
                    if ( stat_val > DIAG_NONE )
                    {
                        input->status |= stat_val;
                    }
                    else 
                    {
						csmutil_logging(error, " Invalid status supplied: %s",statStr);
                        USAGE();
                        return CSMERR_INVALID_PARAM;
                    }

                    statStr = strtok_r(NULL, ",", &saveptr);
                }
                    
				optionalParameterCounter++;
				break;
			}
			default:
				csmutil_logging(error, "unknown arg: '%c'\n", opt);
				csmutil_logging(error, "unknown option: '%s'\n", optarg);
                USAGE();
				return CSMERR_INVALID_PARAM; 
		}
	}
	
	/*Handle command line args*/
	argc -= optind;
	argv += optind;
	
	/*Collect mandatory args*/
	/*Check to see if expected number of arguments is correct.*/
	if(requiredParameterCounter < NUMBER_OF_REQUIRED_ARGUMENTS || optionalParameterCounter < MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS)
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
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        csm_free_struct_ptr(csm_diag_run_query_input_t, input);
		return return_value;           
	}

	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:         %p", input);
	csmutil_logging(debug, "  address of input:       %p", &input);
	csmutil_logging(debug, "  csm_diag_run_query_input_t contains the following:");
	csmutil_logging(debug, "    allocation_ids_count:    %i", input->allocation_ids_count);
	csmutil_logging(debug, "    allocation_ids:          %p", input->allocation_ids);
	for(i = 0; i < input->allocation_ids_count; i++){
		csmutil_logging(debug, "      allocation_ids[%i]: %lld", i, input->allocation_ids[i]);
	}
	csmutil_logging(debug, "    begin_time_search_begin: %s",   input->begin_time_search_begin);
	csmutil_logging(debug, "    begin_time_search_end:   %s",   input->begin_time_search_end);
	csmutil_logging(debug, "    end_time_search_begin:   %s",   input->end_time_search_begin);
	csmutil_logging(debug, "    end_time_search_end:     %s",   input->end_time_search_end);
	csmutil_logging(debug, "    inserted_ras:            %c",   input->inserted_ras);
	csmutil_logging(debug, "    limit:                   %i",   input->limit);
	csmutil_logging(debug, "    offset:                  %i",   input->offset);
	csmutil_logging(debug, "    run_ids_count:           %i", input->run_ids_count);
	csmutil_logging(debug, "    run_ids:                 %p", input->run_ids);
	for(i = 0; i < input->run_ids_count; i++){
		csmutil_logging(debug, "      run_ids[%i]: %lld", i, input->run_ids[i]);
	}
	csmutil_logging(debug, "    status:                  %u",   input->status);
	csmutil_logging(debug, "  value of output:        %p", output);
	csmutil_logging(debug, "  address of output:      %p", &output);
	
	/* Call the actual CSM API */
	return_value = csm_diag_run_query(&csm_obj, input, &output);
    
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(csm_diag_run_query_input_t, input);
    
    switch(return_value)
    {
        case CSMI_SUCCESS:
		    puts("---");
		    printf("Total_Records: %u\n", output->num_runs);
		    for (i = 0; i < output->num_runs; i++) {
		    	printf("RECORD_%i:\n", i+1);
		    	printf("  run_id:        %"PRId64"\n",(output->runs[i])->run_id);
		    	printf("  allocation_id: %"PRId64"\n",(output->runs[i])->allocation_id);
		    	printf("  begin_time:    %s\n",       (output->runs[i])->begin_time);
		    	printf("  cmd_line:      %s\n",       (output->runs[i])->cmd_line);
		    	printf("  end_time:      %s\n",       (output->runs[i])->end_time);
		    	printf("  history_time:  %s\n",       (output->runs[i])->history_time);
		    	printf("  inserted_ras:  %c\n",       (output->runs[i])->inserted_ras);
		    	printf("  log_dir:       %s\n",       (output->runs[i])->log_dir);
		    	printf("  status:        %s\n",       (output->runs[i])->diag_status);
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
