/*================================================================================

    csmi/src/wm/cmd/allocation_step_query.c

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

#define API_PARAMETER_INPUT_TYPE  csm_allocation_step_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_allocation_step_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct(API_PARAMETER_INPUT_TYPE, input); help

void help() {
	puts("_____CSM_ALLOCATION_STEP_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_step_query ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_step_query -a allocation_id [-s step_id] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to gather information about a step from the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_allocation_step_query expects 1 mandatory argument");
	puts("    Argument            | Example value | Description  ");                                                 
	puts("    --------------------|---------------|--------------");
	puts("    -a, --allocation_id | 1             | (INT 64) Primary job id (for lsf this will be the lsf job id). API will ignore values less than 1.");
	puts("                        |               | ");
	puts("  OPTIONAL:");
	puts("    csm_allocation_step_query can have 1 optional argument");
	puts("    Argument      | Example value | Description  ");                                                 
	puts("    --------------|---------------|--------------");
	puts("    -s, --step_id | 1             | (INT 64) Primary job id (for lsf this will be the lsf job id). API will ignore values less than 1.");
	puts("                  |               | ");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_step_query -a 1");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",            no_argument,       0, 'h'},
	{"verbose",         required_argument, 0, 'v'},
	//arguments
	{"allocation_id",   required_argument, 0, 'a'},
	{"step_id",         required_argument, 0, 's'},
	{0, 0, 0, 0}
};

/*
* Summary: Simple command line interface for the CSM API 'allocation step query'. 
*			Works as interface between user managed allocations and the CSM DB.
* 			Takes in the arguments via command line parameters, and queries the data in the CSM database.
*/
int main(int argc, char *argv[])
{
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*Helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 1;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*For for loops*/
	int32_t i = 0;

    char *arg_check = NULL; ///< Used in verifying the long arg values.
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE input;
    API_PARAMETER_OUTPUT_TYPE *output = NULL;
	csm_init_struct(API_PARAMETER_INPUT_TYPE, input);
	
	/*check optional args*/
	while((opt = getopt_long(argc, argv, "hv:a:s:",longopts, &indexptr)) != -1){
		switch(opt){
			case 'h':      
                USAGE();
				return CSMI_HELP;
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':      
				/*Convert the string argument into a uint64_t*/
                csm_optarg_test( "-a, --allocation_id", optarg, USAGE )
                csm_str_to_int64( input.allocation_id, optarg, arg_check, 
                                "-a, --allocation_id", USAGE )
				requiredParameterCounter++;
				break;
			case 's':
				/*Convert the string argument into a uint64_t*/
                csm_optarg_test( "-s, --step_id", optarg, USAGE )
                csm_str_to_int64( input.step_id, optarg, arg_check, 
                                "-s, --step_id", USAGE )
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
            optionalParameterCounter < MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS){
		/*We don't have the correct number of needed arguments passed in.*/
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  Missing operand(s).");
		csmutil_logging(error, "    Encountered %i required parameter(s). Expected %i required parameter(s).", requiredParameterCounter, NUMBER_OF_REQUIRED_ARGUMENTS);
		csmutil_logging(error, "    Encountered %i optional parameter(s). Expected at least %i optional parameter(s).", optionalParameterCounter, MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS);
        USAGE();
		return CSMERR_INVALID_PARAM;
	}
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
	    csm_free_struct(csm_allocation_step_query_input_t, input); 
		return return_value;
	}
	
	/*All that just to call the api.*/
	return_value = csm_allocation_step_query(&csm_obj, &input, &output);
	csm_free_struct(API_PARAMETER_INPUT_TYPE, input);

    switch(return_value)
    {
        case CSMI_SUCCESS:
        
	        csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	        csmutil_logging(debug, "  csm_allocation_step_query has completed successfully!");

	        /*Print out the output of the query to stdout in YAML format*/
	        puts("---");
	        printf("Total_Records: %u\n", output->num_steps);
	        for(i = 0; i < output->num_steps; i++){
                csmi_allocation_step_t *step = output->steps[i];
	        	printf("Record_%u:\n", i+1);
                printf("  allocation_id:        %"PRId64"\n", step->allocation_id);
                printf("  step_id:              %"PRId64"\n", step->step_id);
                printf("  begin_time:           %s\n",        step->begin_time);
                printf("  status:               %s\n", csm_get_string_from_enum(csmi_step_status_t, step->status));
                printf("  executable:           %s\n",        step->executable);
                printf("  working_directory:    %s\n",        step->working_directory);
                printf("  argument:             %s\n",        step->argument);
                printf("  environment_variable: %s\n",        step->environment_variable);
                printf("  num_nodes:            %"PRId32"\n", step->num_nodes);
                printf("  num_processors:       %"PRId32"\n", step->num_processors);
                printf("  num_gpus:             %"PRId32"\n", step->num_gpus);
                printf("  projected_memory:     %"PRId32"\n", step->projected_memory);
                printf("  num_tasks:            %"PRId32"\n", step->num_tasks);
                printf("  user_flags:           %s\n",        step->user_flags);

                if ( step->history )
                {
                    csmi_allocation_step_history_t* history = step->history;
                    printf("  history: \n" );
	        	    printf("    archive_history_time: %s\n", history->archive_history_time);
	        	    printf("    cpu_stats:            %s\n", history->cpu_stats);
	        	    printf("    end_time:             %s\n", history->end_time);
	        	    printf("    error_message:        %s\n", history->error_message);
	        	    printf("    exit_status:          %"PRId32"\n", history->exit_status);
	        	    printf("    gpu_stats:            %s\n", history->gpu_stats);
	        	    printf("    io_stats:             %s\n", history->io_stats);
	        	    printf("    max_memory:           %"PRId64"\n", history->max_memory);
	        	    printf("    memory_stats:         %s\n", history->memory_stats);
	        	    printf("    omp_thread_limit:     %s\n", history->omp_thread_limit);
	        	    printf("    total_s_time:         %f\n", history->total_s_time);
	        	    printf("    total_u_time:         %f\n", history->total_u_time);
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
