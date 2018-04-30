/*================================================================================

    csmi/src/ras/tests/test_csmi_ras_event_create.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "csm_api_ras.h"

#include "csmutil/include/csmutil_logging.h"



#if 0
{
    "msgid": "ufmevents.iblink.linkdown",
    "timestamp": "2015-10-26T12:09:25",
    "location": "fsgb001",
    "rawdata": "[14599] [329] WARNING [Fabric_Topology] Link [Source f4521403008817f0_12  TO Dest: e41d2d0300a4f482_1]: Link went down: f4521403008817f0 (Switch: mellanox-SX6036) : 12 - e41d2d0300a4f482 (Computer: fsgb001 HCA-1) : 1",
    "message": "ib link for fsgb001 is down",
    "category": "ufmevents",
    "component": "iblink",
    "severity": "FATAL",
    "decoder": "",
    "ctlAction": "kill_job",
    "description": "UFM has detected the ib link is down",
    "svcAction": "",
    "relevantDiags": "none",
    "thresholdCount": "0",
    "thresholdPeriod": "0"
}

#endif


int main(int argc, char *argv[])
{
  int errcode = 0;
  int rc;
  csm_api_object *csm_obj;

   time_t rawtime;
   struct tm *info;
   char ftime[80];

   time( &rawtime );

   info = localtime( &rawtime );

   strftime(ftime,80,"%Y-%m-%dT%H:%M:%S %z", info);


  //int i;
  //int nodeCount;
  //csm_api_object *csm_obj = NULL;

  csm_init_lib();

  //csmutil_logging_level_set("trace");
  // how do we actually test this??  there is no feedback here, 
  // since this is a write only interface...
  rc = csm_ras_event_create(&csm_obj,
                          "test.testcat01.test01",
                          ftime,
                          "fsgb001",
                          "test ras message",
                          "k1=v1,k2=v2");


  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("TEST FAILED %s\n", errmsg);
      exit(1);
  }
  // it's the csmi library's responsility to free nodeAttributes and other internal space
  csm_api_object_destroy(csm_obj);

#if 0

  rc = csm_ras_event_create(&csm_obj,
                          "undefined.error.for.me",
                          "2015-10-26T12:09:25",
                          "xxxx",
                          "xxxx",
                          NULL);
  csm_api_object_destroy(csm_obj);
  if (rc == 0) {
      //char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("TEST FAILED expecting an error, but got CSMI_SUCCESS\n");
      exit(1);
  }

#endif  
  rc = csm_ras_event_create(&csm_obj,
                          "test.testcat01.test02",
                          ftime,
                          "fsgb001",
                          "test ras message",
                          "k1=v1,k2=v2");
  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("TEST FAILED %s\n", errmsg);
      exit(1);
  }
  csm_api_object_destroy(csm_obj);

  rc = csm_ras_event_create(&csm_obj,
                          "test.testcat01.test03",
                          ftime,
                          "fsgb001",
                          "test ras message",
                          "k1=v1,k2=v2");
  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("TEST FAILED %s\n", errmsg);
      exit(1);
  }
  csm_api_object_destroy(csm_obj);

  rc = csm_ras_event_create(&csm_obj,
                          "test.testcat01.test04",
                          ftime,
                          "fsgb001",
                          "test ras message",
                          "k1=v1,k2=v2");
  if (rc != 0) {
      char *errmsg = csm_api_object_errmsg_get(csm_obj);
      printf("TEST FAILED %s\n", errmsg);
      exit(1);
  }
  csm_api_object_destroy(csm_obj);




  csm_term_lib();

  printf("TEST PASSED\n");

  return errcode;
}
