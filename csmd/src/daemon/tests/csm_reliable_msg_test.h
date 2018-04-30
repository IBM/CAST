/*================================================================================

    csmd/src/daemon/tests/csm_reliable_msg_test.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_TESTS_CSM_RELIABLE_MSG_TEST_H_
#define CSMD_SRC_DAEMON_TESTS_CSM_RELIABLE_MSG_TEST_H_

// true if the client code should be in separate program for better debugging
#define TEST_SEPARATE_CLIENT ( false )

// number of endpoints of each type
#define UNIX_ENDPOINT_COUNT ( 2 )
#define PTP_ENDPOINT_COUNT  ( 2 )
#define MQTT_ENDPOINT_COUNT ( 0 )  // can only be 1 at the moment
#define CLIENT_THREAD_COUNT ( PTP_ENDPOINT_COUNT + MQTT_ENDPOINT_COUNT + UNIX_ENDPOINT_COUNT )

// number of test loops per client
#define TEST_LOOP_COUNT ( 100 )

// limit when to stop logging recv/send
#define TEST_LOGCOUNT_LIMIT ( 10 )

// allow a delay between iterations
#define TEST_LOOP_DELAY ( 0 )   // arg for usleep [micro seconds]

// testing timeouts, last msg will be delayed
#define TEST_ACK_TIMEOUT ( 2 )  // ACK timeout [seconds]

#define DATA_BUFFER_SIZE 8192

#define TEST_INITIAL_PORT 32420

#define DBG_LEVEL info

std::atomic_int client_count( CLIENT_THREAD_COUNT );
std::atomic_int server_alive( 1 );

#include <algorithm>

int create_response( csm::network::MessageAndAddress &msgAddr )
{
  msgAddr._Msg.SetResp();
  msgAddr._Msg.CheckSumUpdate();

  return 0;
}

namespace csm {
namespace network {

class MultiEndpointTest
{
public:
  int TestNullptrEPEvent()
  {
    int rc = 0;

    MultiEndpoint *MEP = nullptr;
    AddressPTP addr(0x0100007F, 3252 );
    try {
      MEP = new MultiEndpoint();

      AddressCode code = addr.MakeKey();

      MEP->_EPL[ code ] = nullptr;
    }
    catch( std::exception &e )
    {
      return 1;
    }

    // try deletion and expect exception because stored endpoint is nullptr
    try {
      MEP->DeleteEndpoint( &addr );
    }
    catch( csm::network::Exception &e )
    {
      // successful as expected
    }
    catch( std::exception &e )
    {
      rc += 1;
    }

    // retry deletion and expect normal operation
    try {
      MEP->DeleteEndpoint( &addr );
      delete MEP;
    }
    catch( std::exception &e )
    {
      rc += 1;
    }

    return rc;
  }
};

}  // namespace daemon
} // namespace csm

#ifndef TEST_WITH_C_CLIENT

int ClientFinish( csm::network::ReliableMsg &io_MEP, const std::string i_Client )
{
  int rc = 0;
  std::vector<uint64_t> msgList;
  int sync_rc = 1;
  LOG( csmnet, info ) << i_Client << "Client: end main loop with pending ACKs=" << io_MEP.GetPendingACKCount();
  do
  {
    sync_rc = 1;
    msgList.clear();
    while( sync_rc != 0 )
    {
      sync_rc = io_MEP.Sync(csm::network::SYNC_ACTION_ALL );
      sleep( 1 );
    }
  } while( io_MEP.GetPendingACKCount() > 0 );
  std::cout << "Exiting "<< i_Client << " client rc=" << rc << std::endl;

  sleep( std::max( UNIX_ENDPOINT_COUNT, PTP_ENDPOINT_COUNT ) );
  io_MEP.Clear();
  return rc;
}

int msg_memcmp( csm::network::Message &A, csm::network::Message &B )
{
  if( !( A.Validate() && B.Validate() ) )          return 1;
  if( A.GetCommandType() != B.GetCommandType() )   return 2;
  if( A.GetFlags()       != B.GetFlags() )         return 3;
  if( A.GetPriority()    != B.GetPriority() )      return 4;
  if( A.GetMessageID()   != B.GetMessageID() )     return 5;
  if( A.GetDataLen()     != B.GetDataLen() )       return 6;
  if( A.GetCheckSum()    != B.GetCheckSum() )      return 7;
  if( A.GetSrcAddr()     != B.GetSrcAddr() )       return 8;
  if( A.GetDstAddr()     != B.GetDstAddr() )       return 9;
  if( A.GetData()        != B.GetData() )          return 10;
  return 0;
}

int client_send_recv_test( csm::network::ReliableMsg *socket,
                           csm::network::Address_sptr addr,
                           const uint64_t clientcode,
                           const uint32_t MsgId,
                           const uint32_t recvDelay = 0 )
{
  int rc = 0;
  csm::network::MessageAndAddress msgAddr, in;

  rc = msgAddr._Msg.Init( CSM_CMD_ECHO,
                 0,
                 CSM_PRIORITY_DEFAULT,
                 MsgId,
                 0x1234,
                 0x1234,
                 geteuid(),
                 getegid(),
//                 socket->GetLocalAddr()->MakeKey(),
//                 socket->GetRemoteAddr()->MakeKey(),
                 "Hello World"+std::to_string( clientcode ));
  msgAddr.SetAddr( addr );

  if(( rc )&&(server_alive.load()>0)) {
    try
    {
      rc = socket->SendTo( msgAddr );
      if( TEST_LOOP_COUNT < TEST_LOGCOUNT_LIMIT )
        LOG(csmnet, info) << "client_test(): Send returned: " << rc;
      rc *= (rc < 0);  // keep status if < 0

      // turn into reply (to allow comparison with server response)
      create_response( msgAddr );

      if( recvDelay )
        sleep( recvDelay );
      if( TEST_LOOP_COUNT < TEST_LOGCOUNT_LIMIT )
        LOG(csmnet, debug) << "client_test(): entering recv: " << rc;
      while((!rc)&&(server_alive.load()>0))
      {
        rc = socket->RecvFrom( in );
      }
      if( TEST_LOOP_COUNT < TEST_LOGCOUNT_LIMIT )
        LOG(csmnet, info) << "client_test(): recv finished: " << rc;

      std::cout << "." << std::flush;
    }
    catch ( csm::network::Exception &e)
    {
      LOG(csmnet, error) << "client_test(): " << e.what();
      if( ! rc ) rc = errno;
    }

    if( (rc = msg_memcmp( msgAddr._Msg, in._Msg)) != 0 )
    {
      LOG(csmnet, error) << "client_test(): Content Mismatch: rc=" << rc;
      rc = 1;
    }
  }
  else {
    perror("Client: Connection failed");
  }
  return rc;
}


void* client_main_unix( void * retcode)
{
  setLoggingLevel(csmd, DBG_LEVEL);
  setLoggingLevel(csmnet, DBG_LEVEL );

  uint32_t initialMsgID = *(uint32_t*)retcode;
  int rc = 0;

  std::hash<std::thread::id> hashing;
  uint64_t threadHash = hashing( std::this_thread::get_id() );
  srand( (unsigned int)( threadHash ) );
  std::cout << "Starting UNIX client (" << threadHash << ":" << initialMsgID << ")" << std::endl;

  csm::network::ReliableMsg MEP;

  csm::network::EndpointMultiUnix *socket = nullptr;
  csm::network::AddressUnix_sptr saddr;
  csm::network::AddressUnix_sptr caddr;
  csm::network::EndpointOptionsUnix_sptr opts;
//  sleep(500);
  try {
    saddr = std::make_shared<csm::network::AddressUnix>( CSM_NETWORK_LOCAL_SSOCKET );
    std::hash<std::thread::id> hashing;
    std::string path = std::string( CSM_NETWORK_LOCAL_CSOCKET ) + std::to_string( getpid() +  hashing(std::this_thread::get_id()) );
    caddr = std::make_shared<csm::network::AddressUnix>( path.c_str() );
    opts = std::make_shared<csm::network::EndpointOptionsUnix>( saddr.get() );
    socket = dynamic_cast<csm::network::EndpointMultiUnix*>( MEP.NewEndpoint( caddr, opts ) );
    if( socket == nullptr )
      throw csm::network::Exception("Endpoint creation error");
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error" << e.what() << std::endl;
    MEP.Clear();
    client_count--;
    *(int*)retcode = 1;
    return retcode;
  }

  for( int n=0; n<TEST_LOOP_COUNT; ++n )
  {
    rc += client_send_recv_test( &MEP,
                                 saddr,
                                 threadHash,
                                 initialMsgID + n,
                                 // delay the last recv to trigger timeout
                                 (uint32_t)(n == TEST_LOOP_COUNT-1) * (TEST_ACK_TIMEOUT + 2) );
    if( TEST_LOOP_DELAY )
      usleep( TEST_LOOP_DELAY );
  }

  rc += ClientFinish(MEP, "UNIX");

  *(int*)retcode = rc;
  client_count--;
  return retcode;
}


void* client_main_ptp( void * retcode)
{
  setLoggingLevel(csmd, DBG_LEVEL);
  setLoggingLevel(csmnet, DBG_LEVEL );

  uint32_t initialMsgID = *(uint32_t*)retcode;

  int rc = 0;
  std::hash<std::thread::id> hashing;
  uint64_t threadHash = hashing( std::this_thread::get_id() );
  srand( (unsigned int)( threadHash ) );

  std::cout << "Starting PTP client (" << threadHash << ":" << initialMsgID << ")" << std::endl;

  csm::network::ReliableMsg MEP;

  csm::network::EndpointPTP_base *socket = nullptr;
  csm::network::AddressPTP_sptr saddr;
  csm::network::AddressPTP_sptr caddr;
  csm::network::EndpointOptionsPTP_sptr opts;
//  sleep(500);
  try {
    saddr = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT );
    caddr = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT+1 );
    opts = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( saddr.get() );
    socket = dynamic_cast<csm::network::EndpointPTP_base*>( MEP.NewEndpoint( caddr, opts ) );
    if( socket == nullptr )
      throw csm::network::Exception("Endpoint creation error");
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error" << e.what() << std::endl;
    MEP.Clear();
    client_count--;
    *(int*)retcode = 1;
    return retcode;
  }

  for( int n=0; (n<TEST_LOOP_COUNT)&&(rc==0); ++n )
  {
    rc += client_send_recv_test( &MEP,
                                 std::dynamic_pointer_cast<csm::network::Address>( saddr ),
                                 threadHash,
                                 initialMsgID + n,
                                 // delay the last recv to trigger timeout
                                 (uint32_t)(n == TEST_LOOP_COUNT-1) * (TEST_ACK_TIMEOUT + 2) );
    if( TEST_LOOP_DELAY )
      usleep( TEST_LOOP_DELAY );
  }

  rc += ClientFinish( MEP, "PTP" );

  *(int*)retcode = rc;
  client_count--;
  return retcode;
}



int client_test()
{
  // hold on briefly to make sure server is active
  usleep(20000);
  std::thread *t[ CLIENT_THREAD_COUNT ];
  int rc[CLIENT_THREAD_COUNT];
  bzero( rc, sizeof( int ) * CLIENT_THREAD_COUNT );
  int active_clients = 0;

  for( int i=0; (i<CLIENT_THREAD_COUNT) && (server_alive > 0); ++i)
  {
    rc[i] = (i+1) * 1000000;
    if( i < PTP_ENDPOINT_COUNT )
      t[i] = new std::thread( client_main_ptp, &rc[i] );
    else if( i < PTP_ENDPOINT_COUNT + MQTT_ENDPOINT_COUNT )
      t[i] = new std::thread( client_main_ptp, &rc[i] );
    else
      t[i] = new std::thread( client_main_unix, &rc[i] );
    ++active_clients;
  }

  int ret = (CLIENT_THREAD_COUNT - active_clients);
  for( int i=0; i<active_clients; ++i)
  {
    t[i]->join();
    ret += rc[i];
  }
  return ret;
}
#else /* TEST_WITH_C_CLIENT */


