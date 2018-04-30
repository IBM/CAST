/*================================================================================

    csmd/src/daemon/tests/csm_daemon_echo_test.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_daemon_echo_test_c.c
 *
 ******************************************/

#include <stdio.h>
#include <stdlib.h>

#include "csmi/src/common/include/csmi_cmds.h"
#include "csmnet/src/C/csm_network_internal_api_c.h"

int client()
{
  int rc = 0;

  csm_net_endpoint_t *ep = csm_net_unix_Init( 0 );
  if( ep == NULL )
  {
    perror("Client.Init(): ");
    return ENOMEM;
  }
  rc = csm_net_unix_Connect( ep, getenv("CSM_SSOCKET") );
  if( rc )
  {
    perror("Client.Connect(): ");
    csm_net_unix_Exit( ep );
    return rc;
  }

  csm_net_msg_t *msg;
  char *data = (char*)malloc(1024);
  strncpy( data, "Hello World", 10 );

  msg = csm_net_msg_Init( CSM_CMD_ECHO, 1, CSM_PRIORITY_DEFAULT, 1243567, geteuid(), getegid(), data, 10, 0 );

  rc = csm_net_unix_Send( ep->_ep, msg );
  if( rc <= 0 )
  {
    perror("Client.Send: ");
    csm_net_unix_Exit( ep );
    return rc;
  }

  csm_net_msg_t *rmsg = csm_net_unix_Recv( ep->_ep );
  if( ! rmsg )
  {
    perror("Client.Recv: ");
    rc = errno;
    csm_net_unix_Exit( ep );
    return rc;
  }

  if( csm_net_msg_CheckSumCalculate( msg ) != csm_net_msg_CheckSumCalculate( rmsg ) )
    rc = 1;
  else
    rc = 0;

  rc = csm_net_unix_Exit( ep );
  return rc;
}

int main( int argc, char **argv )
{
  int rc = 0;

  // need a client that feeds network messages into a server
  rc += client( );

  return rc;
}

