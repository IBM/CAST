/*================================================================================

    csmi/src/inv/cmd/node_attributes_query_details.c

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
#include "csmi/include/csm_api_inventory.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

/* Define API types to make life easier. */
#define API_PARAMETER_INPUT_TYPE csm_node_attributes_query_details_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_attributes_query_details_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_NODE_ATTRIBUTES_QUERY_DETAILS_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_node_attributes_query_details ARGUMENTS [OPTIONS]");
	puts("  csm_node_attributes_query_details -n node_name [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_node' and related sister tables to get everything there is to know about a node in the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_node_attributes_query_details expects 1 mandatory argument");
	puts("    Argument        | Example value | Description  ");                                                 
	puts("    ----------------|---------------|--------------");
	puts("    -n, --node_name | \"node01\"      | (STRING) This is the node name to query. The node name is a unique identification for a node.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_node_attributes_query_details -n \"node01\"");
	puts("");
	puts("OUTPUT OF THIS COMMAND IS DISPLAYED IN THE YAML FORMAT.");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",       no_argument,       0, 'h'},
	{"verbose",    required_argument, 0, 'v'},
	//api arguments
	{"node_name",  required_argument, 0, 'n'},
	{0,0,0,0}
};

/*
* Summary: Simple command line interface for the CSM API 'node attributes query details'. 
* 			Takes in the name of a single node via command line parameters, queries the 
*			database for this node, and prints all the attributes of that node.
*/
int main(int argc, char *argv[])
{	
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 1;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    //char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	int i = 0;
	
	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "hv:n:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case 'n':
            {
				/*checks if NULL*/
                csm_optarg_test( "-n, --node_name", optarg, USAGE );
				input->node_name = strdup(optarg);
				
				/* Increment requiredParameterCounter so later we can check if arguments were correctly set before calling API. */
				requiredParameterCounter++;
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
	if(return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return return_value;
	}
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:    %p", input);
	csmutil_logging(debug, "  address of input:  %p", &input);
	csmutil_logging(debug, "  input contains the following:");
	csmutil_logging(debug, "    node_name:       %s", input->node_name);
	
    /* Call the C API. */
	return_value = csm_node_attributes_query_details(&csm_obj, input, &output);
	/* Use CSM API free to release arguments. We no longer need them. */
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    switch(return_value)
    {
        case CSMI_SUCCESS:
			puts("---");
			printf("Total_Records: %i\n", output->result_count);
	    	printf("Record_%i:\n", output->result_count);
			printf("  node_name:                    %s\n", output->result[0]->node->node_name);
			printf("  node_collection_time:         %s\n", output->result[0]->node->collection_time);
			printf("  node_update_time:             %s\n", output->result[0]->node->update_time);
			printf("  node_comment:                 \"%s\"\n", output->result[0]->node->comment);
			printf("  node_discovered_cores:        %i\n", output->result[0]->node->discovered_cores);
			printf("  node_discovered_dimms:        %i\n", output->result[0]->node->discovered_dimms);
			printf("  node_discovered_gpus:         %i\n", output->result[0]->node->discovered_gpus);
			printf("  node_discovered_hcas:         %i\n", output->result[0]->node->discovered_hcas);
			printf("  node_discovered_sockets:      %i\n", output->result[0]->node->discovered_sockets);
			printf("  node_discovered_ssds:         %i\n", output->result[0]->node->discovered_ssds);
			printf("  node_feature_1:               %s\n", output->result[0]->node->feature_1);
			printf("  node_feature_2:               %s\n", output->result[0]->node->feature_2);
			printf("  node_feature_3:               %s\n", output->result[0]->node->feature_3);
			printf("  node_feature_4:               %s\n", output->result[0]->node->feature_4);
			printf("  node_hard_power_cap:          %i\n", output->result[0]->node->hard_power_cap);
			printf("  node_installed_memory:        %" PRId64 "\n", output->result[0]->node->installed_memory);
			printf("  node_installed_swap:          %" PRId64 "\n", output->result[0]->node->installed_swap);
			printf("  node_kernel_release:          %s\n", output->result[0]->node->kernel_release);
			printf("  node_kernel_version:          %s\n", output->result[0]->node->kernel_version);
			printf("  node_machine_model:           %s\n", output->result[0]->node->machine_model);
			printf("  node_os_image_name:           %s\n", output->result[0]->node->os_image_name);
			printf("  node_os_image_uuid:           %s\n", output->result[0]->node->os_image_uuid);
			printf("  node_physical_frame_location: %s\n", output->result[0]->node->physical_frame_location);
			printf("  node_physical_u_location:     %s\n", output->result[0]->node->physical_u_location);
			printf("  node_primary_agg:             %s\n", output->result[0]->node->primary_agg);
			printf("  node_secondary_agg:           %s\n", output->result[0]->node->secondary_agg);
			printf("  node_serial_number:           %s\n", output->result[0]->node->serial_number);
	    	printf("  node_state:                   %s\n", csm_get_string_from_enum(csmi_node_state_t, output->result[0]->node->state));
			printf("  node_type:                    %s\n", csm_get_string_from_enum(csmi_node_type_t, output->result[0]->node->type));
			printf("  dimms_count:                  %i\n", output->result[0]->dimms_count);
			if(output->result[0]->dimms_count > 0)
			{
				printf("  dimms:\n");
				for(i = 0; i < output->result[0]->dimms_count; i++)
				{
					printf("    - serial_number:     %s\n", output->result[0]->dimms[i]->serial_number);
					printf("      physical_location: %s\n", output->result[0]->dimms[i]->physical_location);
					printf("      size:              %" PRId32 "\n", output->result[0]->dimms[i]->size);
				}
			}
			printf("  gpus_count:                   %i\n", output->result[0]->gpus_count);
			if(output->result[0]->gpus_count > 0)
			{
				printf("  gpus:\n");
				for(i = 0; i < output->result[0]->gpus_count; i++)
				{
					printf("    - gpu_id:                %" PRId32 "\n", output->result[0]->gpus[i]->gpu_id);
					printf("      device_name:           %s\n", output->result[0]->gpus[i]->device_name);
					printf("      hbm_memory:            %" PRId64 "\n", output->result[0]->gpus[i]->hbm_memory);
					printf("      inforom_image_version: %s\n", output->result[0]->gpus[i]->inforom_image_version);
					printf("      pci_bus_id:            %s\n", output->result[0]->gpus[i]->pci_bus_id);
					printf("      serial_number:         %s\n", output->result[0]->gpus[i]->serial_number);
					printf("      uuid:                  %s\n", output->result[0]->gpus[i]->uuid);
					printf("      vbios:                 %s\n", output->result[0]->gpus[i]->vbios);
				}
			}
			printf("  hcas_count:                   %i\n", output->result[0]->hcas_count);
			if(output->result[0]->hcas_count > 0)
			{
				printf("  hcas:\n");
				for(i = 0; i < output->result[0]->hcas_count; i++)
				{
					printf("    - serial_number: %s\n", output->result[0]->hcas[i]->serial_number);
					printf("      board_id:      %s\n", output->result[0]->hcas[i]->board_id);
					printf("      device_name:   %s\n", output->result[0]->hcas[i]->device_name);
					printf("      fw_ver:        %s\n", output->result[0]->hcas[i]->fw_ver);
					printf("      guid:          %s\n", output->result[0]->hcas[i]->guid);
					printf("      hw_rev:        %s\n", output->result[0]->hcas[i]->hw_rev);
					printf("      part_number:   %s\n", output->result[0]->hcas[i]->part_number);
					printf("      pci_bus_id:    %s\n", output->result[0]->hcas[i]->pci_bus_id);
				}
			}

			printf("  processors_count:             %i\n", output->result[0]->processors_count);
			if(output->result[0]->processors_count > 0)
			{
				printf("  processors:\n");
				for(i = 0; i < output->result[0]->processors_count; i++)
				{
					printf("    - serial_number:     %s\n", output->result[0]->processors[i]->serial_number);
					printf("      discovered_cores:  %" PRId32 "\n", output->result[0]->processors[i]->discovered_cores);
					printf("      physical_location: %s\n", output->result[0]->processors[i]->physical_location);
				}
			}

			printf("  ssds_count:                   %i\n", output->result[0]->ssds_count);
			if(output->result[0]->ssds_count > 0)
			{
				printf("  ssds:\n");
				for(i = 0; i < output->result[0]->ssds_count; i++)
				{
					printf("    - serial_number:                 %s\n", output->result[0]->ssds[i]->serial_number);
					printf("      update_time:                   %s\n", output->result[0]->ssds[i]->update_time);
					printf("      device_name:                   %s\n", output->result[0]->ssds[i]->device_name);
					printf("      fw_ver:                        %s\n", output->result[0]->ssds[i]->fw_ver);
					printf("      pci_bus_id:                    %s\n", output->result[0]->ssds[i]->pci_bus_id);
					printf("      size:                          %" PRId64 "\n", output->result[0]->ssds[i]->size);
					printf("      wear_lifespan_used:            %f\n", output->result[0]->ssds[i]->wear_lifespan_used);
					printf("      wear_total_bytes_read:         %" PRId64 "\n", output->result[0]->ssds[i]->wear_total_bytes_read);
					printf("      wear_total_bytes_written:      %" PRId64 "\n", output->result[0]->ssds[i]->wear_total_bytes_written);
					printf("      wear_percent_spares_remaining: %f\n", output->result[0]->ssds[i]->wear_percent_spares_remaining);
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
            printf("%s FAILED: errcode: %d errmsg: %s\n", argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }
		
    /* Call internal CSM API clean up. */
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