#include "csmnet/src/C/csm_network_msg_c.h"
#include "csmnet/src/C/csm_network_internal_api_c.h"
#include "csmutil/include/csmutil_logging.h"

int client_main_unix_c()
{
  setLoggingLevel(csmd, DBG_LEVEL);
  setLoggingLevel(csmnet, DBG_LEVEL );

  int rc = 0;

  std::hash<std::thread::id> hashing;
  uint64_t threadHash = hashing( std::this_thread::get_id() );

  uint64_t initialMsgID = 500000;

  srand( (unsigned int)( threadHash ) );
  std::cout << "Starting UNIX client (" << threadHash << ":" << initialMsgID << ")" << std::endl;

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

  struct csm_network_msg *msg;
  struct csm_network_msg *rmsg = nullptr;

  char *data = strdup( "Hello world from C-Client" );
  msg = csm_net_msg_Init(CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT, initialMsgID, geteuid(), getegid(), data, strnlen(data, 1000 ), 0 );

  for( int i=0; i<2; ++i )
  {
    if( msg != nullptr )
    {
      int slen = csm_net_unix_Send( ep->_ep, msg );
      if( slen <= 0 )
      {
        ++rc;
      }
      else
      {
        rmsg = csm_net_unix_Recv( ep->_ep );
        if( rmsg == nullptr )
          ++rc;
      }
    }
    else
      ++rc;
    sleep( 70 );
  }

  csm_net_unix_Exit( ep );

  client_count--;
  free( data );
  return rc;
}

#endif  /* TEST_WITH_C_CLIENT */

#endif /* CSMD_SRC_DAEMON_TESTS_CSM_RELIABLE_MSG_TEST_H_ */
