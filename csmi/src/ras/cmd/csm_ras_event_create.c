/*================================================================================

    csmi/src/ras/cmd/csm_ras_event_create.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*C Include*/
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
/*CSM Include*/
#include "csmi/include/csm_api_ras.h"
/*Needed for infrastructure logging*/
#include "csmutil/include/csmutil_logging.h"
/* Command line macros for ease of use. */
#include "csmi/src/common/include/csmi_internal_macros.h"

#define free_attributes()                       \
    if ( msg_id ) free( msg_id );               \
    if ( time_stamp ) free( time_stamp );       \
    if ( location_name ) free( location_name ); \
    if ( raw_data ) free( raw_data );           \
    if ( kvcsv ) free( kvcsv );

///< For use as the usage variable in the input parsers.
#define USAGE free_attributes() help

void help() {
	puts("_____CSM_RAS_EVENT_CREATE_CMD_HELP_____");
	puts("USAGE:");
	puts("  csm_ras_event_create ARGUMENTS [OPTIONS]");
	puts("  csm_ras_event_create -m message_id [-k kvcsv] [-l location_name] [-r raw_data] [-t time_stamp] [-h] [-v verbose_level]");
	puts("");
	puts("SUMMARY: Used by someone or some application that wants to generate a RAS event.");
	puts("");
	puts("EXIT STATUS:");
	puts("  0  if OK,");
	puts("  1  if ERROR.");
	puts("");
	puts("ARGUMENTS:");
	puts("  MANDATORY:");
	puts("    csm_ras_event_create has 1 required argument");
	puts("    Argument         | Example value         | Description  ");                                                 
	puts("    -----------------|-----------------------|--------------");
	puts("    -m, --msg_id     | \"system.component.id\" | (STRING) The message_id of the type of RAS event to create. A matching message_id must exist in the 'csm_ras_type' table of the CSM database.");
	puts("                     |                       |");
	puts("  OPTIONAL:");
	puts("    csm_ras_event_create can have 4 optional arguments");
	puts("    Argument            | Example value                | Description  ");                                                 
	puts("    --------------------|------------------------------|--------------");
	puts("    -k, --kvcsv         | \"location=node01,k1=key01\"   | (STRING) Key and value comma separated string for variable substitution in RAS message.");
	puts("                        |                              | Default Value: NULL empty/blank string");
	puts("    -l, --location_name | \"node01\"                     | (STRING) The node name or location name for this RAS event.");
	puts("                        |                              | Default Value: NULL empty/blank string");
	puts("    -r, --raw_data      | \"errcode=42 This node attempted to compute the answer to the ultimate question of life, the universe, and everything.\"  | (STRING) The raw data for this RAS event.");
	puts("                        |                              | Default Value: NULL empty/blank string");
	puts("    -t, --time_stamp    | \"1999-01-15 12:34:56.123456\" | (STRING) A time stamp to use with this RAS event.");
	puts("                        |                              | Default Value: The current time of the database, ie: 'now'");
	puts("");
	puts("GENERAL OPTIONS:");
	puts("[-h, --help]                  | Help.");
	puts("[-v, --verbose verbose_level] | Set verbose level. Valid verbose levels: {off, trace, debug, info, warning, error, critical, always, disable}");
	puts("");
	puts("EXAMPLE OF USING THIS COMMAND:");
	puts("  csm_ras_event_create -m \"system.component.id\" ");
	puts("____________________");
}

struct option longopts[] = {
	//general options
	{"help",          no_argument,       0, 'h'},
	{"verbose",       required_argument, 0, 'v'},
	//arguments
	{"kvcsv",         required_argument, 0, 'k'},
	{"location_name", required_argument, 0, 'l'},
	{"msg_id",        required_argument, 0, 'm'},
	{"raw_data",      required_argument, 0, 'r'},
	{"time_stamp",    required_argument, 0, 't'},
	{0,0,0,0}
};

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
	int option_index = 0;
	
	//ToDo: fix with new serialization structs later.
	//char ftime[80];
    char *msg_id = NULL;
    char *time_stamp = NULL;
    char *location_name = NULL;
    char *raw_data = NULL;
    char *kvcsv = NULL;

	while ((opt = getopt_long(argc, argv, "hv:k:l:m:r:t:", longopts, &option_index)) != -1) {
		switch (opt) {
			case 'h':
                USAGE();
				return CSMI_HELP;
			case 'v':
                csm_set_verbosity( optarg, USAGE )
				break;
			case'k':
                csm_optarg_test( "-k, --kvcsv", optarg, USAGE );
				kvcsv = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 'l':
                csm_optarg_test( "-k, --kvcsv", optarg, USAGE );
				location_name = strdup(optarg);
				optionalParameterCounter++;
				break;
			case'm':
                csm_optarg_test( "-m, --msg_id", optarg, USAGE );
				msg_id = strdup(optarg);
				requiredParameterCounter++;
				break;
			case 'r':
                csm_optarg_test( "-r, --raw_data", optarg, USAGE );
				raw_data = strdup(optarg);
				optionalParameterCounter++;
				break;
			case 't':
                csm_optarg_test( "-t, --time_stamp", optarg, USAGE );
				time_stamp = strdup(optarg);
				optionalParameterCounter++;
				break;	
			default:
                csmutil_logging(error, "unknown arg: '%c'\n", opt);
                USAGE();
				return CSMERR_INVALID_PARAM;
        }
    }
	
	//ToDo: Remove this. Use database 'now' on back end if no field supplied?
    if (time_stamp == NULL) 
    {
        time_stamp = strdup("now");
        /*
        time_t rawtime;
        struct tm *info;

        time( &rawtime );

        info = localtime( &rawtime );

        strftime(ftime,80,"%Y-%m-%dT%H:%M:%S %z", info);
        time_stamp = strdup(ftime);// If this is not duped it becomes hard to free.
        */
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
		return CSMERR_MISSING_PARAM;
	}
	
	/* Success required to be able to communicate between library and daemon - csmi calls must be made inside the frame created by csm_init_lib() and csm_term_lib()*/
	return_value = csm_init_lib();
	if( return_value != 0){
		csmutil_logging(error, "%s-%d:", __FILE__, __LINE__);
		csmutil_logging(error, "  csm_init_lib rc= %d, Initialization failed. Success is required to be able to communicate between library and daemon. Are the daemons running?", return_value);
        free_attributes()
		return return_value;           
	}
	
	/* Call the actual CSM API */
    return_value = csm_ras_event_create(&csm_obj, 
        msg_id, time_stamp, location_name, raw_data, kvcsv);
    
    if (return_value != CSMI_SUCCESS) 
    {
        printf("%s FAILED: errcode: %d errmsg: %s\n",
            argv[0], return_value,  csm_api_object_errmsg_get(csm_obj));
    }

	// Free up the csm object and attributes
    csm_api_object_destroy(csm_obj);
    free_attributes()
	
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
