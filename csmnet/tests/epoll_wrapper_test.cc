/*================================================================================

    csmnet/tests/epoll_wrapper_test.cc

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <memory>

#include <logging.h>
#include "csmutil/include/csm_test_utils.h"
#include <csm_network_header.h>
#include <CPP/csm_network_msg_cpp.h>
#include <CPP/csm_network_exception.h>
#include <CPP/endpoint.h>
#include <CPP/endpoint_ptp.h>
#include <CPP/endpoint_dual_unix.h>
#include <CPP/endpoint_multi_unix.h>
#include <CPP/multi_endpoint.h>


#define CLIENT_THREAD_COUNT ( 100 )
#define CLIENT_CONN_PER_THREAD ( 5 )
#define CLIENT_SEND_PER_ITER ( 50 )

#define TEST_LOOP_COUNT ( 200 )
#define TEST_LOOP_DELAY ( 0 )   // arg for usleep
#define TEST_RUN_TIME ( 60 )


#define DATA_BUFFER_SIZE 8192

#define TEST_INITIAL_PORT (0x5E5E)

std::atomic_int client_count( CLIENT_THREAD_COUNT );
std::atomic_int server_alive( 1 );

typedef std::chrono::time_point< std::chrono::system_clock > TimeType;


void* server_echo( void * retcode)
{
  std::cout << "Server: Starting" << std::endl;

  int rc = 0;
  ++server_alive;
  client_count = 0;

  csm::network::EpollWrapper epp( EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLPRI );
  csm::network::EpollWrapper epa( EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLPRI );

  csm::network::AddressPTP_sptr addr = std::make_shared<csm::network::AddressPTP>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsPTP_sptr opts_ptp = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( true );

  csm::network::EndpointCompute_plain ep( addr, opts_ptp );

  epp.Add( &ep );

  uint32_t events = 0;

  TimeType end = std::chrono::system_clock::now() + std::chrono::seconds( TEST_RUN_TIME );
  csm::network::MessageAndAddress msgAddr;
  csm::network::Endpoint *active = nullptr;

  LOG( csmnet, always ) << "Starting Testloop.";
  while( std::chrono::system_clock::now() < end )
  {
    while( nullptr != ( active = epp.NextActiveEP( false, &events )) )
    {
      if( active != nullptr )
      {
        LOG( csmnet, always ) << "Passive Event. cl=" << client_count;
        csm::network::Endpoint *cltep = active->Accept();
        if( cltep != nullptr )
        {
          epa.Add( cltep );
          ++client_count;
          LOG( csmnet, always ) << "Clients new:  " << client_count;
        }
        epp.AckEvent();
      }
    }

    // artificial processing delay, to accumulate many events
    while( nullptr != ( active = epa.NextActiveEP( false, &events )) )
    {
      if( events & (EPOLLRDHUP | EPOLLERR) )
      {
        LOG( csmnet, always ) << "Client Disconnected/error:" << active->GetRemoteAddr()->Dump();
        epa.Del( active );
        --client_count;
        LOG( csmnet, always ) << "Clients remaining:  " << client_count;
        delete active;
      }
      else if( events & EPOLLIN )
      {
        LOG( csmnet, always ) << "Client Data:" << active->GetRemoteAddr()->Dump();
        active->RecvFrom( msgAddr );
        if( msgAddr._Msg.GetCommandType() == CSM_CMD_STATUS )
          active->SetVerified();
      }
      epa.AckEvent();
    }
  }

  LOG( csmnet, always ) << "ALL CLIENTS GONE.";

  epp.Del( &ep );


  *(int*)retcode = rc;
  return retcode;
}

void* client_main( void * retcode)
{
  sleep(0);
  std::cout << "Starting client" << std::endl;
  int rc = 0;

  csm::network::AddressPTP_sptr addr = std::make_shared<csm::network::AddressPTP>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsPTP_sptr opts_ptp = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( addr.get() );
  csm::network::EndpointCompute_plain *ep[ CLIENT_CONN_PER_THREAD ];

  csm::network::Message msg, rmsg;
  msg.Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT, random(), 0, 0, geteuid(), getegid(), "HelloWorld", 0 );

  TimeType end = std::chrono::system_clock::now() + std::chrono::seconds( TEST_RUN_TIME-2 );
  int i,n;
  while(( server_alive.load() > 0 ) &&
        ( std::chrono::system_clock::now() < end ))
  {
    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
      ep[i] = new csm::network::EndpointCompute_plain( addr, opts_ptp );

    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
    {
      rc += TESTFAIL( ep[i], nullptr );
      rc += TEST( ep[i]->IsConnected(), true );
    }
    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
      for( n=0; n<CLIENT_SEND_PER_ITER; ++n )
        rc += TESTFAIL( ep[i]->Send( msg ), 0 );
    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
      for( n=0; n<CLIENT_SEND_PER_ITER; ++n )
        ep[i]->Recv( rmsg );
    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
      delete ep[i];
  }

  LOG( csmnet, always ) << "SERVER EXITED.";

  *(int*)retcode = rc;
  return retcode;
}

namespace pt = boost::property_tree;


int main(int argc, char **argv)
{
    int ret = 0;
    int crc[CLIENT_THREAD_COUNT];

    std::string conffile("csm_master.conf");
    if( argc > 1 )
      conffile = argv[ 1 ];
    pt::ptree conftree;
    pt::read_json(conffile, conftree);
    initializeLogging( std::string( "csm.log" ), conftree );

    int src = 0;
    std::thread server( server_echo, &src );

    std::thread *t[ CLIENT_THREAD_COUNT ];
    bzero( crc, sizeof( int ) * CLIENT_THREAD_COUNT );

    for( int i=0; i<CLIENT_THREAD_COUNT; ++i)
    {
      t[i] = new std::thread( client_main, &crc[i] );
    }

    for( int i=0; i<CLIENT_THREAD_COUNT; ++i)
    {
      t[i]->join();
      ret += crc[i];
    }
    server.join();
    ret += src;

    std::cout << "Test exiting: rc=" << ret << std::endl;
    return ret;
}
