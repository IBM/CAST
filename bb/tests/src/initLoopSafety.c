/*******************************************************************************
 |    initLoopSafety.c
 |
 |  The purpose of the progam is to verify that the burst buffer API interfaces,
 |  as defined in bbapi.h, match the actual implementations for those APIs.
 |  The intent is that this program should successfully compile and link.
 |  It is not intended for this program to be run.
 |
 |  � Copyright IBM Corporation 2016,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"

int main(int argc, char **argv)
{
  /* do declares */
  int rc = 0;
  int i = 0;
  uint32_t l_contribId = 0;

  for (i = 0; i < 100; i++)
  {
    rc = BB_InitLibrary(l_contribId, BBAPI_CLIENTVERSIONSTR);
    if (rc)
    {
      printf("InitLibrary failed rc=%d\n", rc);
      exit(-1);
    }
    printf("InitLibrary called i=%d\t", i);

    rc = BB_TerminateLibrary();
    if (rc)
    {
      printf("BB_TerminateLibrary failed\n");
    }
    else
      printf("TerminateLibrary called i=%d\n", i);
  }
  return rc;
}
