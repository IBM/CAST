/*================================================================================

    csmnet/tests/csm_network_transmission_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_network_header_test.cc
 *
 ******************************************/


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
#include <CPP/endpoint_unix.h>

#define DATA_BUFFER_SIZE 8192

#define CSM_NETWORK_INTERNAL_SERVER_SOCKET ( true )
#define CSM_NETWORK_INTERNAL_CLIENT_SOCKET ( false )

int server_echo( const bool aMsg = false )
{
    std::cout << "Server: Starting" << std::endl;

    int rc = 0;

    csm::network::EndpointUnix *socket = NULL;
    try {
        socket = new csm::network::EndpointUnix( CSM_NETWORK_LOCAL_SSOCKET,
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
            csm::network::AddressUnix_sptr RemoteAddr = std::make_shared<csm::network::AddressUnix>();

            try {
              size_t rlen = 0;
              csm::network::MessageAndAddress msgAddr;
              while( rlen == 0 )
              {
                rlen = socket->RecvFrom( msgAddr );
                RemoteAddr = std::dynamic_pointer_cast<csm::network::AddressUnix>( msgAddr.GetAddr() );
              }
              std::cout << "Server: Receved " << rlen << " bytes, clnt_add: " << RemoteAddr->_SockAddr.sun_path << std::endl;

              socket->SendTo( msgAddr._Msg, RemoteAddr );
            }
            catch (csm::network::Exception &e)
            {
              std::cout << "Send/Recv error" << e.what() << std::endl;
              rc = -1;
            }
        }
    }
    else {
        perror("Server: Binding");
    }
    sleep(2);

    delete socket;

    std::cout << "Server: Exiting" << std::endl;
    return rc;
}

int client_test()
{
    sleep(1);
    std::cout << "Starting client" << std::endl;

    int rc = 0;

    csm::network::EndpointUnix *socket = NULL;
    try {
      socket = new csm::network::EndpointUnix( CSM_NETWORK_LOCAL_CSOCKET,
                                               std::make_shared<csm::network::EndpointOptionsUnix>( CSM_NETWORK_LOCAL_SSOCKET ) );
    }
    catch (csm::network::Exception &e) {
      std::cout << "Socket creation error" << e.what() << std::endl;
    }

    try {
      csm::network::AddressUnix SrvAddr( CSM_NETWORK_LOCAL_SSOCKET );

      std::cout << "Connecting to: " << CSM_NETWORK_LOCAL_SSOCKET << std::endl;
      rc = socket->Connect( std::make_shared<csm::network::AddressUnix>( SrvAddr ) );
    }
    catch (csm::network::Exception &e) {
        std::cout << "Socket creation error" << e.what() << std::endl;
        rc = -1;
    }
    std::cout << "Connected to: " << CSM_NETWORK_LOCAL_SSOCKET << std::endl;

    csm::network::Message out, in;

    out.Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT, 0xDEADBEEF, 0, 0, getuid(), getgid(), "Hello World");

    if( ! rc ) {
        socket->Send( out );

        while( socket->Recv( in ) == 0 )
        {};
    }
    else {
      perror("Client: Connection failed");
    }

    if( (rc = memcmp( &out, &in, sizeof( csm_network_header ))) != 0 ) {
      perror("Client: Input and Output data mismatch");
    }

    delete socket;
    unlink( CSM_NETWORK_LOCAL_CSOCKET );

    std::cout << "Exiting client" << std::endl;
    return rc;
}

int main(int argc, char **argv)
{
    int rc = 0;
    bool DebugClient = false;

    pid_t clientPid = fork();

    if( clientPid == 0 )
    {
      if( DebugClient )
        server_echo();
      else
        client_test();
    }

    if( clientPid > 0 )
    {
      if( DebugClient )
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
