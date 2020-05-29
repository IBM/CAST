/*================================================================================

    csmi/src/diag/cmd/diag_run_end.c

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

#define EXPECTED_NUMBER_OF_ARGUMENTS 3

#define API_PARAMETER_INPUT_TYPE csm_diag_run_end_input_t

///< For use as the usage variable in the input parsers.
#define USAGE help

struct option longopts[] = {
  {"help",          no_argument,       0, 'h'},
  {"verbose",       required_argument, 0, 'v'},
  {"inserted_ras",  required_argument, 0, 'i'},
  {"run_id",        required_argument, 0, 'r'},
  {"status",        required_argument, 0, 's'},
  {0,0,0,0}
};

void help(){
	puts("_____CSM_DIAG_RUN_END_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_diag_run_end ARGUMENTS [OPTIONS]");
	puts("  csm_diag_run_end -i inserted_ras -r run_id -s status [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to record in the csm database that a diagnostic run has ended.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_diag_run_end expects 3 mandatory argument");
	puts("    Argument           | Example value | Description  ");                                                 
	puts("    -------------------|---------------|--------------");
	puts("    -i, --inserted_ras | 'f'           | (CHAR) Inserted diagnostic RAS event.");
	puts("                       |               | Valid values: 't' or '1' for true, 'f' or '0' for false");
	puts("    -r, --run_id       | 1             | (INT64) Diagnostic run id.");
	puts("    -s, --status       | \"COMPLETED\"   | (CHAR[16]) Diagnostic status.");
	puts("                       |               | Valid values: \"RUNNING\", \"COMPLETED\", \"CANCELED\", \"COMPLETED_FAIL\", or \"FAILED\"");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_diag_run_end -i f -r 1 -s \"COMPLETED\"");
	puts("____________________");
}

/*
* Summary: Simple command line interface for the CSM API 'diag run end'. 
*			Works as interface between HCDIAG and the CSM DB.
* 			Takes in the run_id, diag_status, and inserted_ras via command line parameters, and inserts the data into the CSM database.
*/
int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
    int return_value = 0;
	char *arg_check = NULL; ///< Used in verifying the long arg values.
	/*Variables for checking cmd line args*/
	int opt;
	int indexptr = 0;
	int parameterCounter = 0;

	/*Set up data*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
    csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:i:r:s:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
				USAGE();
				return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'i':
            {
                csm_optarg_test( "-i, --inserted_ras", optarg, USAGE )

				if( optarg[0] == 't' || optarg[0] == '1' || optarg[0] == 'f' || optarg[0] == '0')
                {
					input->inserted_ras = optarg[0];
				}
                else
                {
					csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
					csmutil_logging(error, "  Parameter [inserted_ras] expected to be either 't', 'f', '0', or '1'. ");
                    USAGE();
					return CSMERR_INVALID_PARAM;
				}
				parameterCounter++;
				break;
            }
			case 'r':
                csm_optarg_test( "-r, --run_id", optarg, USAGE )
                csm_str_to_int64( input->run_id, optarg, arg_check, "-r, --run_id", USAGE) ;
				parameterCounter++;
				break;
			case 's':
                csm_optarg_test( "-s, --status", optarg, USAGE )
                strncpy(input->status, optarg,16);
				input->status[15] = '\0';
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
	if(parameterCounter != EXPECTED_NUMBER_OF_ARGUMENTS){
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
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  csm_diag_run_end_input_t contains the following:");
	csmutil_logging(debug, "    run_id:         %"PRId64, input->run_id);
	csmutil_logging(debug, "    diag_status:    %s", input->status);
	csmutil_logging(debug, "    inserted_ras:   %c", input->inserted_ras);
	
	return_value = csm_diag_run_end(&csm_obj, input);
    switch(return_value)
    {
        case CSMI_SUCCESS:
			// this is only need in debug mode. commenting until we implement the debug print we discussed. 
            // printf("csm_diag_run_end has completed successfully!\n");
        	printf("---\n# csm_diag_run_end has completed successfully!\n...\n");
            break;

        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n",
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
            break;
    }

    //Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
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
