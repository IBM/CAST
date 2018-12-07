/*================================================================================

    csmi/src/wm/cmd/allocation_step_begin.c

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
/*CORAL includes*/
#include "utilities/include/string_tools.h"
/*CSM Include*/
#include "csmi/include/csm_api_workload_manager.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

#define API_PARAMETER_INPUT_TYPE  csm_allocation_step_begin_input_t
#define API_PARAMETER_OUTPUT_TYPE 

#define EXPECTED_NUMBER_OF_ARGUMENTS 11

///< For use as the usage variable in the input parsers.
#define USAGE  csm_free_struct(API_PARAMETER_INPUT_TYPE, input); help

static struct option long_options[] =
{
	{"help",                    no_argument,       0, 'h'},
	{"verbose",                 required_argument, 0, 'v'},
	{"allocation_id",           required_argument, 0, 'a'},
	{"argument",                required_argument, 0, 'r'},
	{"compute_nodes",           required_argument, 0, 'c'},
	{"environment_variable",    required_argument, 0, 'e'},
	{"executable",              required_argument, 0, 'x'},
	{"num_gpus",                required_argument, 0, 'g'},
	{"projected_memory",        required_argument, 0, 'm'},
	{"num_processors",          required_argument, 0, 'p'},
	{"num_tasks",               required_argument, 0, 't'},
	{"step_id",                 required_argument, 0, 'S'},
	{"working_directory",       required_argument, 0, 'w'},
	{0, 0, 0, 0}
};

void help(){
	puts("_____CSM_ALLOCATION_STEP_BEGIN_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_step_begin ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_step_begin --step_id value --allocation_id value --executable value --working_directory value --argument value --environment_variable value --num_processors value --num_gpus value --projected_memory value --num_tasks value --compute_nodes value [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to enter a record into the 'csm_step' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_allocation_step_begin expects 11 mandatory arguments");
	puts("    Argument                      | Example value                 | Description  ");                                                 
	puts("    ------------------------------|-------------------------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -a, --allocation_id           | 1                             | Allocation that this step is part of.");
	puts("    -r, --argument                | \"my argument\"                 | Arguments / parameters.");
	puts("    -c, --compute_nodes           | \"node01,node02\"               | A comma separated list of nodes associated with this step. Used to populate the \"csm_step_node\" table of the CSM DB.");
	puts("    -e, --environment_variable    | \"my environment_variable\"     | Environment variables.");
	puts("    -x, --executable              | \"my executable\"               | Executable / command name / application name.");
	puts("    -g, --num_gpus                | 1                             | The number of gpus that are available.");
	puts("    -m, --projected_memory        | 512                           | The amount of memory available.");
	puts("    -p, --num_processors          | 1                             | Total number of processes running in this step.");
	puts("    -t, --num_tasks               | 1                             | Total number of tasks in a job or step.");
	puts("    -S, --step_id                 | 1                             | Uniquely identify this step.");
	puts("    -w, --working_directory       | \"my working_directory\"        | Working directory.");                    
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_step_begin --step_id 1 --allocation_id 1 --state r --executable \"my executable\" --working_directory \"my working_directory\" --argument \"my argument\" --environment_variable \"my environment_variable\" --num_processors 1 --num_gpus 1 --projected_memory 512 --num_tasks 1 --compute_nodes \"node01,node02\"");
	puts("____________________");
}

