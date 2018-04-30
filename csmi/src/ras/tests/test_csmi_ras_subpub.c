/*================================================================================

    csmi/src/ras/tests/test_csmi_ras_subpub.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "csm_api_ras.h"




// test case for ras pub and sub...

#if 0
typedef void (*ras_event_subscribe_callback)(csm_api_object *csm_obj,
                                             csmi_ras_event_vector_t *event_vect);
int csm_ras_subscribe_callback(csm_api_object **csm_obj,
                               ras_event_subscribe_callback rasEventCallback);
#endif

unsigned g_rasEventCount = 0;

void local_ras_callback(csmi_ras_event_vector_t *event_vect)
{
    unsigned n;
    printf("event_vect->num_ras_events = %d\n", event_vect->num_ras_events);

    // now do we want to test this that we actually got the callback...
    for (n = 0; n < event_vect->num_ras_events; n++) {
        csmi_ras_event_t *pevent = event_vect->events[n];
        g_rasEventCount++;

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

int main(int argc, char *argv[])
{
  int errcode = 0;
  int rc;
  csm_api_object *csm_obj;
  time_t rawtime;
  struct tm *info;
  char ftime[80];
  //csm_ras_msg_type_t *msgType = NULL;
  //unsigned nErrs;

  csm_init_lib();

  rc = csm_ras_subscribe(&csm_obj,
                         "test.testcat01");       

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);



  rc = csm_ras_unsubscribe(&csm_obj,
                         "test.testcat01");       

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);

  rc = csm_ras_subscribe(&csm_obj,
                         "test.testcat01");       

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);


  rc = csm_ras_subscribe(&csm_obj,
                         "test.#");       

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);

  rc = csm_ras_subscribe(&csm_obj,
                         "+.testcat01");       

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);


  rc = csm_ras_unsubscribe(&csm_obj,            // get them to unsubscribe to all...
                         "ALL");                // same as delete...

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);


  // connect up our callback function...
  rc = csm_ras_subscribe_callback(local_ras_callback);
  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }

  // 
  // now test a subscription where we actually publish an event..
  rc = csm_ras_subscribe(&csm_obj,
                         "test.testcat01");       

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);

  time( &rawtime );
  info = localtime( &rawtime );
  strftime(ftime,80,"%Y-%m-%dT%H:%M:%S %z", info);

  rc = csm_ras_event_create(&csm_obj,
                          "test.testcat01.test01",
                          ftime,
                          "fsgb001",
                          "test ras message",
                          "k1=v1,k2=v2");


  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("TEST FAILED %s\n", errmsg);
      goto fail;
  }
  // it's the csmi library's responsility to free nodeAttributes and other internal space
  csm_api_object_destroy(csm_obj);

  printf("waiting 5 seconds for ras callback\n");
  sleep(5);

  if (g_rasEventCount != 1) {
      printf("no callback detected\n");
      goto fail;
  }

  rc = csm_ras_unsubscribe(&csm_obj,            // get them to unsubscribe to all...
                         "ALL");                // same as delete...

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);


  // leave a subscription around and see that we disconnect it...
  rc = csm_ras_subscribe(&csm_obj,
                         "test.testcat01");       

  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("msg = %s\n", errmsg);
      goto fail;
  }
  csm_api_object_destroy(csm_obj);

  csm_term_lib();

  printf("TEST PASSED\n");

  return errcode;

fail:
  csm_term_lib();
  printf("TEST FAILED\n");
  return(1);


}
