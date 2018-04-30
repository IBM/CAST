/*================================================================================

    csmi/src/common/tests/test_csmi_cli.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>

extern int csmi_client(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  return csmi_client(argc, argv);
}
