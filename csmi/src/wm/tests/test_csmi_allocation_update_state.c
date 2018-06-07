/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_update_state.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/include/csm_api_workload_manager.h"
#include "csmutil/include/csmutil_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>


int csmi_client(int argc, char *argv[])
{
  int retval = 0;
  uint32_t i;
  int j;
  uint64_t allocation_id= 3;
  csm_api_object    *csm_obj = NULL;
  int dataCount = 0;
  
  csmutil_logging_level_set("trace"); 

  csmutil_logging(trace, "%s-%d: csm_allocation_query_active_all Begin.", __FILE__, __LINE__);

  assert (csm_init_lib() == 0);

  retval = csm_allocation_update_state( &csm_obj, allocation_id, STAGING_IN);

  retval = csm_allocation_update_state( &csm_obj, allocation_id, RUNNING);

  retval = csm_allocation_update_state( &csm_obj, allocation_id, STAGING_OUT);

  csm_api_object_destroy(csm_obj);

  assert( csm_term_lib() == 0);
  csmutil_logging(trace, "%s-%d: csm_allocation_query_active_all End.", __FILE__, __LINE__);

  return retval;
}
