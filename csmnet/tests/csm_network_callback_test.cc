/*================================================================================

    csmnet/tests/csm_network_callback_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_network_callback_test.cc
 *
 ******************************************/

#include <iostream>
#include <memory>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>

#include <logging.h>
#include "csmutil/include/csm_test_utils.h"
#include <csm_network_header.h>
#include <CPP/csm_network_msg_cpp.h>
#include <CPP/csm_network_exception.h>
#include <CPP/endpoint.h>
#include <CPP/endpoint_dual_unix.h>

#include <csm_network_config.h>
#include <C/csm_network_msg_c.h>
#include <C/csm_network_internal_api_c.h>


#define DATA_BUFFER_SIZE 8192

#define CSM_NETWORK_INTERNAL_SERVER_SOCKET ( true )
#define CSM_NETWORK_INTERNAL_CLIENT_SOCKET ( false )

int server_echo( )
{
    std::cout << "Server: Starting" << std::endl;

    int rc = 0;

    csm::network::EndpointDualUnix *socket = NULL;
    try {
        socket = new csm::network::EndpointDualUnix( CSM_NETWORK_LOCAL_SSOCKET,
                                                     std::make_shared<csm::network::EndpointOptionsUnix>( CSM_NETWORK_INTERNAL_SERVER_SOCKET ) );
    }
    catch (csm::network::Exception &e) {
        std::cout << "Socket creation error" << e.what() << std::endl;
        rc = -1;
    }

    if( ! rc ) {

        char *buffer = (char*)malloc( DATA_BUFFER_SIZE );
        bzero( buffer, DATA_BUFFER_SIZE );

        for( int i=0; ( i<1 ) && ( rc == 0 ); i++) {
            csm::network::AddressUnix_sptr RemoteAddr;

            try {
              size_t rlen = 0;
              csm::network::MessageAndAddress msgAddr;
              while( rlen == 0 )
              {
                rlen = socket->RecvFrom( msgAddr );
                if( rlen > 0)
                  RemoteAddr = std::dynamic_pointer_cast<csm::network::AddressUnix>( msgAddr.GetAddr() );
              }
              std::cout << "Server: Receved " << rlen << " bytes, clnt_add: " << RemoteAddr->Dump() << std::endl;

              msgAddr._Msg.SetCbk();   // make sure we echo via CB channel
              msgAddr._Msg.CheckSumUpdate(); // update the checksum after flag change...
              rc = socket->SendTo( msgAddr._Msg, msgAddr.GetAddr() );
              if( rc > 0 )
                rc = 0;
            }
            catch (csm::network::Exception &e)
            {
              std::cout << "Send/Recv error" << e.what() << std::endl;
              rc = EINVAL;
            }
        }
    }
    else {
        perror("Server: Binding");
    }
    sleep(2);

    delete socket;

    std::cout << "Server: Exiting rc=" << rc << std::endl;
    return rc;
}

pthread_cond_t callback_done;
pthread_mutex_t callback_mutex;

int testcallback( csm_net_msg_t *msg )
{
  printf("ENTERING CALLBACK:\n");
  csm_net_msg_Dump( msg );
  pthread_mutex_lock( &callback_mutex );
  pthread_cond_signal( &callback_done );
  pthread_mutex_unlock( &callback_mutex );
  return 0;
}


int client_test()
{
  int rc = 0;
  struct timespec timeout;

  if( pthread_mutex_init( &callback_mutex, NULL ) )
    return 1;

  if( pthread_mutex_lock( &callback_mutex ) )
    return 2;

  if( pthread_cond_init( &callback_done, NULL ) )
    return 3;

  csm_net_endpoint_t *ep = csm_net_unix_Init( 0 );
  if( ep == NULL )
  {
    perror("Client.Init(): ");
    return ENOMEM;
  }
  rc = csm_net_unix_Connect( ep, CSM_NETWORK_LOCAL_SSOCKET );
  if( rc )
  {
    perror("Client.Connect(): ");
    csm_net_unix_Exit( ep );
    return rc;
  }
  rc = csm_net_unix_RegisterCallback( ep, testcallback );

  csm_net_msg_t *msg;
  char *data = (char*)malloc(1024);
  strncpy( data, "Hello World", 10 );

  msg = csm_net_msg_Init( CSM_CMD_ECHO, 1, CSM_PRIORITY_DEFAULT, 1243567, 0x01020304, 0x04030201, geteuid(), getegid(), data, 10 );

  rc = csm_net_unix_Send( ep->_ep, msg );
  if( rc <= 0 )
  {
    perror("Client.Send: ");
    csm_net_unix_Exit( ep );
    return rc;
  }

  // timedwait requires absolute time... aka deadline
  struct timeval cond_deadline;
  gettimeofday(&cond_deadline, NULL);
  timeout.tv_sec = cond_deadline.tv_sec + 5;
  timeout.tv_nsec = 0;
  rc = pthread_cond_timedwait( &callback_done, &callback_mutex, &timeout);
  if(( rc ) && ( errno == ETIMEDOUT ))
  {
    perror("Main Thread waiting for callback: ");
    rc = ETIMEDOUT;
  }

  rc = csm_net_unix_UnRegisterCallback( ep);

  pthread_mutex_unlock( &callback_mutex );
  rc += csm_net_unix_Exit( ep );

  rc += pthread_cond_destroy( &callback_done );
  rc += pthread_mutex_destroy( &callback_mutex );

  std::cout << "Client: Exiting rc=" << rc << std::endl;
  return rc;
}

int main(int argc, char **argv)
{
    int rc = 0;

    pid_t clientPid = fork();

    if( clientPid == 0 )
      rc = server_echo();

    if( clientPid > 0 )
    {
      rc = client_test();
      int status;
      waitpid( clientPid, &status, 0 );
      rc += WEXITSTATUS(status);
    }

    if( clientPid < 0 )
    {
      perror("Forking child failed");
      rc = clientPid;
    }

    return rc;
}

