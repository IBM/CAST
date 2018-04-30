/*================================================================================

    csmi/src/diag/cmd/diag_run_query_details.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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
#include "csmi/include/csm_api_diagnostics.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"

#define EXPECTED_NUMBER_OF_ARGUMENTS 1

///< For use as the usage variable in the input parsers.
#define USAGE help

struct option longopts[] = {
  {"help",          no_argument,       0, 'h'},
  {"verbose",       required_argument, 0, 'v'},
  {"run_id",        required_argument, 0, 'r'},
  {0,0,0,0}
};

void help() {
	puts("_____CSM_DIAG_RUN_QUERY_DETAILS_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_diag_run_query_details ARGUMENTS [OPTIONS]");
	puts("  csm_diag_run_query_details -r run_id [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to acquire information on a particular 'diag run' along with information on all 'diag results' related to that 'diag run'.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_diag_run_query_details expects 1 mandatory argument");
	puts("    Argument      | Example value | Description  ");                                                 
	puts("    --------------|---------------|--------------");
	puts("    -r, --run_id  | 1             | (LONG INTEGER) Diagnostic run id.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_diag_run_query_details -r 1");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

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
	/*Variables for checking cmd line args*/
	int opt;
	int indexptr = 0;
	int parameterCounter = 0;
    char *arg_check = NULL; ///< Used in verifying the long arg values.
	
	/*Set up data for API call*/
    csm_diag_run_query_details_input_t args;
    args.run_id = 0;

	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:r:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'r':
                csm_optarg_test( "-r, --run_id", optarg, USAGE )
                csm_str_to_int64( args.run_id,  optarg, arg_check, "-r, --run_id", USAGE );
				parameterCounter++;
				break;
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
	if(parameterCounter < EXPECTED_NUMBER_OF_ARGUMENTS){
		/*We don't have the correct number of arguments passed in. We expecting EXPECTED_NUMBER_OF_ARGUMENTS.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s). Encountered %i parameter(s). Expected %i mandatory parameter(s).", parameterCounter, EXPECTED_NUMBER_OF_ARGUMENTS);
		USAGE();
        return CSMERR_MISSING_PARAM;
	}
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
		return return_value;           
	}

    csm_diag_run_query_details_output_t * output = NULL;

    // Run the query.
	return_value = csm_diag_run_query_details( &csm_obj, &args, &output );
    switch(return_value)
    {
        case CSMI_SUCCESS:
	        puts("---");
            csmi_diag_run_t *runData = output->run_data;
            csmi_diag_run_query_details_result_t **resultData = output->details;
	        
            if(runData)
            {
	        	printf("runData:\n");
	        	printf("  history_time:     %s \n", runData->history_time);
	        	printf("  run_id:           %"PRId64" \n", runData->run_id);
	        	printf("  allocation_id:    %"PRId64" \n", runData->allocation_id);
	        	printf("  begin_time:       %s \n", runData->begin_time);
	        	printf("  end_time:         %s \n", runData->end_time);
	        	printf("  diag_status:      %s \n", runData->diag_status);
	        	printf("  inserted_ras:     %c \n", runData->inserted_ras);
	        	printf("  log_dir:          %s \n", runData->log_dir);
	        	printf("  cmd_line:         %s \n", runData->cmd_line);
	        	printf("Total_Result_Records: %"PRId64"\n", output->num_details);

	        	uint32_t i = 0;
	        	for(i = 0; i < output->num_details; i++){
	        		printf("Result_Record_%u:\n", i+1);
	        		printf("  history_time:     %s\n",  resultData[i]->history_time);
	        		printf("  run_id:           %"PRId64" \n",  resultData[i]->run_id);
	        		printf("  test_name:        %s\n",  resultData[i]->test_name);
	        		printf("  node_name:        %s\n",  resultData[i]->node_name);
	        		printf("  serial_number:    %s\n",  resultData[i]->serial_number);
	        		printf("  begin_time:       %s\n",  resultData[i]->begin_time);
	        		printf("  end_time:         %s\n",  resultData[i]->end_time);
	        		printf("  status:           %s\n",  resultData[i]->status);
	        		printf("  log_file:         %s\n",  resultData[i]->log_file);
	        	}
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
