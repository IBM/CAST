/*================================================================================

    csmnet/tests/versioning.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <CPP/endpoint_ptp.h>
#include <CPP/endpoint_ptp_plain.h>
#include <sys/wait.h>


#define DATA_BUFFER_SIZE 8192

#define TEST_INITIAL_PORT 31416


int server() {
  int rc = 0;
  csm::network::EndpointPTP_base *socket = nullptr;
  csm::network::AddressPTP_sptr addr;
  csm::network::EndpointOptionsPTP_sptr opts;
  csm::network::EndpointPTP_base *clt = nullptr;
  try {
    addr = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT );
    opts = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( true );
    socket = new csm::network::EndpointCompute_plain( addr, opts );
    while( clt == nullptr ) {
      clt = dynamic_cast<csm::network::EndpointCompute_plain*>( socket->Accept() );
    }
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error" << e.what() << std::endl;
    rc = -1;
  }

  csm::network::Message msg;
  while( clt->Recv( msg ) == 0 )
  {
    sleep( 1 );
  }

  clt->Send( msg );

  std::cout << "Server: Exiting" << std::endl;

  if( clt ) delete clt;
  if( socket ) delete socket;


  return rc;
}

int client() {
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
    if( !rc ) {
      std::cout << "Connected to: " << SrvAddr << std::endl;
    }
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error: " << e.what() << std::endl;
    rc = -1;
  }

  csm::network::Message msg;
  msg.Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT, 351, 0x1234, 0x1234, geteuid(), getegid(), "TEST" );
  socket->Send( msg );

  while( socket->Recv( msg ) == 0 )
  {
    sleep( 1 );
  }
  std::cout << "Exiting client" << std::endl;

  delete socket;
  return rc;
}

int main(int argc, char *argv[])
{
  int rc = 0;
  pid_t clientPid = fork();

  setLoggingLevel(csmnet, debug);

  if(0 == clientPid) {
    client();
  }
  else if(clientPid > 0) {
    server();
    int status;
    waitpid( clientPid, &status, 0 );
  }
  else {
    perror("Forking child failed");
    rc = clientPid;
  }

  return rc;
}

