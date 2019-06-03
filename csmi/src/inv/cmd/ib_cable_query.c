/*================================================================================

    csmi/src/inv/cmd/ib_cable_query.c

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
#define API_PARAMETER_INPUT_TYPE csm_ib_cable_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ib_cable_query_output_t

///< For use as the usage variable in the input parsers.
#define USAGE csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input); help

void help(){
	puts("_____CSM_IB_CABLE_QUERY_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_ib_cable_query ARGUMENTS [OPTIONS]");
	puts("  csm_ib_cable_query [-c comments] [-g guids] [-i identifiers] [-L lengths] [-n names] [-p part_numbers] [-P ports] [-r revisions] [-s serial_numbers] [-S severities] [-t types] [-w widths] [-l limit] [-o offset] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used to query the 'csm_ib_cable' table of the CSM database.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  OPTIONAL:");
	puts("    csm_ib_cable_query has 12 optional arguments used to filter query results.");
	puts("    Argument             | Example value                | Description  ");                                                 
	puts("    ---------------------|------------------------------|--------------");
	puts("    -c, --comments       | \"comment,%nick%\"             | (STRING) Filter results of the database query to only include cables containing comments in this csv list.");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("    -g, --guids          | \"248a:0703:006d:40f0,        | (STRING) Filter results of the database query to only include cables containing guids in this csv list. ");
	puts("                         | ec0d:9a03:00d3:05b4\"         | This field will search both 'guid_s1' and 'guid_s2' fields in 'csm_ib_cable' table. ");
	puts("    -i, --identifiers    | \"QSFP+,Unknown\"              | (STRING) Filter results of the database query to only include cables containing dentifiers in this csv list. ");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("    -L, --lengths        | \"2 m,%3%\"                    | (STRING) Filter results of the database query to only include cables containing lengths in this csv list. ");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("    -n, --names          | \"%248a0703006d40f0_14%\"      | (STRING) Filter results of the database query to only include cables containing names in this csv list.");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("                         |                              |  name is the (Id) of link object in UFM. Based on link sorce and destination. ");
	puts("    -p, --part_numbers   | \"00WT050,%XXYYZZ\"            | (STRING) Filter results of the database query to only include cables containing part_numbers in this csv list.");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("    -P, --ports          | \"14,%2%\"                     | (STRING) Filter results of the database query to only include cables containing ports in this csv list.");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("                         |                              |  API will search both 'port_s1' and 'port_s2' fields in 'csm_ib_cable' table.");
	puts("    -r, --revisions      | \"A1,B%\"                      | (STRING) Filter results of the database query to only include cables containing revisions in this csv list.");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("    -s, --serial_numbers | \"abc123,xyz789\"              | (STRING) This is a csv field of serial numbers to query. Filter results to only include records that have a matching serial numbers. ");
	puts("                         |                              | The serial number is a unique identification for an ib cable. ");
	puts("    -S, --severities     | \"Info\"                       | (STRING) Filter results of the database query to only include cables containing severities in this csv list.");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("    -t, --types          | \"Copper cable- unequalized,  | (STRING) This is a csv field of types to query. Filter results to only include records that have a matching types. ");
	puts("                         | %VCSEL%\"                     | This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%'");
	puts("    -w, --widths         | \"IB_4x,%8x%\"                 | (STRING) This is a csv field of widths to query. Filter results to only include records that have a matching widths.");
	puts("                         |                              |  This field uses psql search of 'LIKE'. So add '%' to get partial matches. '%value%' ");
	puts("                         |                              |  The width of the cable - physical state of IB port (Optional Values: IB_1x ,IB_4x, IB_8x, IB_12x");
	puts("                         |                              | ");
	puts("  FILTERS:");
	puts("    csm_ib_cable_query can have 2 optional filters.");
	puts("    Argument     | Example value | Description  ");                                                 
	puts("    -------------|---------------|--------------");
	puts("    -l, --limit  | 10            | (INTEGER) SQL 'LIMIT' numeric value.");
    puts("    -o, --offset | 1             | (INTEGER) SQL 'OFFSET' numeric value.");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_ib_cable_query -s \"abc123\"");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",           no_argument,       0, 'h'},
	{"verbose",        required_argument, 0, 'v'},
	//api arguments
	{"comments",       required_argument, 0, 'c'},
	{"guids",          required_argument, 0, 'g'},
	{"identifiers",    required_argument, 0, 'i'},
	{"lengths",        required_argument, 0, 'L'},
	{"names",          required_argument, 0, 'n'},
	{"part_numbers",   required_argument, 0, 'p'},
	{"ports",          required_argument, 0, 'P'},
	{"revisions",      required_argument, 0, 'r'},
	{"serial_numbers", required_argument, 0, 's'},
	{"severities",     required_argument, 0, 'S'},
	{"types",          required_argument, 0, 't'},
	{"widths",         required_argument, 0, 'w'},
	//filters
	{"limit",          required_argument, 0, 'l'},
	{"offset",         required_argument, 0, 'o'},
	{0,0,0,0}
};

/*
* Summary: Simple command line interface for the CSM API 'csm_ib_cable_query'.
*/
int main(int argc, char *argv[])
{	
	/*CSM Variables*/
	csm_api_object *csm_obj = NULL;
	/*helper Variables*/
	int return_value = 0;
	int requiredParameterCounter = 0;
	int optionalParameterCounter = 0;
	const int NUMBER_OF_REQUIRED_ARGUMENTS = 0;
	const int MINIMUM_NUMBER_OF_OPTIONAL_ARGUMENTS = 0;
	/*Variables for checking cmd line args*/
	int opt;
    char *arg_check = NULL; ///< Used in verifying the long arg values.

	/* getopt_long stores the option index here. */
	int indexptr = 0;
	/*i var for 'for loops'*/
	uint32_t i = 0;

	/*Set up data to call API*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	API_PARAMETER_OUTPUT_TYPE* output = NULL;
	
	/*check optional args*/
	while ((opt = getopt_long(argc, argv, "c:g:hi:l:L:n:o:p:P:r:s:S:t:v:w:", longopts, &indexptr)) != -1) {
		switch(opt){
			case 'c':
			{
                csm_optarg_test( "-c, --comments", optarg, USAGE );
                csm_parse_csv( optarg, input->comments, input->comments_count,
                            char*, csm_str_to_char, NULL, "-c, --comments", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'g':
			{
                csm_optarg_test( "-g, --guids", optarg, USAGE );
                csm_parse_csv( optarg, input->guids, input->guids_count,
                            char*, csm_str_to_char, NULL, "-g, --guids", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'i':
			{
                csm_optarg_test( "-i, --identifiers", optarg, USAGE );
                csm_parse_csv( optarg, input->identifiers, input->identifiers_count,
                            char*, csm_str_to_char, NULL, "-i, --identifiers", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'l':
				csm_optarg_test( "-l, --limit", optarg, USAGE );
                csm_str_to_int32( input->limit, optarg, arg_check, "-l, --limit", USAGE );
                break;
            case 'L':
			{
                csm_optarg_test( "-L, --lengths", optarg, USAGE );
                csm_parse_csv( optarg, input->lengths, input->lengths_count,
                            char*, csm_str_to_char, NULL, "-L, --lengths", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'n':
			{
                csm_optarg_test( "-n, --names", optarg, USAGE );
                csm_parse_csv( optarg, input->names, input->names_count,
                            char*, csm_str_to_char, NULL, "-n, --names", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'o':
                csm_optarg_test( "-o, --offset", optarg, USAGE );
                csm_str_to_int32( input->offset, optarg, arg_check, "-o, --offset", USAGE );
				break;
			case 'p':
			{
                csm_optarg_test( "-p, --part_numbers", optarg, USAGE );
                csm_parse_csv( optarg, input->part_numbers, input->part_numbers_count,
                            char*, csm_str_to_char, NULL, "-p, --part_numbers", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'P':
			{
                csm_optarg_test( "-P, --ports", optarg, USAGE );
                csm_parse_csv( optarg, input->ports, input->ports_count,
                            char*, csm_str_to_char, NULL, "-P, --ports", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'r':
			{
                csm_optarg_test( "-r, --revisions", optarg, USAGE );
                csm_parse_csv( optarg, input->revisions, input->revisions_count,
                            char*, csm_str_to_char, NULL, "-r, --revisions", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 's':
			{
                csm_optarg_test( "-s, --serial_numbers", optarg, USAGE );
                csm_parse_csv( optarg, input->serial_numbers, input->serial_numbers_count,
                            char*, csm_str_to_char, NULL, "-s, --serial_numbers", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'S':
			{
                csm_optarg_test( "-S, --severities", optarg, USAGE );
                csm_parse_csv( optarg, input->severities, input->severities_count,
                            char*, csm_str_to_char, NULL, "-S, --severities", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 't':
			{
                csm_optarg_test( "-t, --types", optarg, USAGE );
                csm_parse_csv( optarg, input->types, input->types_count,
                            char*, csm_str_to_char, NULL, "-t, --types", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'w':
			{
                csm_optarg_test( "-w, --widths", optarg, USAGE );
                csm_parse_csv( optarg, input->widths, input->widths_count,
                            char*, csm_str_to_char, NULL, "-w, --widths", USAGE );
				/* Increment optionalParameterCounter so later we can check if arguments were correctly set before calling API. */
				optionalParameterCounter++;
				break;
			}
			case 'v':
				/*Error check to make sure 'verbose' field is valid.*/
                csm_set_verbosity( optarg, USAGE )
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
	
	//This will print out the contents of the struct that we will pass to the api
	csmutil_logging(debug, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(debug, "  Preparing to call the CSM API...");
	csmutil_logging(debug, "  value of input:         %p", input);
	csmutil_logging(debug, "  address of input:       %p", &input);
	csmutil_logging(debug, "  API_PARAMETER_INPUT_TYPE contains the following:");
	csmutil_logging(debug, "    comments_count:       %i", input->comments_count);
	csmutil_logging(debug, "    comments:             %p", input->comments);
	for(i = 0; i < input->comments_count; i++){
		csmutil_logging(debug, "      comments[%i]: %s", i, input->comments[i]);
	}
	csmutil_logging(debug, "    guids_count:          %i", input->guids_count);
	csmutil_logging(debug, "    guids:                %p", input->guids);
	for(i = 0; i < input->guids_count; i++){
		csmutil_logging(debug, "      guids[%i]: %s", i, input->guids[i]);
	}
	csmutil_logging(debug, "    identifiers_count:    %i", input->identifiers_count);
	csmutil_logging(debug, "    identifiers:          %p", input->identifiers);
	for(i = 0; i < input->identifiers_count; i++){
		csmutil_logging(debug, "      identifiers[%i]: %s", i, input->identifiers[i]);
	}
	csmutil_logging(debug, "    lengths_count:        %i", input->lengths_count);
	csmutil_logging(debug, "    lengths:              %p", input->lengths);
	for(i = 0; i < input->lengths_count; i++){
		csmutil_logging(debug, "      lengths[%i]: %s", i, input->lengths[i]);
	}
	csmutil_logging(debug, "    names_count:          %i", input->names_count);
	csmutil_logging(debug, "    names:                %p", input->names);
	for(i = 0; i < input->names_count; i++){
		csmutil_logging(debug, "      names[%i]: %s", i, input->names[i]);
	}
	csmutil_logging(debug, "    part_numbers_count:   %i", input->part_numbers_count);
	csmutil_logging(debug, "    part_numbers:         %p", input->part_numbers);
	for(i = 0; i < input->part_numbers_count; i++){
		csmutil_logging(debug, "      part_numbers[%i]: %s", i, input->part_numbers[i]);
	}
	csmutil_logging(debug, "    limit:                %i", input->limit);
	csmutil_logging(debug, "    offset:               %i", input->offset);
	csmutil_logging(debug, "    serial_numbers_count: %i", input->serial_numbers_count);
	csmutil_logging(debug, "    serial_numbers:       %p", input->serial_numbers);
	for(i = 0; i < input->serial_numbers_count; i++){
		csmutil_logging(debug, "      serial_numbers[%i]: %s", i, input->serial_numbers[i]);
	}
	csmutil_logging(debug, "  value of output:        %p", output);
	csmutil_logging(debug, "  address of output:      %p", &output);
	
	/* Call the actual CSM API */
	return_value = csm_ib_cable_query(&csm_obj, input, &output);
	//Use CSM API free to release arguments. We no longer need them.
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
    switch ( return_value )
    {
        case CSMI_SUCCESS:
	    	puts("---");
	    	printf("Total_Records: %i\n", output->results_count);
	    	for (i = 0; i < output->results_count; i++) {
	    		printf("RECORD_%i:\n", i+1);
	    		printf("  serial_number:  %s\n", output->results[i]->serial_number);
				printf("  discovery_time:  %s\n", output->results[i]->discovery_time);
				printf("  collection_time: %s\n", output->results[i]->collection_time);
	    		printf("  comment:         %s\n", output->results[i]->comment);
				printf("  guid_s1:         %s\n", output->results[i]->guid_s1);
				printf("  guid_s2:         %s\n", output->results[i]->guid_s2);
				printf("  identifier:      %s\n", output->results[i]->identifier);
				printf("  length:          %s\n", output->results[i]->length);
				printf("  name:            %s\n", output->results[i]->name);
				printf("  part_number:     %s\n", output->results[i]->part_number);
				printf("  port_s1:         %s\n", output->results[i]->port_s1);
				printf("  port_s2:         %s\n", output->results[i]->port_s2);
				printf("  revision:        %s\n", output->results[i]->revision);
				printf("  severity:        %s\n", output->results[i]->severity);
				printf("  type:            %s\n", output->results[i]->type);
				printf("  width:           %s\n", output->results[i]->width);
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
