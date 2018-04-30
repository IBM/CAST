/*================================================================================

    csmd/src/daemon/tests/csm_network_engine_test.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

// test source, sink, and processor for network handling

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include <logging.h>

#include <csm_network_header.h>
#include <csmnet/src/CPP/csm_network_msg_cpp.h>
#include <csmnet/src/CPP/csm_network_exception.h>
#include <csmnet/src/CPP/endpoint.h>
#include <csmnet/src/CPP/endpoint_ptp.h>
#include <csmnet/src/CPP/endpoint_dual_unix.h>
#include <csmnet/src/CPP/endpoint_multi_unix.h>
#include "csmnet/src/CPP/multi_endpoint.h"
#include <csmnet/src/CPP/reliable_msg.h>

#include "include/csm_event_manager.h"
#include "include/csm_daemon_network_manager.h"
#include "csmutil/include/csm_test_utils.h"

#include "include/csm_event_routing.h"
#include "csmd/include/csm_daemon_config.h"

#define NUMTESTS ( 1 )

int server( pid_t aClient )
{
  std::cout << "Server endpoint created: " << std::endl;
  int rc = 0;

  csm::daemon::EventRoutingUtility eventRouting;
  csm::daemon::DaemonStateMaster daemonState(csm::daemon::RUN_MODE::STARTED, 0x854 );
  csm::daemon::EventManagerNetwork nwm((csm::daemon::EventRouting *) &eventRouting, &daemonState );

  csm::daemon::EventSourceNetwork netSrc( &nwm );
  csm::daemon::EventSinkNetwork netSink( &nwm );
  
  for( int i=0; i < NUMTESTS; ++i )
  {
    std::cout << "Server loop Starting iteration " << i << std::endl;
    try {
      // get network event
      csm::daemon::CoreEvent *ev = nullptr;
      while( ! ev )
        ev = netSrc.GetEvent();

      switch( ev->GetEventType() )
      {
        case csm::daemon::EVENT_TYPE_NETWORK:
        {
          csm::daemon::NetworkEvent *nev = dynamic_cast<csm::daemon::NetworkEvent*>( ev );
          std::cout << "Server received network event: " << nev->GetContent() << std::endl;
          // just echo the event back
          int r = netSink.PostEvent( *nev );
          std::cout << "PostEvent returned: " << r << std::endl;
          break;
        }
        default:
          std::cout << "Server received non-network event type: " << csm::daemon::EventTypeToString( ev->GetEventType() ) << std::endl;
          i--;
      }

    }
    catch ( csm::network::Exception &e ) {
      std::cerr << "Server event failure " << e.what() << std::endl;
      rc = 1;
    }
  }
  int status;
  std::cout << "Server loop finished. Waiting for PID=" << aClient << " to exit." << std::endl;
  rc += ( waitpid( aClient, &status, 0 ) - aClient );

  std::cout << "Server ("<< getpid() <<") exiting after signal: " << aClient << " rc=" << rc << std::endl;
  return rc;
}

#define DATA_BUFFER_SIZE ( 1024 )

bool SendRecvSequence( csm::network::ReliableMsg &ep,
                       csm::network::MessageAndAddress &aMsgAddr )
{
  int rc = 0;

  rc = ep.SendTo( aMsgAddr );
  std::cout << "Client sent data: " << rc << std::endl;

  csm::network::MessageAndAddress reply;
  do {
    rc = ep.RecvFrom( reply );
  } while( ! rc );
  if(( rc > 0 ) && ( reply._Msg.Validate() ))
    std::cout << "Client: received. Msg: " << reply._Msg << std::endl;
  else
  {
    std::cerr << "Client: receive error. rc=" << rc << std::endl;
    if( !rc ) rc = -1;
  }

  // compare message data if we didn't get an error response
  if( ( reply._Msg.GetCommandType() != CSM_CMD_ERROR ) && ( rc > 0 ) )
  {
    std::cout << "Client input data: " << aMsgAddr._Msg.GetDataLen() << " data: " << aMsgAddr._Msg.GetData() << std::endl;
    rc = strncmp( (const char*)reply._Msg.GetData().c_str(),
                  (const char*)aMsgAddr._Msg.GetData().c_str(),
                  std::min( reply._Msg.GetDataLen(), aMsgAddr._Msg.GetDataLen() ) );
  }

  return rc == 0;
}

int client( pid_t aServer )
{
  int rc = 0;
  std::cout << "Started client " << std::endl;
  csm::network::ReliableMsg ep;
  try {
    csm::network::AddressUnix_sptr caddr = std::make_shared<csm::network::AddressUnix>( CSM_NETWORK_LOCAL_CSOCKET);
    csm::network::AddressUnix_sptr saddr = std::make_shared<csm::network::AddressUnix>( CSM_NETWORK_LOCAL_SSOCKET );
    csm::network::EndpointOptionsUnix_sptr options = std::make_shared<csm::network::EndpointOptionsUnix>( CSM_NETWORK_LOCAL_SSOCKET );
    ep.NewEndpoint( caddr, options );
    std::cout << "Client endpoint created. waiting for server signal" << std::endl;

    if( NUMTESTS > 0 )
    {
      csm::network::MessageAndAddress msgAddr;
      bool hdrvalid = msgAddr._Msg.Init( CSM_CMD_ECHO,
                               0,
                               CSM_PRIORITY_DEFAULT,
                               12345678,
                               0x01020304,
                               0x04030201,
                               geteuid(), getegid(),
                               "Hi Other World, this test should succeed." );
      msgAddr.SetAddr( saddr );
      std::cout << "Header correctness check: " << hdrvalid << std::endl;
      rc += TEST( SendRecvSequence( ep, msgAddr ),
                  true );
    }
  }
  catch (csm::network::Exception &e) {
    std::cerr << "FAILED CLIENT" << e.what() << std::endl;
  }
  std::cout << "client exit: " << aServer << " rc=" << rc << std::endl;
  ep.Clear();
  return rc;
}

int main( int argc, char **argv )
{
  // need to initialize the config instnce before initializing EventRouting...
  try {
    csm::daemon::Configuration::Instance( argc, argv );
  }

  catch (csm::daemon::ConfigurationException &e) {
    if( errno )
    {
      LOG(csmd,error) << e.what();
      std::cerr << e.what() << std::endl;
    }
    return errno;
  }
  
  int rc = 0;


  pid_t serverPid = getpid();
  pid_t clientPid = fork();

  // convenient way to switch server/client role for debugger access
  //bool ImTheServer = ( clientPid == 0 );   // if child should be server (only for debugging!)
  bool ImTheServer = ( clientPid != 0 );   // if parent should be server

  if( clientPid < 0 )
  {
    perror("Forking child failed");
    rc = clientPid;
  }
  else
  {
    if ( ImTheServer )
    {
      // if the server is the child process, the clientPid needs to be the pre-fork pid
      if( clientPid == 0 )
      {
        clientPid = serverPid;
        serverPid = getpid();
      }

      // need a server that has an event processor and event mgr to echo incoming messages
      std::cout << "Server pid: " << serverPid << " Client pid: " << clientPid << std::endl;
      rc += server( clientPid );
    }
    else
    {
      // if the client is the parent process, the server has the clientPid returned from fork
      if( serverPid == getpid() )
        serverPid = clientPid;
      // need a client that feeds network messages into a server
      rc += client( serverPid );
    }
  }

  std::cout << getpid() << " Exiting..." << std::endl;

  return rc;
}
