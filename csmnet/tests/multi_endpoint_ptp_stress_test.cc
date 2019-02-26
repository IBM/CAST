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
#include <CPP/endpoint_aggregator.h>
#include <CPP/endpoint_utility.h>
#include <CPP/endpoint_dual_unix.h>
#include <CPP/endpoint_multi_unix.h>

#include "csmutil_logging.h"
#include "csmi/include/csm_api_common.h"
#include "csmi/src/common/include/csmi_common_utils.h"
#include "csmi/src/common/include/csmi_api_internal.h"

#include "csmd/src/daemon/include/csm_api_acl.h"
#include "csmd/src/daemon/include/csm_api_config.h"

//#define MULTI_ENDPOINT_TEST

#ifdef MULTI_ENDPOINT_TEST
#include <CPP/multi_endpoint.h>
#else
#include <CPP/reliable_msg.h>
#endif

//#define WITH_PASSIVE_SOCKET_ERROR

#define CLIENT_THREAD_COUNT ( 2 )
#define CLIENT_CONN_PER_THREAD ( 10 )
#define CLIENT_SEND_PER_ITER ( 10 )

//#define CLIENT_ONLY
#define LOCAL_COUNT ( 1 )
#define COMPUTE_ADDR
//#define AGGREGATOR_ADDR
//#define UTILITY_ADDR

#define TEST_LOOP_COUNT ( 200 )
#define TEST_LOOP_DELAY ( 0 )   // arg for usleep
#define TEST_RUN_TIME ( 100 )


#define DATA_BUFFER_SIZE 8192

#define TEST_INITIAL_PORT (0x5E5E)

std::atomic_int client_count( CLIENT_THREAD_COUNT );
std::atomic_int server_alive( 1 );
volatile uint64_t disconnect_events( 0 );
volatile uint64_t connect_events( 0 );
volatile uint64_t unverified_events( 0 );

typedef std::chrono::time_point< std::chrono::system_clock > TimeType;

static inline
int process_ctl_events( csm::network::MultiEndpoint &MEP )
{
  csm::network::NetworkCtrlInfo_sptr ctlev;
  while( (ctlev = MEP.GetCtrlEvent()) != nullptr )
  {
    switch( ctlev->_Type )
    {
      case csm::network::NetworkCtrlEventType::NET_CTL_DISCONNECT:
        ++disconnect_events;
        --client_count;
        LOG( csmnet, always ) << "DISCONNECT: Clients remaining:" << client_count;
        break;
      case csm::network::NetworkCtrlEventType::NET_CTL_UNVERIFIED:
        ++unverified_events;
        ++client_count;
        break;
      case csm::network::NetworkCtrlEventType::NET_CTL_CONNECT:
        ++connect_events;
        LOG( csmnet, always ) << "CONNECT: Clients new:      " << client_count;
        break;
      case csm::network::NetworkCtrlEventType::NET_CTL_FATAL:
        --client_count;
        LOG( csmnet, always ) << "FATAL: Clients remaining:" << client_count;
        break;
      default:
        LOG( csmnet, always ) << "CtlEvent: " << ctlev->_Type;
        break;
    }
    LOG( csmnet, always ) << "u|c|d[" << unverified_events << "|" << connect_events << "|" << disconnect_events << "]";
  }
  return 0;
}


