/*================================================================================

    csmnet/src/C/csm_network_local.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un

#include "csm_test_utils.h"
#include "csmnet/src/C/csm_network_internal_api_c.h"


int main( int argc, char **argv )
{
  int rc = 0;

  csm_net_endpoint_t *ep = csm_net_unix_Init( 0 );
  rc += TESTFAIL( ep, NULL );

  sleep( 1 );
  csm_net_endpoint_t *src_ep = csm_net_unix_Init( 0 );
  rc += TEST( connect( src_ep->_ep->_Socket, (struct sockaddr *)&ep->_ep->_Addr, sizeof(struct sockaddr_un) ), 0 );
  rc += TEST( connect( ep->_ep->_Socket, (struct sockaddr *)&src_ep->_ep->_Addr, sizeof(struct sockaddr_un) ), 0 );

  rc += TESTFAIL( src_ep, NULL );

  int s = src_ep->_ep->_Socket;

  csm_net_msg_t *src_msg = csm_net_msg_Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_NO_ACK, 910953, geteuid(), getegid(), "Nothing", 7, 0 );
  rc += TESTFAIL( src_msg, NULL );

  if( rc > 0 )
  {
    printf( "Aborting with rc=%d\n", rc );
    return rc;
  }

  csm_net_unix_Send( src_ep->_ep, src_msg );


  csm_net_msg_t *msg = csm_net_unix_Recv( ep->_ep );
  rc += TEST( csm_header_validate( &msg->_Header ), 1 );


  send( s, src_msg, 5, 0 );
  printf( "Sent incomplete msg" );

  sleep( 1 );

  msg = csm_net_unix_Recv( ep->_ep );
  rc += TEST( msg, NULL );

  // complete the header but still fail because the data is missing
  send( s, ((char*)(src_msg)) + 5, sizeof( csm_network_header_t ) - 5, 0 );
  msg = csm_net_unix_Recv( ep->_ep );
  rc += TEST( msg, NULL );

  // complete the message and now succeed
  send( s, csm_net_msg_GetData( src_msg ), csm_net_msg_GetDataLen( src_msg ), 0 ); // send the msg data
  msg = csm_net_unix_Recv( ep->_ep );
  rc += TEST( csm_header_validate( &msg->_Header ), 1 );

  csm_net_unix_Send( src_ep->_ep, src_msg );
  msg = csm_net_unix_Recv( ep->_ep );
  rc += TEST( csm_header_validate( &msg->_Header ), 1 );


  csm_net_unix_Exit( src_ep );
  csm_net_unix_Exit( ep );

  printf( "Exiting with rc=%d\n", rc );
  return rc;
}