/*
* Summary: command line interface for the CSM API 'allocation step begin'. 
*           for user managed allocations
* 			Takes in the fields for a csmi_allocation_step_t struct as command line parameters, and inserts the data into the CSM database.
*/
int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int return_value = 0;
	int parameterCounter = 0;
    char *arg_check = NULL; ///< Used in verifying the long arg values.
	
	/*Variables for checking cmd line args*/
	int opt;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE input;
    csm_init_struct(API_PARAMETER_INPUT_TYPE, input);
	
	/* getopt_long stores the option index here. */
	int option_index = 0;
	
	/*check optional args*/
	/*Only include 'hv:' to limit single char parameters*/
	/* well, now we are thinking of supporting single char options for everything*/
	while((opt = getopt_long (argc, argv, "hv:a:r:c:e:x:g:m:p:t:i:s:S:w:", long_options, &option_index)) != -1){
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			/*Specific API parameters.*/
			/*Populate with data received from cmd line parameters.*/
			case 'a':
				/*allocation_id*/
                csm_optarg_test( "-a, --allocation_id", optarg, USAGE )
                csm_str_to_int64( input.allocation_id, optarg, arg_check,
                                "-a, --allocation_id", USAGE )
				parameterCounter++;
				break;
			case 'S':
				/*step_id*/
                csm_optarg_test( "-S, --step_id", optarg, USAGE )
                csm_str_to_int64( input.step_id, optarg, arg_check,
                                "-S, --step_id", USAGE )
				parameterCounter++;
				break;
			case 'x':
				/*executable*/
                csm_optarg_test( "-x, --executable", optarg, USAGE )
				input.executable = strdup(optarg);
				parameterCounter++;
				break;
			case 'w':
				/*working_directory*/
                csm_optarg_test( "-w, --working_directory", optarg, USAGE )
				input.working_directory = strdup(optarg);
				parameterCounter++;
				break;
			case 'r':
				/*argument*/
                csm_optarg_test( "-r, --argument", optarg, USAGE )
				input.argument = strdup(optarg);
				parameterCounter++;
				break;
			case 'e':
				/*environment_variable*/
                csm_optarg_test( "-e, --environment_variable", optarg, USAGE )
				input.environment_variable = strdup(optarg);
				parameterCounter++;
				break;
			case 'p':
				/*num_processors*/
                csm_optarg_test( "-p, --num_processors", optarg, USAGE )
                csm_str_to_int32( input.num_processors, optarg, arg_check,
                                "-p, --num_processors", USAGE )
				parameterCounter++;
				break;
			case 'g':
				/*num_gpus*/
                csm_optarg_test( "-g, --num_gpus", optarg, USAGE )
                csm_str_to_int32( input.num_gpus, optarg, arg_check,
                                "-g, --num_gpus", USAGE )
				parameterCounter++;
				break;
			case 'm':
				/*projected_memory*/
                csm_optarg_test( "-m, --projected_memory", optarg, USAGE )
                csm_str_to_int32( input.projected_memory, optarg, arg_check,
                                "-m, --projected_memory", USAGE )
				parameterCounter++;
				break;
			case 't':
				/*num_tasks*/
                csm_optarg_test( "-t, --num_tasks", optarg, USAGE )
                csm_str_to_int32( input.num_tasks, optarg, arg_check,
                                "-t, --num_tasks", USAGE )
				parameterCounter++;
				break;
			case 'c':
			{
                csm_optarg_test( "-c, --compute_nodes", optarg, USAGE )
                csm_parse_csv( optarg, input.compute_nodes, input.num_nodes, char*,
                            csm_str_to_char, NULL, "-c, --compute_nodes", USAGE )
				parameterCounter++;
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
	if(parameterCounter != EXPECTED_NUMBER_OF_ARGUMENTS){
		/*We don't have the correct number of arguments passed in. We expecting EXPECTED_NUMBER_OF_ARGUMENTS.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s). Encountered %i parameter(s). Expected %i mandatory parameter(s).", parameterCounter, EXPECTED_NUMBER_OF_ARGUMENTS);
        USAGE();
		return CSMERR_INVALID_PARAM;
	}
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required"
            " to be able to communicate between library and daemon. Are the daemons running?", 
            return_value);
        csm_free_struct(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}

	/*All that just to call the api.*/
	return_value = csm_allocation_step_begin(&csm_obj, &input);
	
    if ( return_value != CSMI_SUCCESS )
    {
        printf("%s FAILED: errcode: %d errmsg: %s\n",
            argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));

        csm_print_node_errors(csm_obj)
	}

    // Destroy the csm_obj.
    csm_api_object_destroy(csm_obj);
	csm_free_struct(API_PARAMETER_INPUT_TYPE, input);

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
