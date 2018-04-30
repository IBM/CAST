/*================================================================================

    csmd/src/daemon/tests/csm_reliable_msg_test.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <atomic>
#include <thread>

#include <logging.h>
#include "csmutil/include/csm_test_utils.h"
#include <csm_network_header.h>
#include <csmnet/src/CPP/csm_network_msg_cpp.h>
#include <csmnet/src/CPP/csm_network_exception.h>
#include <csmnet/src/CPP/endpoint.h>
#include <csmnet/src/CPP/endpoint_ptp.h>
#include <csmnet/src/CPP/endpoint_dual_unix.h>
#include <csmnet/src/CPP/endpoint_multi_unix.h>
#include <csmnet/src/CPP/reliable_msg.h>

#include "csm_reliable_msg_test.h"

int server_echo( )
{
  std::cout << "Server: Starting" << std::endl;

  int rc = 0;

  csm::network::ReliableMsg MEP;
  MEP.SetDefaultTimeout( TEST_ACK_TIMEOUT );

#if UNIX_ENDPOINT_COUNT > 0
  csm::network::EndpointMultiUnix *local = nullptr;
#endif

  csm::network::AddressUnix_sptr addr_local;
  csm::network::EndpointOptionsUnix_sptr opts_local;

#if PTP_ENDPOINT_COUNT > 0
  csm::network::EndpointPTP *socket = nullptr;
#endif

  csm::network::AddressPTP_sptr addr_ptp;
  csm::network::EndpointOptionsPTP_sptr opts_ptp;

  csm::network::SSLFilesCollection ssl_files;

  try
  {
    addr_local = std::make_shared<csm::network::AddressUnix>( CSM_NETWORK_LOCAL_SSOCKET );
    opts_local = std::make_shared<csm::network::EndpointOptionsUnix>( true );

    addr_ptp = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT );
    opts_ptp = std::make_shared<csm::network::EndpointOptionsPTP>( true );

#if UNIX_ENDPOINT_COUNT > 0
    local = dynamic_cast<csm::network::EndpointMultiUnix*>( MEP.NewEndpoint( addr_local, opts_local ) );
    if( ! local )
      rc = 1;
#endif

#if PTP_ENDPOINT_COUNT > 0
    socket = dynamic_cast<csm::network::EndpointPTP*>( MEP.NewEndpoint( addr_ptp, opts_ptp ) );
    if( ! socket )
      rc = 1;
#endif

#if MQTT_ENDPOINT_COUNT > 0
    broker = dynamic_cast<csm::network::EndpointPubSub*>( MEP.NewEndpoint( addr_pubsub, opts_pubsub ) );
    if( broker == nullptr )
      rc = 1;
    else
    {
      csm::network::MessageAndAddress msg;
      srv_topic = std::make_shared<csm::network::AddressMQTopic>(
          PUBSUB_SERVER_TOPIC, csm::network::AddressMQTopic::SUBSCRIBE );
      msg.SetAddr( srv_topic );
      MEP.SendTo( msg );
    }
#endif

    if( rc != 0 )
      LOG(csmnet, error) << "SOCKET CREATION ERROR";
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error" << e.what() << std::endl;
    rc = 1;
  }


  if( ! rc ) {

    char *buffer = (char*)malloc( DATA_BUFFER_SIZE );
    bzero( buffer, DATA_BUFFER_SIZE );

    if( TEST_SEPARATE_CLIENT )
      client_count = CLIENT_THREAD_COUNT;

    bool keepRunning = true;
    int pendingActivity = 0;
    while( keepRunning )
    {
      csm::network::Address_sptr RemoteAddr;
      {
        csm::network::Endpoint *clt = nullptr;
        clt = MEP.Accept( false );
        if( clt != nullptr )
          LOG(csmnet, info) << "SERVER: ACCEPTED EP: " << clt->GetLocalAddr()->Dump() << "->" << clt->GetRemoteAddr()->Dump();
      }

      try
      {
        size_t rlen = 0;
        csm::network::MessageAndAddress msgAddr;
        rlen = MEP.RecvFrom( msgAddr );

        if( rlen > 0 )
        {
          RemoteAddr = msgAddr.GetAddr();
          if( TEST_LOOP_COUNT < TEST_LOGCOUNT_LIMIT )
            LOG(csmnet, info) << "Server: Received " << rlen << " bytes, clnt_add: " << RemoteAddr->Dump();

          // turn into reply
          create_response( msgAddr );

          if( MEP.SendTo( msgAddr ) == 0 )
          {
            LOG(csmnet, info) << "server_echo(): Warning: Sent returned 0.";
            rc = 1;
          }
        }

        pendingActivity = MEP.Sync( csm::network::SYNC_ACTION_MAINTENANCE );
        if(( pendingActivity ) || ( MEP.CtrlEventCount() ))
        {
          LOG( csmnet, debug ) << "server has " << MEP.CtrlEventCount() << " CtrlEvents pending.";
          csm::network::NetworkCtrlInfo_sptr info = MEP.GetCtrlEvent();
          while( info != nullptr )
          {
            LOG( csmnet, info ) << "Reaping CtrlEvent type=" << info->_Type;
            switch( info->_Type )
            {
              case csm::network::NET_CTL_TIMEOUT:
                break;
              case csm::network::NET_CTL_DISCONNECT:
                client_count--;
                LOG(csmnet, error) << "SERVER: UNIX Client disconnected. Remaining clients: " << client_count;
                break;
              case csm::network::NET_CTL_OTHER:
              default:
                throw csm::network::Exception("Unrecognized network ctrl event.");
                break;
            }
            info = MEP.GetCtrlEvent();
          }
        }
      }
      catch (csm::network::ExceptionEndpointDown &e )
      {
        client_count--;
        LOG(csmnet, error) << "SERVER: Client disconnected. Remaining clients: " << client_count;
        rc = 0;
      }
      catch (csm::network::Exception &e)
      {
        LOG(csmnet, error) << "SERVER: Send/Recv error: " << e.what();
        rc = 1;
      }
      keepRunning = ((( rc == 0 ) && ( client_count > 0 )) || ( pendingActivity > 0 ));
    }
    free( buffer );
  }
  else {
    perror("Server: Binding");
  }
  int sync_rc = 1;
  LOG( csmnet, info ) << "SERVER: end main loop with pending ACKs=" << MEP.GetPendingACKCount();
  do
  {
    sync_rc = 1;
    while( sync_rc != 0 )
      sync_rc = MEP.Sync(csm::network::SYNC_ACTION_ALL );
    sleep( 1 );
  } while( MEP.GetPendingACKCount() > 0 );
  std::cout << "Server: Exiting" << std::endl;

  server_alive--;
  sleep(1);
  for( csm::network::NetworkCtrlInfo_sptr issue = MEP.GetCtrlEvent();
       issue != nullptr;
       issue = MEP.GetCtrlEvent() )
  {
    LOG( csmd, info ) << "SERVER: NetCtrlEvents type: " << issue->_Type;
  }
  MEP.Clear();

  return rc;
}


int main(int argc, char **argv)
{
    int rc = 0;
    // set true if you want client to be parent process for debugging
    bool SeparateClient = TEST_SEPARATE_CLIENT;
    bool DebugClient = ( !SeparateClient ) | false;

    setLoggingLevel(csmd, DBG_LEVEL);
    setLoggingLevel(csmnet, DBG_LEVEL);

    pid_t clientPid = SeparateClient ? 1234 : fork();

    if( clientPid == 0 )
    {
      if( DebugClient )
        rc = server_echo();
      else
        rc = client_test();
    }

    if( clientPid > 0 )
    {
      if(( DebugClient ) && ( ! SeparateClient ))
        rc = client_test();
      else
        rc = server_echo();

      int status;
      waitpid( clientPid, &status, 0 );
    }

    if( clientPid < 0 )
    {
      perror("Forking child failed");
      rc = clientPid;
    }
    std::cout << "Test exiting: rc=" << rc << std::endl;
    return rc;
}
