/*================================================================================

    csmi/src/wm/cmd/allocation_step_end.c

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
#include "csmi/include/csm_api_workload_manager.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

#define API_PARAMETER_INPUT_TYPE  csm_allocation_step_end_input_t
#define API_PARAMETER_OUTPUT_TYPE 

#define EXPECTED_NUMBER_OF_ARGUMENTS 12

#define USAGE csm_free_struct(API_PARAMETER_INPUT_TYPE, input); help

static struct option long_options[] =
{
	{"help",             no_argument,       0, 'h'},
	{"verbose",          required_argument, 0, 'v'},
	{"allocation_id",    required_argument, 0, 'a'},
	{"cpu_stats",        required_argument, 0, 'c'},
	{"exit_status",      required_argument, 0, 'e'},
	{"error_message",    required_argument, 0, 'E'},
	{"gpu_stats",        required_argument, 0, 'G'},
	{"io_stats",         required_argument, 0, 'i'},
	{"memory_stats",     required_argument, 0, 'm'},
	{"max_memory",       required_argument, 0, 'M'},
	{"omp_thread_limit", required_argument, 0, 'n'},
	{"step_id",          required_argument, 0, 's'},
	{"total_u_time",     required_argument, 0, 't'},
	{"total_s_time",     required_argument, 0, 'T'},
	{0, 0, 0, 0}
};

void help(){
	puts("_____CSM_ALLOCATION_STEP_END_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_step_end ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_step_end -a allocation_id -c cpu_stats -e exit_status -E err_text -G gpu_stats -i io_stats -m memory_stats -M max_memory -n total_num_threads -s step_id -t total_u_time -T total_s_time");
	puts("");
	puts("SUMMARY: Used to move a record in the 'csm_step' table to the 'csm_step_history' of CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_allocation_step_end expects 12 mandatory parameters");
	puts("    Argument                | Example value          | Description  ");                                                 
	puts("    ------------------------|------------------------|--------------");
	/*The following lines may have 2 extra spaces to account for the escaped quotes. This way it lines up in the command line window.*/
	puts("    -a, --allocation_id     | 1                      | (LONG INTEGER) Allocation that this step is part of.");
	puts("    -c, --cpu_stats         | \"cpu_good\"             | (STRING) Statistics gathered from the CPU for the step. Tracked and given to CSM by job leader.");
	puts("    -e, --exit_status       | 1                      | (INTEGER) Step's exit status. Tracked and given to CSM by job leader.");
	puts("    -E, --err_text          | \"error\"                | (STRING) Step's error text. Tracked and given to CSM by job leader.");
	puts("    -G, --gpu_stats         | \"gpu_s_good\"           | (STRING) Statistics gathered from the GPU for the step. Tracked and given to CSM by job leader.");
	puts("    -i, --io_stats          | \"io_sts_good\"          | (STRING) General input output statistics for the step.");
	puts("    -m, --memory_stats      | \"mem_sts_good\"         | (STRING) Memory statistics for the the step.");
	puts("    -M, --max_memory        | 1                      | (LONG INTEGER) The maximum memory usage of the step.");
	puts("    -n, --omp_thread_limit |  \"omp_thread_limit\"     | (STRING) Max number of omp threads used by the step.");
	puts("    -s, --step_id           | 1                      | (LONG INTEGER) Uniquely identify this step.");
	puts("    -t, --total_u_time      | 0.0                    | (DOUBLE) Relates to the 'us' (aka: user mode) value of %Cpu(s) of the 'top' Linux cmd.");
	puts("    -T, --total_s_time      | 1.5                    | (DOUBLE) Relates to the 'sy' (aka: system mode) value of %Cpu(s) of the 'top' Linux cmd.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("FULL EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_step_end -a 1 -c \"cpu_good\" -e 1 -E \"error\" -G \"gpu_s_good\" -i \"io_sts_good\" -m \"mem_sts_good\" -M 1 -n \"t_num_threads_good\" -s 1 -t 0.0 -T 1.5");
	puts("____________________");
}

