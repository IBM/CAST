/*================================================================================

    csmi/src/ras/tests/csm_ras_event_subscribe.c

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
#include <signal.h>
#include <unistd.h>
#include "csmi/include/csm_api_ras.h"


// global kill variable
int g_killme = 0;

void signalHandler( int signum )
{
   g_killme = 1;
   exit(signum);  

}


void local_ras_callback(csmi_ras_event_vector_t *event_vect)
{
    unsigned n;
    printf("event_vect->num_ras_events = %d\n", event_vect->num_ras_events);

    // now do we want to test this that we actually got the callback...
    for (n = 0; n < event_vect->num_ras_events; n++) {
        csmi_ras_event_t *pevent = event_vect->events + n;

        printf("pevent->msg_id = %s\n", pevent->msg_id);
        printf("pevent->min_time_in_pool %d\n", pevent->min_time_in_pool);
        printf("pevent->suppress_ids %s\n", pevent->suppress_ids);
        printf("pevent->severity %s\n", pevent->severity);
        printf("pevent->time_stamp %s\n", pevent->time_stamp);
        printf("pevent->location_name %s\n", pevent->location_name);
        printf("pevent->processor %d\n", pevent->processor);
        printf("pevent->count %d\n", pevent->count);
        printf("pevent->control_action %s\n", pevent->control_action);
        printf("pevent->message %s\n", pevent->message);
        printf("pevent->raw_data %s\n", pevent->raw_data);
        printf("\n");
    }


}


static void usage(const char *argv0)
{
   fprintf(stderr, "usage: %s topic \\ \n", argv0);
   fprintf(stderr, "      subscribe to a ras topic and wait \n");
   fprintf(stderr, "    This function, used mostly for demonstration and testing, subscribes for a ras topic \n"
                   "      and waits forever for the topic callbacks to happen, \n"
                   "      when they do, it prints them to the screen\n"
                   "      ctrl-c or kill will exit the program \n");
   exit(1);
}


int main(int argc, char *argv[])
{
    int opt;
    int rc;
    char *topic = NULL;

    csm_api_object *csm_obj = NULL;

    struct option longopts[] = {
      {"help", no_argument, 0, 'h'},
      {0,0,0,0}
    };
    int option_index;

    while ((opt = getopt_long(argc, argv, "ht:l:r:k:", longopts, &option_index)) != -1) {

        switch (opt) {
        case 'h':
            usage(argv[0]);
            break;
        default:
            usage(argv[0]);
            break;
        }
    }
    if (optind < argc) {
        if (topic == NULL)
            topic = argv[optind];
    }

    if (topic == NULL) {
        fprintf(stderr, "missing required topic paramter\n");
        usage(argv[0]);
    }

    signal(SIGINT, signalHandler);  

    csm_init_lib();

    rc = csm_ras_subscribe_callback(local_ras_callback);
    if (rc != 0) {
        char *errmsg = csm_api_object_errmsg_get(csm_obj);
        printf("msg = %s\n", errmsg);
        goto fail;
    }

    rc = csm_ras_subscribe(&csm_obj, topic);       
    if (rc != 0) {
        char *errmsg = csm_api_object_errmsg_get(csm_obj);
        printf("msg = %s\n", errmsg);
        goto fail;
    }

    csm_api_object_destroy(csm_obj);

    // NEED a clean signal hander to signal we want to kill this process...
    
    printf("\nwaiting for subscription events\n");
    for (;;) {
        if (g_killme)
            break;
        sleep(60);
    }



    csm_term_lib();

    return(0);
fail:
  csm_term_lib();
  return(1);
}