void* server_echo( void * retcode,
                   csm::daemon::CSMIAuthList_sptr auth,
                   csm::daemon::CSMAPIConfig_sptr apiconf )
{
  std::cout << "Server: Starting" << std::endl;

  int rc = 0;
  ++server_alive;
  client_count = 0;

#ifdef MULTI_ENDPOINT_TEST
  csm::network::MultiEndpoint MEP;
#else
  csm::network::ReliableMsg MEP;
#endif

#ifdef COMPUTE_ADDR
  csm::network::AddressPTP_sptr addr = std::make_shared<csm::network::AddressPTP>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsPTP_sptr opts_ptp = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( true );
  csm::network::EndpointCompute_plain *listener = new csm::network::EndpointCompute_plain( addr, opts_ptp );
#endif
#ifdef AGGREGATOR_ADDR
  csm::network::AddressAggregator_sptr addr = std::make_shared<csm::network::AddressAggregator>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsAggregator_sptr opts_ptp =
      std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressAggregator>>( true );
  csm::network::EndpointAggregator_plain *listener = new csm::network::EndpointAggregator_plain( addr, opts_ptp );
#endif
#ifdef UTILITY_ADDR
  csm::network::AddressUtility_sptr addr = std::make_shared<csm::network::AddressUtility>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsUtility_sptr opts_ptp =
      std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressUtility>>( true );
  csm::network::EndpointUtility_plain *listener = new csm::network::EndpointUtility_plain( addr, opts_ptp );
#endif

  csm::network::AddressUnix_sptr addr_local = std::make_shared<csm::network::AddressUnix>( "/tmp/mep_testsock" );
  csm::network::EndpointOptionsUnix_sptr opts_local = std::make_shared<csm::network::EndpointOptionsUnix>( true, auth, apiconf, 0777, "users" );
  csm::network::EndpointMultiUnix *local = new csm::network::EndpointMultiUnix( addr_local, opts_local );

  MEP.NewEndpoint( local );
  MEP.NewEndpoint( listener );

  TimeType end = std::chrono::system_clock::now() + std::chrono::seconds( TEST_RUN_TIME );
  csm::network::MessageAndAddress msgAddr;
  ssize_t rsize = 0;

  LOG( csmnet, always ) << "Starting Testloop.";
  do
  {
    try{  MEP.Accept( false ); }
    catch( csm::network::Exception &e ) { LOG( csmnet, always ) << "SV: Accept error" << e.what(); }

    /* data synchronization, e.g. flush any buffers */
    try{ MEP.Sync( csm::network::SyncAction::SYNC_ACTION_MAINTENANCE ); }
    catch ( csm::network::Exception &e ) { LOG( csmnet, always ) << "SV: Sync error: " << e.what(); }
    rc += process_ctl_events( MEP );

    try{ rsize = MEP.RecvFrom( msgAddr ); }
    catch( csm::network::Exception &e ) { LOG( csmnet, always ) << "SV: Recv error" << e.what(); rsize = 0;}

    if( rsize < 0 )
    {
      LOG( csmnet, always ) << "Recv Error: " << rsize;
      rc += 1;
    }
    if( rsize>0 )
    {
      switch ( msgAddr._Msg.GetCommandType() )
      {
        case CSM_CMD_STATUS:
#ifdef MULTI_ENDPOINT_TEST
          MEP.GetEndpoint( msgAddr.GetAddr().get() )->SetVerified();
          ++connect_events;
#endif
          break;
        case CSM_CMD_ECHO:
        {
//          LOG( csmnet, always ) << "Received a msg. Responding now." << csm::network::cmd_to_string( msgAddr._Msg.GetCommandType() );
          msgAddr._Msg.SetResp();
          msgAddr._Msg.CheckSumUpdate();
          try { MEP.SendTo( msgAddr ); }
          catch( csm::network::Exception &e ) { LOG( csmnet, always ) << "SV: Endpoint already down: " << e.what(); }
//          LOG( csmnet, always ) << "Received a msg. Finishing?." << csm::network::cmd_to_string( msgAddr._Msg.GetCommandType() );
          break;
        }
        default:
          break;
      }
    }

    /* data synchronization, e.g. flush any buffers */
    try{ MEP.Sync( csm::network::SyncAction::SYNC_ACTION_MAINTENANCE ); }
    catch ( csm::network::Exception &e ) { LOG( csmnet, always ) << "SV: Sync error: " << e.what(); }
    rc += process_ctl_events( MEP );

#ifdef WITH_PASSIVE_SOCKET_ERROR
    // force error in random situation
    if(( random() % 1000 == 0 ) && ( client_count > 0 ))
    {
      LOG( csmnet, always ) << "Deleting listener to inject error.";
      MEP.DeleteEndpoint( listener, "Main:Server:Inject" );
      MEP.DeleteEndpoint( local, "Main:Server:Inject" );
      listener = nullptr;
      break;
    }
#endif
  }
  while(( std::chrono::system_clock::now() < end ) && ( rc == 0 ));

  LOG( csmnet, always ) << "SV: LISTENER SHUTDOWN u|c|d["
    << unverified_events << "|" << connect_events << "|" << disconnect_events << "] rc=" << rc;
  if( listener )
    MEP.DeleteEndpoint( listener, "Main:Server" );
  if( local )
    MEP.DeleteEndpoint( local, "Main:Server" );

  if( client_count > 0 )
    LOG( csmnet, always ) << "SV: Waiting for " << client_count << " remaining connections to time out.";

  while( client_count > 0 )
  {
    /* data synchronization, e.g. flush any buffers */
    try{ MEP.Sync( csm::network::SyncAction::SYNC_ACTION_MAINTENANCE ); }
    catch ( csm::network::Exception &e ) { LOG( csmnet, always ) << "SV: Sync error: " << e.what(); }
    rc += process_ctl_events( MEP );

    try{ rsize = MEP.RecvFrom( msgAddr ); }
    catch( csm::network::Exception &e ) { LOG( csmnet, always ) << "SV: Recv error" << e.what(); rsize = 0;}
  }

  rc += TEST( disconnect_events, unverified_events );
  LOG( csmnet, always ) << "ALL CLIENTS GONE. u|c|d["
      << unverified_events << "|" << connect_events << "|" << disconnect_events << "] rc=" << rc;

  *(int*)retcode = rc;
  return retcode;
}

