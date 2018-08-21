/*================================================================================

    csmi/src/wm/cmd/allocation_query_active_all.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

e===============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <inttypes.h>
#include "csmi/include/csm_api_workload_manager.h"
/*Needed for CSM logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"
#include "csmi/src/wm/include/csmi_wm_internal.h"


#include <assert.h>

#define API_PARAMETER_INPUT_TYPE csm_allocation_query_active_all_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_allocation_query_active_all_output_t
#define API_FORMAT_START "num_allocations,allocations{"
#define API_FORMAT_STRING "allocation_id,primary_job_id,begin_time,state,user_name,num_nodes,launch_node_name"
#define API_FORMAT_END   "}"

///< For use as the usage variable in the input parsers.
#define USAGE  help

struct option longopts[] = {
	{"help",    no_argument,       0, 'h'},
	{"verbose", required_argument, 0, 'v'},
    {"format",  required_argument, 0, 'f'},
	{0,0,0,0}
}
;

static void help(){
	puts("_____CSM_ALLOCATION_QUERY_ACTIVE_ALL_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_allocation_query_active_all ARGUMENTS [OPTIONS]");
	puts("  csm_allocation_query_active_all [-h] [-v verbose_level] [-f format_string]");
	puts("");
	puts("SUMMARY: Used to get information about all active allocations.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    Argument     | Example value               | Description  ");
	puts("    -------------|-----------------------------|--------------");
	puts("    -f, --format | allocation_id,compute_nodes | (STRING) Format string detailing the fields to display.");
    puts("                 |                             | Values are comma delimited, matching the contents of the struct.");
    puts("                 |                             | \"%\" Indicates all values should be shown.");
	puts("                 |                             | (default = \"allocation_id,primary_job_id,begin_time,state,user_name,num_nodes,launch_node_name\")");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  >0 if ERROR.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_allocation_query_active_all");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

int main(int argc, char *argv[])
{
	/*Helper Variables*/
    //int j;
	int return_value = 0;
    int opt;
    int indexptr = 0;
    char *format = NULL;
	/*CSM Variables*/
	csm_api_object    *csm_obj = NULL;
	/*API Variables*/
    API_PARAMETER_INPUT_TYPE input;
	API_PARAMETER_OUTPUT_TYPE* output = NULL;

    csm_init_struct(API_PARAMETER_INPUT_TYPE, input);
	
	//new format
	char format_select = 0; 
	char format_all = 0; 
	uint32_t user_selected_field_list_count = 0;
	char** user_selected_field_list = NULL;

    while ((opt = getopt_long(argc, argv, "hf:v:", longopts, &indexptr)) != -1) {
        switch(opt){
			case 'h':
                USAGE();
                return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
            case 'f':
            {    
				if(optarg[0] == '%'){
					//wild card to print everything.
					format_all = 1; 
				}else{
					//format to only print user selected values
					csm_parse_csv( optarg, user_selected_field_list, user_selected_field_list_count,
						char*, csm_str_to_char, NULL, "-n, --format", USAGE );
							
					format_select = 1;
				}
                break;
			}
			default:
				csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
				return CSMERR_INVALID_PARAM;
        }
    }

    /* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if ( return_value != 0 )
    {
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required "
            "to be able to communicate between library and daemon. Are the daemons running?", 
            return_value);
		return return_value;
	}

    return_value = csm_allocation_query_active_all( &csm_obj, &input, &output);

    switch(return_value)
    {
        case CSMI_SUCCESS:
        {
			if(format_all == 1)
			{
				//print everything
				puts("---");
				printf("num_allocations: %i\n", output->num_allocations);
				puts("allocations:");
				int i = 0;
				for(i = 0; i < output->num_allocations; i++){
					printf("  - allocation_id: %" PRId64 "\n", output->allocations[i]->allocation_id);
					printf("    primary_job_id: %" PRId64 "\n", output->allocations[i]->primary_job_id);
					printf("    secondary_job_id: %" PRId32 "\n", output->allocations[i]->secondary_job_id);
					printf("    ssd_file_system_name: %s\n", output->allocations[i]->ssd_file_system_name);
					printf("    launch_node_name: %s\n", output->allocations[i]->launch_node_name);
					printf("    isolated_cores: %" PRId32 "\n", output->allocations[i]->isolated_cores);
					printf("    user_flags: %s\n", output->allocations[i]->user_flags);
					printf("    system_flags: %s\n", output->allocations[i]->system_flags);
					printf("    ssd_min: %" PRId64 "\n", output->allocations[i]->ssd_min);
					printf("    ssd_max: %" PRId64 "\n", output->allocations[i]->ssd_max);
					printf("    num_nodes: %" PRIu32 "\n", output->allocations[i]->num_nodes);
					printf("    num_processors: %" PRIu32 "\n", output->allocations[i]->num_processors);
					printf("    num_gpus: %" PRIu32 "\n", output->allocations[i]->num_gpus);
					printf("    projected_memory: %" PRIu32 "\n", output->allocations[i]->projected_memory);
					printf("    state: %s\n", csm_get_string_from_enum(csmi_state_t, output->allocations[i]->state));
					printf("    type: %s\n", csm_get_string_from_enum(csmi_allocation_type_t, output->allocations[i]->type));
					printf("    job_type: %s\n", csm_get_string_from_enum(csmi_job_type_t, output->allocations[i]->job_type));
					printf("    user_name: %s\n", output->allocations[i]->user_name);
					printf("    user_id: %" PRIu32 "\n", output->allocations[i]->user_id);
					printf("    user_group_id: %" PRIu32 "\n", output->allocations[i]->user_group_id);
					//=================
					//FIELD MISSING FIX
					printf("    user_group_name: \n");
					//=================
					printf("    user_script: %s\n", output->allocations[i]->user_script);
					printf("    begin_time: %s\n", output->allocations[i]->begin_time);
					printf("    account: %s\n", output->allocations[i]->account);
					printf("    comment: %s\n", output->allocations[i]->comment);
					printf("    job_name: %s\n", output->allocations[i]->job_name);
					printf("    job_submit_time: %s\n", output->allocations[i]->job_submit_time);
					printf("    queue: %s\n", output->allocations[i]->queue);
					printf("    requeue: %s\n", output->allocations[i]->requeue);
					printf("    time_limit: %" PRId64 "\n", output->allocations[i]->time_limit);
					printf("    wc_key: %s\n", output->allocations[i]->wc_key);
					//special
					printf("    shared: %c\n", csm_print_bool_custom(output->allocations[i]->shared, 't','f'));
					//compute nodes
					puts("    compute_nodes: ");
					int n = 0;
					for(n = 0; n < output->allocations[i]->num_nodes; n++){
						printf("      - %s\n", output->allocations[i]->compute_nodes[n]);
					}
					//no need to print history - ACTIVE ALL - 
				}
				puts("...");
			}else if(format_select == 1){
				puts("---");
				printf("num_allocations: %i\n", output->num_allocations);
				puts("allocations:");
				int i = 0;
				for(i = 0; i < output->num_allocations; i++){
					//loop through the -f format printer list
					int j = 0;
					for(j = 0; j < user_selected_field_list_count; j++)
					{
						// To keep John's original format printing. 
						// I stick with his "allocation" as a list
						//the following if / else will determin if this is the first item of a new record
						// if it is, then print/indicate that it is a new element in YAML
						if(j == 0)
						{
							printf("  - ");
						}else{
							printf("    ");
						}
						//============================================
						//compare the iteration to a valid field
						//if a match is found
						//then print the data
						if(strncmp(user_selected_field_list[j], "allocation_id", 13) == 0){
							printf("allocation_id: %" PRId64 "\n", output->allocations[i]->allocation_id);
						}else if(strncmp(user_selected_field_list[j], "primary_job_id", 14) == 0){
							printf("primary_job_id: %" PRId64 "\n", output->allocations[i]->primary_job_id);
						}else if(strncmp(user_selected_field_list[j], "secondary_job_id", 16) == 0){
							printf("secondary_job_id: %" PRId32 "\n", output->allocations[i]->secondary_job_id);
						}else if(strncmp(user_selected_field_list[j], "ssd_file_system_name", 20) == 0){
							printf("ssd_file_system_name: %s\n", output->allocations[i]->ssd_file_system_name);
						}else if(strncmp(user_selected_field_list[j], "launch_node_name", 16) == 0){
							printf("launch_node_name: %s\n", output->allocations[i]->launch_node_name);
						}else if(strncmp(user_selected_field_list[j], "isolated_cores", 14) == 0){
							printf("isolated_cores: %"PRId32 "\n", output->allocations[i]->isolated_cores);
						}else if(strncmp(user_selected_field_list[j], "user_flags", 10) == 0){
							printf("user_flags: %s\n", output->allocations[i]->user_flags);
						}else if(strncmp(user_selected_field_list[j], "system_flags", 12) == 0){
							printf("system_flags: %s\n", output->allocations[i]->system_flags);
						}else if(strncmp(user_selected_field_list[j], "ssd_min", 7) == 0){
							printf("ssd_min: %" PRId64 "\n", output->allocations[i]->ssd_min);
						}else if(strncmp(user_selected_field_list[j], "ssd_max", 7) == 0){
							printf("ssd_max: %" PRId64 "\n", output->allocations[i]->ssd_max);
						}else if(strncmp(user_selected_field_list[j], "num_nodes", 9) == 0){
							printf("num_nodes: %" PRIu32 "\n", output->allocations[i]->num_nodes);
						}else if(strncmp(user_selected_field_list[j], "num_processors", 14) == 0){
							printf("num_processors: %" PRIu32 "\n", output->allocations[i]->num_processors);
						}else if(strncmp(user_selected_field_list[j], "num_gpus", 8) == 0){
							printf("num_gpus: %" PRIu32 "\n", output->allocations[i]->num_gpus);
						}else if(strncmp(user_selected_field_list[j], "projected_memory", 16) == 0){
							printf("projected_memory: %" PRIu32 "\n", output->allocations[i]->projected_memory);
						}else if(strncmp(user_selected_field_list[j], "state", 5) == 0){
							printf("state: %s\n", csm_get_string_from_enum(csmi_state_t, output->allocations[i]->state));
						}else if(strncmp(user_selected_field_list[j], "type", 4) == 0){
							printf("type: %s\n", csm_get_string_from_enum(csmi_allocation_type_t, output->allocations[i]->type));
						}else if(strncmp(user_selected_field_list[j], "job_type", 8) == 0){
							printf("job_type: %s\n", csm_get_string_from_enum(csmi_job_type_t, output->allocations[i]->job_type));
						}else if(strncmp(user_selected_field_list[j], "user_name", 9) == 0){
							printf("user_name: %s\n", output->allocations[i]->user_name);
						}else if(strncmp(user_selected_field_list[j], "user_id", 7) == 0){
							printf("user_id: %" PRIu32 "\n", output->allocations[i]->user_id);
						}else if(strncmp(user_selected_field_list[j], "user_group_id", 13) == 0){
							printf("user_group_id: %" PRIu32 "\n", output->allocations[i]->user_group_id);
						}else if(strncmp(user_selected_field_list[j], "user_group_name", 15) == 0){
							//=================
							//FIELD MISSING FIX
							printf("user_group_name: \n");
							//=================
						}else if(strncmp(user_selected_field_list[j], "user_script", 11) == 0){
							printf("user_script: %s\n", output->allocations[i]->user_script);
						}else if(strncmp(user_selected_field_list[j], "begin_time", 10) == 0){
							printf("begin_time: %s\n", output->allocations[i]->begin_time);
						}else if(strncmp(user_selected_field_list[j], "account", 7) == 0){
							printf("account: %s\n", output->allocations[i]->account);
						}else if(strncmp(user_selected_field_list[j], "comment", 7) == 0){
							printf("comment: %s\n", output->allocations[i]->comment);
						}else if(strncmp(user_selected_field_list[j], "job_name", 8) == 0){
							printf("job_name: %s\n", output->allocations[i]->job_name);
						}else if(strncmp(user_selected_field_list[j], "job_submit_time", 15) == 0){
							printf("job_submit_time: %s\n", output->allocations[i]->job_submit_time);
						}else if(strncmp(user_selected_field_list[j], "queue", 5) == 0){
							printf("queue: %s\n", output->allocations[i]->queue);
						}else if(strncmp(user_selected_field_list[j], "requeue", 7) == 0){
							printf("requeue: %s\n", output->allocations[i]->requeue);
						}else if(strncmp(user_selected_field_list[j], "time_limit", 10) == 0){
							printf("time_limit: %" PRId64 "\n", output->allocations[i]->time_limit);
						}else if(strncmp(user_selected_field_list[j], "wc_key", 6) == 0){
							printf("wc_key: %s\n", output->allocations[i]->wc_key);
						}else if(strncmp(user_selected_field_list[j], "shared", 6) == 0){
							printf("shared: %c\n", csm_print_bool_custom(output->allocations[i]->shared, 't','f'));
						}else if(strncmp(user_selected_field_list[j], "compute_nodes", 13) == 0){
							puts("compute_nodes: ");
							int n = 0;
							for(n = 0; n < output->allocations[i]->num_nodes; n++){
								printf("      - %s\n", output->allocations[i]->compute_nodes[n]);
							}
						}else{
							//Print a YAML error incase the user had a typo in their input selection. 
							printf("# ERROR: \"%s\" has no match to format input.\n", user_selected_field_list[j]);
						}
					}// end of for loop - user_selected_field_list_count
				}//end of for loop -  printing each allocation 
				puts("...");
			}else{
				//default prints 
				//allocation_id,primary_job_id,begin_time,state,user_name,num_nodes,launch_node_name
				//Labs requested not printing out all fields by default. 
				puts("---");
				printf("num_allocations: %i\n", output->num_allocations);
				puts("allocations:");
				int i = 0;
				for(i = 0; i < output->num_allocations; i++){
					printf("  - allocation_id: %" PRId64 "\n", output->allocations[i]->allocation_id);
					printf("    primary_job_id: %" PRId64 "\n", output->allocations[i]->primary_job_id);
					printf("    begin_time: %s\n", output->allocations[i]->begin_time);
					printf("    state: %s\n", csm_get_string_from_enum(csmi_state_t, output->allocations[i]->state));
					printf("    user_name: %s\n", output->allocations[i]->user_name);
					printf("    num_nodes: %" PRIu32 "\n", output->allocations[i]->num_nodes);
					printf("    launch_node_name: %s\n", output->allocations[i]->launch_node_name);
				}
				puts("...");
			}
            
            break;
        }
        case CSMI_NO_RESULTS:
            puts("---");
            puts("num_allocations: 0");
            puts("# No matching records found.");
            puts("...");
            break;
        default:
	    	printf("%s FAILED: errcode: %d errmsg: %s\n", argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
	}
	
    if ( format ) free(format);

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
