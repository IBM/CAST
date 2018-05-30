/*================================================================================

    csmi/src/common/tests/test_csmi_corner_cases.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


#include "csmi/include/csmi_type_common.h"

#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_common_utils.h"


int csmi_undef_hdl(csm_api_object **csm_obj)
{
  csmi_cmd_t ExpectedCmd = CSM_error_inject;
  int errcode;
  uint32_t recvDataLen=0;
  char *recvData=NULL;

  *csm_obj = csm_api_object_new(ExpectedCmd, NULL);
  assert(*csm_obj != NULL);

  errcode = csmi_sendrecv_cmd(*csm_obj, ExpectedCmd, NULL, 0, &recvData, &recvDataLen);

  assert(errcode != CSMI_SUCCESS);
  assert(recvData == NULL && recvDataLen == 0);
 
  return errcode;
}

int csmi_undef_cmd(csm_api_object **csm_obj)
{

  csmi_cmd_t ExpectedCmd = CSM_error_inject;
  int errcode;
  uint32_t recvDataLen=0;
  char *recvData=NULL;

  *csm_obj = csm_api_object_new(ExpectedCmd, NULL);
  assert(*csm_obj != NULL);

  errcode = csmi_sendrecv_cmd(*csm_obj, ExpectedCmd, NULL, 0, &recvData, &recvDataLen);

  assert(errcode != CSMI_SUCCESS);
  assert(recvData == NULL && recvDataLen == 0);

  return errcode;
}

int csmi_client(int argc, char *argv[])
{
  int retval = 0;
  csm_api_object *csm_obj = NULL;

  assert (csm_init_lib() == 0);

  retval = csmi_undef_hdl(&csm_obj);
  assert (retval != 0);

  printf("client:csm_undef_hdl: errcode=%d errmsg=\"%s\"\n",
		csm_api_object_errcode_get(csm_obj),
		csm_api_object_errmsg_get(csm_obj));

  csm_api_object_destroy(csm_obj);

  retval = csmi_undef_cmd(&csm_obj);
  assert (retval != 0);

  printf("client:csm_undef_cmd: errcode=%d errmsg=\"%s\"\n",
                csm_api_object_errcode_get(csm_obj),
                csm_api_object_errmsg_get(csm_obj));

  csm_api_object_destroy(csm_obj);

  assert(csm_term_lib() == 0);

  return 0;
}
