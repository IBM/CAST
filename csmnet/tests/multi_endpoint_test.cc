/*================================================================================

    csmnet/tests/multi_endpoint_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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
#include <CPP/csm_network_msg_cpp.h>
#include <CPP/csm_network_exception.h>
#include <CPP/endpoint.h>
#include <CPP/endpoint_ptp.h>
#include <CPP/endpoint_dual_unix.h>
#include <CPP/endpoint_multi_unix.h>
#include <CPP/multi_endpoint.h>
#include <CPP/pubsub_ack_rewrite.h>

#define CLIENT_THREAD_COUNT ( 10 )
#define PTP_ENDPOINT_COUNT ( CLIENT_THREAD_COUNT - 5 )

#define TEST_LOOP_COUNT ( 200 )
#define TEST_LOOP_DELAY ( 0 )   // arg for usleep

#define DATA_BUFFER_SIZE 8192

#define TEST_INITIAL_PORT 31420

std::atomic_int client_count( CLIENT_THREAD_COUNT );
std::atomic_int server_alive( 1 );

void* server_echo( void * retcode)
{
  std::cout << "Server: Starting" << std::endl;

  int rc = 0;

  csm::network::MultiEndpoint MEP;

  csm::network::EndpointMultiUnix *local = nullptr;
  csm::network::AddressUnix_sptr addr_local;
  csm::network::EndpointOptionsUnix_sptr opts_local;

#if PTP_ENDPOINT_COUNT > 0
  csm::network::EndpointPTP *socket = nullptr;
#endif
  csm::network::AddressPTP_sptr addr_ptp;
  csm::network::EndpointOptionsPTP_sptr opts_ptp;

  try
  {
    addr_local = std::make_shared<csm::network::AddressUnix>( CSM_NETWORK_LOCAL_SSOCKET );
    opts_local = std::make_shared<csm::network::EndpointOptionsUnix>( true );

    addr_ptp = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT );
    opts_ptp = std::make_shared<csm::network::EndpointOptionsPTP>( true );

    local = dynamic_cast<csm::network::EndpointMultiUnix*>( MEP.NewEndpoint( addr_local, opts_local ) );
    if( ! local )
      rc = 1;

#if PTP_ENDPOINT_COUNT > 0
    socket = dynamic_cast<csm::network::EndpointPTP*>( MEP.NewEndpoint( addr_ptp, opts_ptp ) );
    if( ! socket )
      rc = 1;
#endif

    if( rc != 0 )
      LOG(csmnet, error) << "Local or PTP socket not created.";

    for( int i=0; i<PTP_ENDPOINT_COUNT; ++i)
    {
      csm::network::Endpoint *clt = nullptr;
      while( clt == nullptr )
        clt = MEP.Accept( true );
      if( clt->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP )
        throw csm::network::Exception("server_echo(): Accept call returned invalid endpoint type.");
      LOG(csmnet, info) << "SERVER: ACCEPTED EP: " << clt->GetLocalAddr()->Dump() << "->" << clt->GetRemoteAddr()->Dump();
    }
  }
  catch (csm::network::Exception &e) {
    std::cout << "Socket creation error" << e.what() << std::endl;
    rc = 1;
  }

  LOG(csmnet, info) << "#####################################################################################";

  if( ! rc ) {

    char *buffer = (char*)malloc( DATA_BUFFER_SIZE );
    bzero( buffer, DATA_BUFFER_SIZE );

    for( int i=0;
        (rc==0)&&( i< CLIENT_THREAD_COUNT  * TEST_LOOP_COUNT )&&(client_count == CLIENT_THREAD_COUNT);
        ++i )
    {
      csm::network::Address_sptr RemoteAddr;

      try
      {
        size_t rlen = 0;
        csm::network::MessageAndAddress msgAddr;
        std::cout << "Server: going to recv" << std::endl;
        while( rlen == 0 )
        {
          rlen = MEP.RecvFrom( msgAddr );
        }
        if( rlen > 0 )
          RemoteAddr = msgAddr.GetAddr();
        std::cout << "Server: Receved " << rlen
          << " bytes, clnt_add: " << RemoteAddr->Dump() << std::endl;

        if( MEP.SendTo( msgAddr ) == 0 )
        {
          LOG(csmnet, info) << "server_echo(): Warning: Sent returned 0.";
          rc = 1;
        }
      }
      catch (csm::network::Exception &e)
      {
        std::cout << "Send/Recv error: " << e.what() << std::endl;
        rc = 1;
      }
    }
  }
  else {
    perror("Server: Binding");
  }
  while( client_count.load() )
  {
    usleep( 10000 );
  }
  std::cout << "Server: Exiting" << std::endl;

  server_alive--;
  MEP.Clear();

  *(int*)retcode = rc;
  return retcode;
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
  if( A.GetData()        != B.GetData() )          return 8;
  return 0;
}

int client_send_recv_test( csm::network::Endpoint *socket, const std::string &clientcode )
{
  int rc = 0;
  csm::network::Message msg, in;

  rc = msg.Init( CSM_CMD_ECHO,
                 0,
                 CSM_PRIORITY_DEFAULT,
                 2563245,
                 0x07341204,
                 0x06341204,
                 geteuid(), getegid(),
                 "Hello World"+clientcode );

  if(( rc )&&(server_alive.load()>0)) {
    try
    {
      rc = socket->Send( msg );
      LOG(csmnet, info) << "client_test(): Send returned: " << rc;
      rc *= (rc < 0);  // keep status if < 0

      LOG(csmnet, info) << "client_test(): entering recv: " << rc;
      while((!rc)&&(server_alive.load()>0))
      {
        rc = socket->Recv( in );
      }
      LOG(csmnet, info) << "client_test(): recv finished: " << rc;

    }
    catch ( csm::network::Exception &e)
    {
      LOG(csmnet, error) << "client_test(): Network Send error" << e.what();
      if( ! rc ) rc = errno;
    }

    if( (rc = msg_memcmp( msg, in)) != 0 )
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
  std::cout << "Starting UNIX client" << std::endl;
  int rc = 0;

  csm::network::MultiEndpoint MEP;

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
    rc += client_send_recv_test( dynamic_cast<csm::network::Endpoint*>( socket ),
                                 std::to_string(*((uint64_t*)retcode)) );
    if( TEST_LOOP_DELAY )
      usleep( TEST_LOOP_DELAY );
  }

  sleep(1);
  std::cout << "Exiting client" << std::endl;

  MEP.Clear();

  *(int*)retcode = rc;
  client_count--;
  return retcode;
}


void* client_main_ptp( void * retcode)
{
  std::cout << "Starting PTP client" << std::endl;

  int rc = 0;

  csm::network::MultiEndpoint MEP;

  csm::network::EndpointPTP *socket = nullptr;
  csm::network::AddressPTP_sptr saddr;
  csm::network::AddressPTP_sptr caddr;
  csm::network::EndpointOptionsPTP_sptr opts;
//  sleep(500);
  try {
    saddr = std::make_shared<csm::network::AddressPTP>( 0x7f000001, TEST_INITIAL_PORT );
    caddr = std::make_shared<csm::network::AddressPTP>( 0x7f000002, TEST_INITIAL_PORT+1 );
    opts = std::make_shared<csm::network::EndpointOptionsPTP>( saddr.get() );
    socket = dynamic_cast<csm::network::EndpointPTP*>( MEP.NewEndpoint( caddr, opts ) );
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
    rc += client_send_recv_test( dynamic_cast<csm::network::Endpoint*>( socket ),
                                 std::to_string(*((uint64_t*)retcode)) );
    if( TEST_LOOP_DELAY )
      usleep( TEST_LOOP_DELAY );
  }

  sleep(1);
  std::cout << "Exiting client" << std::endl;

  MEP.Clear();

  *(int*)retcode = rc;
  client_count--;
  return retcode;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int crc[CLIENT_THREAD_COUNT];

//    setLoggingLevel(csmd, warning);
//    setLoggingLevel(csmnet, warning);

    int src = 0;
    std::thread server( server_echo, &src );

    std::thread *t[ CLIENT_THREAD_COUNT ];
    bzero( crc, sizeof( int ) * CLIENT_THREAD_COUNT );

    for( int i=0; i<CLIENT_THREAD_COUNT; ++i)
    {
      if( i < PTP_ENDPOINT_COUNT )
        t[i] = new std::thread( client_main_ptp, &crc[i] );
      else
        t[i] = new std::thread( client_main_unix, &crc[i] );
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
