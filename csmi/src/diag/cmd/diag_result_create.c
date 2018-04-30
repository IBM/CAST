/*================================================================================

    csmi/src/diag/cmd/diag_result_create.c

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

#define EXPECTED_NUMBER_OF_ARGUMENTS 7

///< For use as the usage variable in the input parsers.
#define USAGE  csm_free_struct_ptr(csm_diag_result_create_input_t, resultData); help

struct option longopts[] = {
  {"help",          no_argument,       0, 'h'},
  {"verbose",       required_argument, 0, 'v'},
  {"begin_time",    required_argument, 0, 'b'}, 
  {"log_file",      required_argument, 0, 'l'},
  {"node_name",     required_argument, 0, 'n'},
  {"run_id",        required_argument, 0, 'r'},
  {"status",        required_argument, 0, 's'},
  {"serial_number", required_argument, 0, 'S'},
  {"test_name",     required_argument, 0, 't'},
  {0,0,0,0}
};

void help(){
	puts("_____CSM_DIAG_RESULT_CREATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_diag_result_create ARGUMENTS [OPTIONS]");
	puts("  csm_diag_result_create -b begin_time -l log_file -n node_name -r run_id -s status -S serial_number -t test_name [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to enter a record in the csm_diag_result table of CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm diag result create expects 7 mandatory arguments");
	puts("    Argument            | Example value                | Description  ");                                                 
	puts("    --------------------|------------------------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -b, --begin_time    | \"1999-01-27 13:37:33.134317\" | (STRING) The time when the task begins.");
	puts("    -l, --log_file      | \"myLogFile\"                  | (STRING) Location of diagnostic log file.");
	puts("    -n, --node_name     | \"node01\"                     | (STRING) Identifies which node.");
	puts("    -r, --run_id        | 1                            | (INT64) Diagnostic run id.");
	puts("    -s, --status        | \"PASS\"                       | (CHAR[16]) Hardware status after the diagnostic finishes.");
	puts("                        |                              | Valid values: \"PASS\" or \"FAIL\"");
	puts("    -S, --serial_number | \"mySerialNumber\"             | (STRING) Serial number of the field replaceable unit (fru) that this diagnostic was run against.");
	puts("    -t, --test_name     | \"My test name\"               | (STRING) The name of the specific test case.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("FULL EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_diag_result_create -b \"1999-01-27 13:37:33.134317\" -l \"myLogFile\" -n \"node01\" -r 1 -s \"PASS\" -S \"mySerialNumber\" -t \"My test name\"");
	puts("____________________");
}

/*
* Summary: Simple command line interface for the CSM API 'diag result create'. 
*			Works as interface between HCDIAG and the CSM DB.
* 			Takes in the fields for a csm_diag_result_create_input_t struct as command line parameters, and inserts the data into the CSM database.
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
	csm_diag_result_create_input_t* resultData = NULL;
    csm_init_struct_ptr(csm_diag_result_create_input_t, resultData);

	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:b:l:n:r:s:S:t:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case'b':
                csm_optarg_test( "-b, --begin_time", optarg, USAGE )
				resultData->begin_time = strdup(optarg);
				parameterCounter++;
				break;
			case'l':
                csm_optarg_test( "-l, --log_file", optarg, USAGE )
				resultData->log_file = strdup(optarg);
				parameterCounter++;
				break;
			case'n':
                csm_optarg_test( "-n, --node_name", optarg, USAGE )
				resultData->node_name = strdup(optarg);
				parameterCounter++;
				break;
			case 'r':
                csm_optarg_test( "-r, --run_id", optarg, USAGE )
                csm_str_to_int64( resultData->run_id, optarg, arg_check,
                                "-r, --run_id", USAGE )
				parameterCounter++;
				break;
			case 's':
                csm_optarg_test( "-s, --status", optarg, USAGE )
				strncpy(resultData->status, optarg,16);
				resultData->status[15] = '\0';
				parameterCounter++;
				break;
			case 'S':
                csm_optarg_test( "-S, --serial_number", optarg, USAGE )
				resultData->serial_number = strdup(optarg);
				parameterCounter++;
				break;
			case 't':
                csm_optarg_test( "-t, --test_name", optarg, USAGE )
				resultData->test_name = strdup(optarg);
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
	if(return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        csm_free_struct_ptr(csm_diag_result_create_input_t, resultData);
		return return_value;
	}
	
            
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  csm_diag_result_create_input_t contains the following:");
	csmutil_logging(debug, "    run_id:         %"PRId64, resultData->run_id);
	csmutil_logging(debug, "    test_name:      %s", resultData->test_name);
	csmutil_logging(debug, "    node_name:      %s", resultData->node_name);
	csmutil_logging(debug, "    serial_number:  %s", resultData->serial_number);
	csmutil_logging(debug, "    status:         %s", resultData->status);
	csmutil_logging(debug, "    log_file:       %s", resultData->log_file);
	
	/*All that just to call the api.*/
	return_value = csm_diag_result_create(&csm_obj, resultData);
    switch( return_value )
    {
        case CSMI_SUCCESS:
            printf("csm_diag_result_create has completed successfully!\n");
            break;

        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n",
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }

    csm_api_object_destroy(csm_obj);
    csm_free_struct_ptr(csm_diag_result_create_input_t, resultData);

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
