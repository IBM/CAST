/*================================================================================

    csmd/src/daemon/tests/csm_c_client_ack_test.cc

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
#include <csmnet/src/CPP/endpoint_dual_unix.h>
#include <csmnet/src/CPP/endpoint_multi_unix.h>
#include <csmnet/src/CPP/network_ctrl_path.h>
#include <csmnet/src/CPP/reliable_msg.h>

#define TEST_WITH_C_CLIENT
#include "csm_reliable_msg_test.h"

int server_echo( )
{
  std::cout << "Server: Starting" << std::endl;

  int rc = 0;

  csm::network::ReliableMsg MEP;
  MEP.SetDefaultTimeout( TEST_ACK_TIMEOUT );

  csm::network::EndpointMultiUnix *local = nullptr;
  csm::network::AddressUnix_sptr addr_local;
  csm::network::EndpointOptionsUnix_sptr opts_local;

  try
  {
    addr_local = std::make_shared<csm::network::AddressUnix>( CSM_NETWORK_LOCAL_SSOCKET );
    opts_local = std::make_shared<csm::network::EndpointOptionsUnix>( true );
    local = dynamic_cast<csm::network::EndpointMultiUnix*>( MEP.NewEndpoint( addr_local, opts_local ) );
    if( ! local )
      rc = 1;

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

    for( int i=0;
        (rc==0)&&(client_count > 0);
        ++i )
    {
      csm::network::Address_sptr RemoteAddr;
      if( TEST_LOOP_COUNT < TEST_LOGCOUNT_LIMIT )
        LOG(csmnet, info) << "-------------------------------------";
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
        MEP.Sync(csm::network::SYNC_ACTION_ALL );

        if( MEP.Accept( false ) )
          client_count++;
      }
      catch (csm::network::ExceptionEndpointDown &e )
      {
        LOG(csmnet, error) << "SERVER: Client disconnected.";
        client_count--;
        --i;  // disconnected clients shouldn't count
        rc = 0;
      }
      catch (csm::network::ExceptionSend &e)
      {
        LOG(csmnet, error) << "SERVER: Send error: " << e.what();
        rc = 1;
      }
      catch (csm::network::ExceptionRecv &e)
      {
        LOG(csmnet, error) << "SERVER: Recv error: " << e.what();
        rc = 1;
      }
      catch( csm::network::Exception &e)
      {
        LOG(csmnet, error) << "General Send/Recv error. " << e.what();
        rc = 1;
      }
    }
    if(buffer) free(buffer);
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
      sync_rc = MEP.Sync(csm::network::SYNC_ACTION_ALL);
    sleep( 1 );
    for( csm::network::NetworkCtrlInfo_sptr issue = MEP.GetCtrlEvent();
         issue != nullptr;
         issue = MEP.GetCtrlEvent() )
    {
      LOG( csmd, info ) << "SERVER: NetCtrlEvents type: " << issue->_Type;
    }
  } while( MEP.GetPendingACKCount() > 0 );
  sync_rc = MEP.Sync(csm::network::SYNC_ACTION_ALL );
  std::cout << "Server: Exiting" << std::endl;

  server_alive--;
  sleep(1);
  MEP.Clear();

  return rc;
}


int main(int argc, char **argv)
{
    int rc = 0;
    // set true if you want client to be parent process for debugging
    bool SeparateClient = TEST_SEPARATE_CLIENT;
    bool DebugClient = ( !SeparateClient ) & false;

    setLoggingLevel(csmd, DBG_LEVEL);
    setLoggingLevel(csmnet, DBG_LEVEL);

    pid_t clientPid = SeparateClient ? 1234 : fork();

    if( clientPid == 0 )
    {
      if( DebugClient )
        rc = server_echo();
      else
        rc = client_main_unix_c();
    }

    if( clientPid > 0 )
    {
      if(( DebugClient ) && ( ! SeparateClient ))
        rc = client_main_unix_c();
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
