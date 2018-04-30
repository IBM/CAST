/*================================================================================

    csmi/src/ras/tests/csm_ras_msg_type_query.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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
#include <time.h>
#include <sys/time.h>
#include "csmi/include/csm_api_ras.h"




static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [-i <msg_id> ] [-p|--suppressids <suppress_ids>] \\ \n"
                    "                   [-s|--severity <severity> ] [-m|--message <message> ] [-c|--control_aciton <control_action> ] \\ -n"
                    "                   [-n <limit> ] [-o offset ] \n", argv0);
    fprintf(stderr, "                   -i|--msg_id msg_id -- msgid search string i.e. csmapi.%%\n");
    fprintf(stderr, "                   -p|--suppressids suppress_ids -- suppress_ids search string \n");
    fprintf(stderr, "                   -s|--severity severity -- severity search string \n");
    fprintf(stderr, "                   -c|--control_action control_action -- control_action search string \n");
    fprintf(stderr, "                   -m|--message message  -- message search string \n");
    fprintf(stderr, "                   -n|--limit limit -- limit numeric value\n");
    fprintf(stderr, "                   -o|--offset offset -- offset numeric value\n");
   exit(1);
}





int main(int argc, char *argv[])
{
    int opt;
    int rc;
    csm_ras_msg_type_vector_t *msg_type_vect;
    const char *msg_id = NULL;
    const char *suppress_ids = NULL;
    const char *severity = NULL;
    const char *control_action = NULL;
    const char *message = NULL;
    unsigned limit = 0;
    unsigned offset = 0;
    unsigned n;

    csm_api_object *csm_obj;

    struct option longopts[] = {
      {"help", no_argument, 0, 'h'},
      {"msg_id", required_argument, 0, 'i'},
      {"severity", required_argument, 0, 's'},
      {"suppressids", required_argument, 0, 'p'},
      {"control_action", required_argument, 0, 'c'},
      {"message", required_argument, 0, 'm'},
      {"limit", required_argument, 0, 'n'},
      {"offset", required_argument, 0, 'o'},

      {0,0,0,0}
    };
    int option_index;


    while ((opt = getopt_long(argc, argv, "hi:p:s:c:m:n:o:", longopts, &option_index)) != -1) {

        switch (opt) {
			case 'h':
				usage(argv[0]);
				break;
			case 'i':
				msg_id = optarg;
				break;
			case 'p':
				suppress_ids = optarg;
				break;
			case 's':
				severity = optarg;
				break;
			case 'c':
				control_action = optarg;
				break;
			case 'm':
				message = optarg;
				break;
			case 'n':
				limit = atoi(optarg);     // todo, error check this...
				break;
			case 'o':
				offset = atoi(optarg);    // todo error check this..
				break;
			default:
				usage(argv[0]);
				break;
		}
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc) {
        if (msg_id == NULL)
            msg_id = argv[optind];
    }



    csm_init_lib();

    rc = csm_ras_msg_type_query(&csm_obj,
                             msg_id,
                             suppress_ids,
                             severity,
                             control_action,
                             message,
                             limit,
                             offset,
                             &msg_type_vect);



    if (rc != 0) {
        char *errmsg = csm_api_object_errmsg_get(csm_obj);
        printf("FAILED %s\n", errmsg);
        exit(1);
    }
	
	if(msg_type_vect->num_ras_msg_types > 0){
		puts("---");
		printf("Total_Records: %i\n", msg_type_vect->num_ras_msg_types);
		for (n = 0; n < msg_type_vect->num_ras_msg_types; n++) {
#if 0
			const char *msg_id;
			int min_time_in_pool;
			const char *suppress_ids;
			const char *severity;
			const char *message;
			const char *decoder;
			const char *control_action;
			const char *description;
			int  threshold_count;
			const char *threshold_period;
			const char *relevant_diags;
#endif 
			csm_ras_msg_type_t *ptype = msg_type_vect->msg_types + n;
			printf("RECORD_%i:\n", n+1);
			printf("  msg_id:           %s\n", ptype->msg_id);
			printf("  min_time_in_pool: %d\n", ptype->min_time_in_pool);
			printf("  suppress_ids:     %s\n", ptype->suppress_ids);
			printf("  severity:         %s\n", ptype->severity);
			printf("  message:          %s\n", ptype->message);
			printf("  decoder:          %s\n", ptype->decoder);
			printf("  control_action:   %s\n", ptype->control_action);
			printf("  description:      %s\n", ptype->description);
			printf("  threshold_count:  %d\n", ptype->threshold_count);
			printf("  threshold_period: %s\n", ptype->threshold_period);
			printf("  relevant_diags:   %s\n", ptype->relevant_diags);
		}
		puts("...");
	}else if(msg_type_vect->num_ras_msg_types == 0){
		puts("---");
		printf("Total_Records: %i\n", msg_type_vect->num_ras_msg_types);
		printf("# No matching record found.\n");
		puts("...");
	}else{
		//ToDo: ERROR: FIX LATER
		return 1;
	}
    csm_api_object_destroy(csm_obj);
	
    csm_term_lib();

    return(0);
}