void* client_main( void * retcode)
{
  sleep(1);
  int id = *(int*)retcode;
  LOG( csmnet, info ) << id << "CL:Starting clientid" << std::endl;
  int rc = 0;
  *(int*)retcode = rc;

#ifdef COMPUTE_ADDR
  csm::network::AddressPTP_sptr addr = std::make_shared<csm::network::AddressPTP>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsPTP_sptr opts_ptp =
      std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( addr.get() );
#endif
#ifdef AGGREGATOR_ADDR
  csm::network::AddressAggregator_sptr addr = std::make_shared<csm::network::AddressAggregator>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsAggregator_sptr opts_ptp =
      std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressAggregator>>( addr.get() );
#endif
#ifdef UTILITY_ADDR
  csm::network::AddressUtility_sptr addr = std::make_shared<csm::network::AddressUtility>( 0x7F000001, TEST_INITIAL_PORT );
  csm::network::EndpointOptionsUtility_sptr opts_ptp =
      std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressUtility>>( addr.get() );
#endif

  csm::network::Endpoint *ep[ CLIENT_CONN_PER_THREAD ];
  memset( ep, 0, sizeof( csm::network::Endpoint* ) * CLIENT_CONN_PER_THREAD );
  std::deque<csm::network::Endpoint*> deadqueue;

  csm::network::Message msg, rmsg;
  msg.Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT, random(), 0, 0, geteuid(), getegid(), "HelloWorld", 0 );

  TimeType end = std::chrono::system_clock::now() + std::chrono::seconds( TEST_RUN_TIME-2 );
  int i,n;
  ssize_t rsize;
  while(( server_alive.load() > 0 ) &&
        ( std::chrono::system_clock::now() < end ) &&
        ( rc == 0 ))
  {
//    LOG( csmnet, always ) << "CLLLOOOOOP";

    int testcase = random() % 4;

    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
      while(( ep[i] == nullptr ) && ( std::chrono::system_clock::now() < end ))
      {
        try {
#ifdef COMPUTE_ADDR
          ep[i] = new csm::network::EndpointCompute_plain( addr, opts_ptp );
#endif
#ifdef AGGREGATOR_ADDR
          ep[i] = new csm::network::EndpointAggregator_plain( addr, opts_ptp );
#endif
#ifdef UTILITY_ADDR
          ep[i] = new csm::network::EndpointUtility_plain( addr, opts_ptp );
#endif
          LOG( csmnet, info ) << id << "Client created: " << i << "; " <<  ep[i]->GetLocalAddr()->Dump();
        }
        catch ( csm::network::Exception &e ) { LOG( csmnet, error ) << e.what(); sleep( 1 ); }
      }
    if( std::chrono::system_clock::now() >= end )
      break;

    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
    {
      rc += TESTFAIL( ep[i], nullptr );
      if( ep[i] != nullptr )
        rc += TEST( ep[i]->IsConnected(), true );
    }
    // if we had an error, make sure we skip the send/recv
    if( rc != 0 )
    {
      ++rc;
      LOG( csmnet, error ) << id << "CL: connection error rc=" << rc;
      continue;
    }

    if( --testcase > 0 )
      for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
        for( n=0; n<CLIENT_SEND_PER_ITER; ++n )
        {
          try { ep[i]->Send( msg ); }
          catch ( csm::network::Exception &e ) { LOG( csmnet, always ) << e.what(); ++rc; }
        }
//    else
//      LOG( csmnet, always ) << "CL:Early exit case 1: no send/recv";

    if( --testcase > 0 )
      for( i=0; (i<CLIENT_CONN_PER_THREAD); ++i ) // build in a breaker that causes eary deletion of sockets
        for( n=0; n<CLIENT_SEND_PER_ITER; ++n )
        {
          rsize = 0;
          while(( rsize == 0 ) && ( std::chrono::system_clock::now() < end ))
          {
            try{ rsize = ep[i]->Recv( rmsg ); }
            catch( csm::network::Exception &e ) { LOG( csmnet, error ) << e.what(); break; }
          }
//          if( rsize == 0 )
//          {
//            LOG( csmnet, error ) << id << "Recv timed out";
//            break;
//          }
        }
//    else
//      LOG( csmnet, always ) << "CL:Early exit case 2: no recv";

    for( i=0; i<CLIENT_CONN_PER_THREAD; ++i )
    {
      if( ep[i] )
      {
        if(( random() % 10000 ) != 0 )
        {
          LOG( csmnet, info ) << id << "Client deleting: " << i << "; " << ep[i]->GetLocalAddr()->Dump();
          delete ep[i];
        }
        else
        {
          LOG( csmnet, info ) << id << "Client creating dead connection: " << i << "; " << ep[i]->GetLocalAddr()->Dump();
          deadqueue.push_back( ep[i] );
        }
      }
      ep[i] = nullptr;
    }
  }

  LOG( csmnet, always ) << id << "CL:SERVER EXITED. rc=" << rc;

  // sleep-loop because a single sleep might be interrupted by signals of failing
  int delay = 60;
  LOG( csmnet, always ) << id << "CL: Waiting " << delay << "s for more dead connections to fail.";
  end = std::chrono::system_clock::now() + std::chrono::seconds( delay );
  while(( !deadqueue.empty() ) &&
        ( std::chrono::system_clock::now() < end ))
  {
    sleep( 1 );
  }
  // kill the dead connection that were forcefully left over
  while( ! deadqueue.empty() )
  {
    csm::network::Endpoint *dep = deadqueue.front();
    deadqueue.pop_front();
    try
    {
      // why can I still send here....
      dep->Send( msg );
      dep->Send( msg );
      LOG( csmnet, always ) << id << "CL:Forced dead connection still send/recv-able. addr=" << dep->GetLocalAddr()->Dump();
    }
    catch ( csm::network::Exception &e )
    {
      LOG( csmnet, always ) << id << "CL:Forced dead connection is really dead ;-): " << e.what()
          << " addr=" << dep->GetLocalAddr()->Dump();
    }
    delete dep;
  }


  LOG( csmnet, always ) << id << "CL: Exit with rc=" << rc;
  *(int*)retcode = rc;
  return retcode;
}


