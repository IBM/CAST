/*================================================================================

    csmi/src/diag/cmd/diag_run_begin.c

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

#define API_PARAMETER_INPUT_TYPE csm_diag_run_begin_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_DiagRunBegin_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

struct option longopts[] = {
	//general options
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	//api arguments
	{"allocation_id", required_argument, 0, 'a'}, 
	{"cmd_line",      required_argument, 0, 'c'},
	{"log_dir",       required_argument, 0, 'l'},
	{"run_id",        required_argument, 0, 'r'},
	{0,0,0,0}
};

void help() {
	puts("_____CSM_DIAG_RUN_BEGIN_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_diag_run_begin ARGUMENTS [OPTIONS]");
	puts("  csm_diag_run_begin -a allocation_id -l log_dir -r run_id [-c cmd_line] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to record in the csm database that a diagnostic run has started.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_diag_run_begin expects 3 mandatory arguments");
	puts("    Argument            | Example value | Description  ");                                                 
	puts("    --------------------|---------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -a, --allocation_id | 1             | (INT64) Allocation that this diag_run is part of.");
	puts("    -l, --log_dir       | \"/tmp\"        | (STRING) Location of diagnostic log files.");
	puts("    -r, --run_id        | 1             | (INT64) Diagnostic run id.");
	puts("  OPTIONAL:");
	puts("    csm_diag_run_begin can have 1 optional argument");
	puts("    Argument       | Example value  | Description  ");                                                 
	puts("    ---------------|----------------|--------------");
	puts("    -c, --cmd_line | \"myProgram.py\" | (STRING) How diagnostic program was invoked. Program and arguments.");
	puts("                   |                | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_diag_run_begin -a 1 -l \"/tmp\" -r 1 -c \"myProgram.py\" ");
	puts("____________________");
}

/*
* Summary: Simple command line interface for the CSM API 'diag run begin'. 
*			Works as interface between HCDIAG and the CSM DB.
* 			Takes in the run_id, diag_status, inserted_ras, and log_dir via command line parameters, and inserts the data into the CSM database.
*/
int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
    int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 3;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
	int indexptr = 0;
	char *arg_check = NULL; ///< Used in verifying the long arg values.
	
	/*Set up data*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
    csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:a:l:r:c:s:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
				/*allocation_id*/
                csm_optarg_test("-a, --allocation_id", optarg, USAGE )
                csm_str_to_int64(input->allocation_id, optarg, arg_check, 
                                "-a, --allocation_id",USAGE);
                requiredParameterCounter++;
				break;
			case 'c':
                csm_optarg_test( "-c, --cmd_line", optarg, USAGE )
				input->cmd_line = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'l':
                csm_optarg_test( "-l, --log_dir", optarg, USAGE )
				input->log_dir = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 'r':
				/*run_id*/
                csm_optarg_test("-r, --run_id", optarg, USAGE )
                csm_str_to_int64(input->run_id, optarg, arg_check, 
                                "-r, --run_id",USAGE);
				requiredParameterCounter++;
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

	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  input value:   %p", input);
	csmutil_logging(debug, "  input address: %p", &input);
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    run_id:         %"PRId64, input->run_id);
	csmutil_logging(debug, "    allocation_id:  %"PRId64, input->allocation_id);
	csmutil_logging(debug, "    cmd_line:       %s", input->cmd_line);
	csmutil_logging(debug, "    log_dir:        %s", input->log_dir);
	
	
    return_value = csm_diag_run_begin(&csm_obj, input);
    switch( return_value )
    {
        case CSMI_SUCCESS:
			/* Print as a YAML comment ? debug only? */
            //csmutil_logging(debug, "---\n# csm_diag_run_begin has completed successfully!\n...\n");
			//csmutil_logging(debug, "csm_diag_run_begin has completed successfully!");
			printf("---\n# csm_diag_run_begin has completed successfully!\n...\n");
            break;
        default:
            printf("%s FAILED: errcode: %d errmsg: %s\n",
                argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }
    
    csm_api_object_destroy(csm_obj);
    csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
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