/*
* Summary: command line interface for the CSM API 'allocation step end'. 
*           Needed for user managed allocations
* 			Takes in the fields for a csmi_allocation_step_history_t struct as command line parameters, and inserts the data into the CSM database.
*/
int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int return_value = 0;
	int parameterCounter = 0;
	/* getopt_long stores the option index here. */
	int option_index = 0;
    char *arg_test = NULL;
	
	/*Set up test data*/
    API_PARAMETER_INPUT_TYPE input;
	csm_init_struct_ptr(csmi_allocation_step_history_t, input.history);
	
	/*Variables for checking cmd line args*/
	int opt;
	/*check optional args*/
	while((opt = getopt_long(argc, argv, "hv:a:c:e:E:G:i:m:M:n:s:t:T:", long_options, &option_index)) != -1){
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
                break;
			case'a':
				/*allocation_id*/
                csm_optarg_test( "-a, --allocation", optarg, USAGE )
                csm_str_to_int64( input.allocation_id, optarg,  
                                arg_test, "-a, --allocation", USAGE )
				parameterCounter++;
				break;
			case 'c':
                csm_optarg_test( "-c, --cpu_stats", optarg, USAGE )
				input.history->cpu_stats = strdup(optarg);
				parameterCounter++;
				break;
			case 'e':
				/*exit_status*/
                csm_optarg_test( "-e, --exit_status", optarg, USAGE )
                csm_str_to_int32( input.history->exit_status, optarg, arg_test, 
                    "-e, --exit_status", USAGE )
				parameterCounter++;
				break;
			case 'E':
                csm_optarg_test( "-E, --error_message", optarg, USAGE )
				input.history->error_message = strdup(optarg);
				parameterCounter++;
				break;
			case 'G':
                csm_optarg_test( "-G, --gpu_stats", optarg, USAGE )
				input.history->gpu_stats = strdup(optarg);
				parameterCounter++;
				break;
			case 'i':
                csm_optarg_test( "-i, --io_stats", optarg, USAGE )
				input.history->io_stats = strdup(optarg);
				parameterCounter++;
				break;
			case 'm':
                csm_optarg_test( "-m, --memory_stats", optarg, USAGE )
				input.history->memory_stats = strdup(optarg);
				parameterCounter++;
				break;
			case 'M':
                csm_str_to_int64( input.history->max_memory, optarg,
                                arg_test, "-M, --max_memory", USAGE )
				parameterCounter++;
				break;
			case 'n':
				/* total_num_threads */
                csm_optarg_test( "-n, --omp_thread_limit", optarg, USAGE )
				input.history->omp_thread_limit = strdup(optarg);
				parameterCounter++;
				break;
			case 's':
                csm_optarg_test( "-s, --step_id", optarg, USAGE )
                csm_str_to_int64( input.step_id, optarg, arg_test, 
                    "-s, --step_id", USAGE )
				parameterCounter++;
				break;
			case 't':
                csm_optarg_test( "-t, --total_u_time", optarg, USAGE )
				csm_str_to_double( input.history->total_u_time, optarg,
                                arg_test, "-t, --total_u_time", USAGE )
				parameterCounter++;
				break;
			case 'T':
                csm_optarg_test( "-T, --total_s_time", optarg, USAGE )
				csm_str_to_double( input.history->total_s_time, optarg,
                                arg_test, "-T, --total_s_time", USAGE )

				parameterCounter++;
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
	if(parameterCounter != EXPECTED_NUMBER_OF_ARGUMENTS){
		/*We don't have the correct number of arguments passed in. We expecting EXPECTED_NUMBER_OF_ARGUMENTS.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s). Encountered %i parameter(s). Expected %i mandatory parameter(s).", parameterCounter, EXPECTED_NUMBER_OF_ARGUMENTS);
        USAGE();
		return CSMERR_INVALID_PARAM;
	}
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if ( return_value != 0 )
    {
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required "
            "to be able to communicate between library and daemon. Are the daemons running?", 
            return_value);
        csm_free_struct(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}

	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  csmi_allocation_step_history_t contains the following:");
	csmutil_logging(debug, "    step_id:           %"PRId64, input.step_id);
	csmutil_logging(debug, "    allocation_id:     %"PRId64, input.allocation_id);
	csmutil_logging(debug, "    exit_status:       %"PRId32, input.history->exit_status);
	csmutil_logging(debug, "    error_message:     %s",      input.history->error_message);
	csmutil_logging(debug, "    cpu_stats:         %s",      input.history->cpu_stats);
	csmutil_logging(debug, "    total_u_time:      %lf",     input.history->total_u_time);
	csmutil_logging(debug, "    total_s_time:      %lf",     input.history->total_s_time);
	csmutil_logging(debug, "    omp_thread_limit:  %s",      input.history->omp_thread_limit);
	csmutil_logging(debug, "    gpu_stats:         %s",      input.history->gpu_stats);
	csmutil_logging(debug, "    memory_stats:      %s",      input.history->memory_stats);
	csmutil_logging(debug, "    max_memory:        %"PRId64, input.history->max_memory);
	csmutil_logging(debug, "    io_stats:          %s",      input.history->io_stats);		
	
	/*All that just to call the api.*/
	return_value = csm_allocation_step_end(&csm_obj, &input);

	if( return_value != CSMI_SUCCESS )
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