int general_request_response_test( const int expected,
                                   const std::string &payload,
                                   char **recvData, uint32_t *recvDataLen )
{
  int rc = 0;

  csm_api_object* csm_obj = csm_api_object_new(CSM_error_inject, NULL);

  *recvData = nullptr;
  *recvDataLen = 0;

  int err = csmi_sendrecv_cmd(csm_obj, CSM_CMD_ECHO, payload.c_str(), payload.length(),
                              recvData, recvDataLen);

  if(err != expected)
  {
    std::cerr << "ERROR: errcode = " << csm_api_object_errcode_get(csm_obj) <<
        " errmsg = " << csm_api_object_errmsg_get(csm_obj) << std::endl;
    rc++;
  }

  csm_api_object_destroy(csm_obj);
  return rc;
}



void* client_unix( void * retcode)
{
  char *warning = strdup( "warning" );
  csmutil_logging_level_set( warning );
  free(warning);

  sleep(1);
  int id = *(int*)retcode;
  char *recvData = nullptr;

  LOG( csmnet, info ) << id << "CL:Starting clientid" << std::endl;
  int rc = 0;
  *(int*)retcode = rc;

  TimeType end = std::chrono::system_clock::now() + std::chrono::seconds( TEST_RUN_TIME-2 );

  while(( server_alive.load() > 0 ) &&
        ( std::chrono::system_clock::now() < end ) &&
        ( rc == 0 ))
  {
    if (csm_init_lib() != 0)
    {
      LOG( csmnet, error ) << id << "CL:ERROR: csm_init_lib() failed." << std::endl;
      ++rc;
      goto exit;
    }

    while( random() % 100 != 0 )
    {
      LOG( csmnet, trace ) << id << "CL:Sending request";
      uint32_t recvDataLen;
      rc = general_request_response_test( 0, std::string("HelloWorld"), &recvData, &recvDataLen );
      LOG( csmnet, trace ) << id << "CL:Received response rc=" << rc;
      free( recvData );
    }

    if (csm_term_lib() != 0)
    {
      LOG( csmnet, error ) << id << "CL:ERROR: csm_term_lib() failed." << std::endl;
      ++rc;
    }
  }

exit:
  LOG( csmnet, always ) << id << "CL: Exit with rc=" << rc;
  *(int*)retcode = rc;
  return retcode;
}

