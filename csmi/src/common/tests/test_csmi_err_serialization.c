/*================================================================================

    csmi/src/common/tests/test_csmi_err_serialization.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "../include/csmi_serialization.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

const char MSG[] = "THIS";

void test_csmi_err(void)
{
 
  // test node_attributes_pack and node_attributes_unpack
  char *buf;
  uint32_t bufLen;
  csmi_err_t *unpackVal;
  int errcode=10;

  printf("Testing csmi_err_pack and csmi_err_unpack...\n");

  buf = csmi_err_pack(errcode, MSG, &bufLen);
  unpackVal = csmi_err_unpack(buf, bufLen);
  assert(unpackVal->errcode == errcode);
  assert(strcmp(unpackVal->errmsg, MSG) == 0);
}

int main(int argc, char *argv[]) {
  test_csmi_err();
  return 0;
}
