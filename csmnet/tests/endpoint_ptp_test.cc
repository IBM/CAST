/*================================================================================

    csmnet/tests/endpoint_ptp_test.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include <iostream>
#include <memory>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <logging.h>
#include "csmutil/include/csm_test_utils.h"
#include <csm_network_header.h>
#include <CPP/csm_network_msg_cpp.h>
#include <CPP/csm_network_exception.h>
#include <CPP/endpoint.h>
#include <CPP/endpoint_ptp.h>


#define DATA_BUFFER_SIZE 8192

#define TEST_INITIAL_PORT 31416

#define CSM_NETWORK_INTERNAL_SERVER_SOCKET ( true )
#define CSM_NETWORK_INTERNAL_CLIENT_SOCKET ( false )

int server_echo( const bool aMsg = false )
{
  std::cout << "Server: Starting" << std::endl;

  int rc = 0;

  csm::network::EndpointPTP_base *socket = nullptr;
  csm::network::AddressPTP_sptr addr;
  csm::network::EndpointOptionsPTP_sptr opts;
  csm::network::EndpointPTP_base *clt = nullptr;
  try {
    addr = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT );
    opts = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( true );
    socket = new csm::network::EndpointCompute_plain( addr, opts );
    while( clt == nullptr )
      clt = dynamic_cast<csm::network::EndpointCompute_plain*>( socket->Accept() );
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error" << e.what() << std::endl;
    rc = -1;
  }

  if( ! rc ) {

    char *buffer = (char*)malloc( DATA_BUFFER_SIZE );
    bzero( buffer, DATA_BUFFER_SIZE );

    for( int i=0; ( i<1 ) && ( rc == 0 ); i++) {
      csm::network::AddressPTP_sptr RemoteAddr = std::make_shared<csm::network::AddressPTP>();

      try {
        size_t rlen = 0;
        csm::network::MessageAndAddress msgAddr;
        std::cout << "Server: going to recv" << std::endl;
        while( rlen == 0 )
        {
          rlen = clt->RecvFrom( msgAddr );
          RemoteAddr = std::dynamic_pointer_cast<csm::network::AddressPTP>( msgAddr.GetAddr() );
        }
        std::cout << "Server: Receved " << rlen
          << " bytes, clnt_add: " << RemoteAddr << std::endl;

        clt->SendTo( msgAddr._Msg, RemoteAddr );
      }
      catch (csm::network::Exception &e)
      {
        std::cout << "Send/Recv error" << e.what() << std::endl;
      }
    }
    free(buffer);
  }
  else {
    perror("Server: Binding");
  }
  std::cout << "Server: Exiting" << std::endl;

  if( clt ) delete clt;
  if( socket ) delete socket;

  return rc;
}

int client_test()
{
  std::cout << "Starting client" << std::endl;

  int rc = 0;

  csm::network::EndpointPTP_base *socket = nullptr;
  csm::network::AddressPTP_sptr addr;
  csm::network::EndpointOptionsPTP_sptr opts;
  csm::network::SSLFilesCollection sslfiles;
  try {
    addr = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT+1 );
    opts = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( 0x7f000001, TEST_INITIAL_PORT, sslfiles );
    socket = new csm::network::EndpointCompute_plain( addr, opts );
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error" << e.what() << std::endl;
  }

  try {
    csm::network::AddressPTP SrvAddr( 0x7f000001, TEST_INITIAL_PORT );

    std::cout << "Connecting to: " << SrvAddr << std::endl;
    rc = socket->Connect( std::make_shared<csm::network::AddressPTP>( SrvAddr ) );
    if( !rc )
      std::cout << "Connected to: " << SrvAddr << std::endl;
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error: " << e.what() << std::endl;
    rc = -1;
  }

  csm::network::Message out, in;

  out.Init( CSM_CMD_ECHO,
            0, CSM_PRIORITY_DEFAULT, 0xDEADBEEF, 0, 0, getuid(), getgid(), "Hello world");

  if( ! rc ) {
    try
    {
      rc = socket->Send( out );
      LOG(csmnet, info) << "client_test(): Send returned: " << rc;
      rc *= (rc < 0);  // keep status if < 0

      LOG(csmnet, info) << "client_test(): entering recv: " << rc;
      while( (!rc) && ( rc = socket->Recv( in )) == 0 )
      {};

    }
    catch ( csm::network::Exception &e)
    {
      LOG(csmnet, error) << "client_test(): Network Send error" << e.what();
      if( ! rc ) rc = errno;
    }

    if( (rc = memcmp( &out, &in, sizeof( csm_network_header ))) != 0 ) {
      perror("Client: Input and Output data mismatch");
    }
  }
  else {
    perror("Client: Connection failed");
  }
  std::cout << "Exiting client" << std::endl;

  delete socket;

  return rc;
}

int main(int argc, char **argv)
{
    int rc = 0;
    const bool ServerClientSwitch = false;

    pid_t clientPid = fork();

    if( clientPid == 0 )
    {
      if( ServerClientSwitch )
        server_echo();
      else
        client_test();
    }

    if( clientPid > 0 )
    {
      if( ServerClientSwitch )
        client_test();
      else
        server_echo();

      int status;
      waitpid( clientPid, &status, 0 );
    }

    if( clientPid < 0 )
    {
      perror("Forking child failed");
      rc = clientPid;
    }
    return rc;
}
