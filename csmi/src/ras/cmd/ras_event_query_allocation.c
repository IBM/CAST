/*================================================================================

    csmi/src/ras/cmd/ras_event_query_allocation.c

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
#include "csmi/include/csm_api_ras.h"
/*Needed for CSM logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

#include <assert.h>

#define API_PARAMETER_INPUT_TYPE  csm_ras_event_query_allocation_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_event_query_allocation_output_t

///< For use as the usage variable in the input parsers.
#define USAGE  help

struct option longopts[] = {
  {"help",             no_argument,       0, 'h'},
  {"verbose",          required_argument, 0, 'v'},
  {"allocation_id",    required_argument, 0, 'a'}, 
  {0,0,0,0}
};

static void help()
{
	puts("_____CSM_RAS_EVENT_QUERY_ALLOCATION_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_ras_event_query_allocation ARGUMENTS [OPTIONS]");
	puts("  csm_ras_event_query_allocation [-a id] ");
	puts("");
	puts("SUMMARY: Displays all RAS events that occurred during the execution of the supplied allocation.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  >0  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_ras_event_query_allocation expects 1 mandatory argument: ");
	puts("    Argument                  | Example value | Description  ");                                                 
	puts("    --------------------------|---------------|--------------");
	puts("    -a, --allocation_id       | 1             | (LONG INTEGER) Unique identifier for an allocation.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_ras_event_query_allocation -a 1");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

int main(int argc, char *argv[])
{
	/*Helper Variables*/
	int i;
	int opt;
	int indexptr = 0;
	int return_value = 0;
	char *arg_check = NULL; ///< Used in verifying the long arg values.
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*API Variables*/
    API_PARAMETER_INPUT_TYPE input;
    API_PARAMETER_OUTPUT_TYPE *output = NULL;

    csm_init_struct(API_PARAMETER_INPUT_TYPE, input);

	while ((opt = getopt_long(argc, argv, "hv:a:j:J:", longopts, &indexptr)) != -1) {
		switch (opt) {
			case 'h':
                USAGE();
                return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'a':
                csm_optarg_test( "-a, --allocation", optarg, USAGE )
                csm_str_to_int64(input.allocation_id, optarg, arg_check, "-a, --allocation", help)
				break;
			default:
				csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
                return CSMERR_INVALID_PARAM;
		}
	}

    if (input.allocation_id <= 0)
    {
        printf("Allocation id not provided.\n");
        return CSMERR_INVALID_PARAM;
    }

	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
		return return_value;
	}

	return_value = csm_ras_event_query_allocation(&csm_obj, &input, &output);

    switch(return_value)
    {
        case CSMI_SUCCESS:
        { 
            // Grab the allocation struct for convenience.
            csmi_ras_event_action_t **events = output->events;

	    	puts("---");
            printf("Total_Records: %i\n", output->num_events);

            for( i=0; i < output->num_events; i = i + 1)
            {
                printf("Record_%i:\n", i+1);
                printf("  rec_id:               %" PRId64 "\n", events[i]->rec_id);
                printf("  msg_id:               %s\n", events[i]->msg_id);
                printf("  msg_id_seq:           %"PRId32"\n", events[i]->msg_id_seq);
                printf("  time_stamp:           %s\n", events[i]->time_stamp);
                printf("  count:                %" PRId32 "\n", events[i]->count);
                printf("  message:              %s\n", events[i]->message);
                printf("  raw_data:             %s\n", events[i]->raw_data);
                printf("  archive_history_time: %s\n", events[i]->archive_history_time);
            }

	    	puts("...");
            break;
        }

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

	// it's the csmi library's responsibility to free internal space
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
