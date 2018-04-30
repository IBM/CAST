/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_delete.c

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
#include <inttypes.h>
#include "csmi/include/csm_api_workload_manager.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
  int                retval;
  csm_allocation_delete_input_t input;
  csm_api_object    *csm_obj = NULL;

  assert (csm_init_lib() == 0);

  input.allocation_id = 1;

  printf("\ncsm_allocation_delete %ld:\n", input.allocation_id);
  retval = csm_allocation_delete(&csm_obj, &input);

  if (retval == 0) {
    printf("\tallocation %ld successfully deleted\n", input.allocation_id);
  }
  else {
    printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", argv[0], csm_api_object_errcode_get(csm_obj),
           csm_api_object_errmsg_get(csm_obj));
  }

  // it's the csmi library's responsibility to free internal space
  csm_api_object_destroy(csm_obj);

  assert( csm_term_lib() == 0);

  return 0;
}