namespace pt = boost::property_tree;

int main(int argc, char **argv)
{
    int ret = 0;
    int crc[CLIENT_THREAD_COUNT * 2];

    std::string conffile("csm_master.conf");
    if( argc > 1 )
      conffile = argv[ 1 ];
    pt::ptree conftree;
    pt::read_json(conffile, conftree);
    initializeLogging( std::string( "csm.log" ), conftree );

    std::string authfile = conftree.get("csm.api_permission_file", "nofile");
    std::string apiconffile = conftree.get("csm.api_configuration_file", "nofile");
    csm::daemon::CSMIAuthList_sptr auth = std::make_shared<csm::daemon::CSMIAuthList>( authfile );
    csm::daemon::CSMAPIConfig_sptr apiconf = std::make_shared<csm::daemon::CSMAPIConfig>( apiconffile );

    int src = 0;
    std::thread server( server_echo, &src, auth, apiconf );

    std::thread *t[ CLIENT_THREAD_COUNT * 2 ];
    bzero( crc, sizeof( int ) * (CLIENT_THREAD_COUNT * 2 ));

#ifndef CLIENT_ONLY
    for( int i=0; i<CLIENT_THREAD_COUNT; ++i)
    {
      crc[i] = i+1;
      t[i] = new std::thread( client_main, &crc[i] );
    }
#endif

    for( int i=0; i<LOCAL_COUNT; ++i )
    {
      crc[CLIENT_THREAD_COUNT + i ] = CLIENT_THREAD_COUNT * 2 + i;
      t[CLIENT_THREAD_COUNT + i] = new std::thread( client_unix, &crc[CLIENT_THREAD_COUNT + i]);
    }

#ifndef CLIENT_ONLY
    for( int i=0; i<CLIENT_THREAD_COUNT; ++i)
    {
      t[i]->join();
      ret += crc[i];
    }
#endif

    for( int i=0; i<LOCAL_COUNT; ++i )
    {
      t[ CLIENT_THREAD_COUNT + i]->join();
      ret += crc[ CLIENT_THREAD_COUNT + i];
    }

    server.join();
    ret += src;

    std::cout << "Test exiting: rc=" << ret << std::endl;
    return ret;
}
