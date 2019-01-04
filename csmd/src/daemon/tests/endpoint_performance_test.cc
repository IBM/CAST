/*================================================================================

    csmd/src/daemon/tests/endpoint_performance_test.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include <unistd.h>
#include <netdb.h>

#include <iostream>
#include <chrono>

#include "logging.h"
#include "csm_network_header.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/endpoint_options.h"
#include "csmnet/src/CPP/reliable_msg.h"

#include "include/csm_retry_backoff.h"


#define SRV_UNIX "srv_perf_test"
#define SRV_PTP_IP 0x1f000001
#define SRV_PTP_PORT 17356

#define CLT_UNIX "clt_perf_test"
#define CLT_PTP_IP 0x1f000001
#define CLT_PTP_PORT 17357



struct config
{
  bool _server;
  csm::network::AddressType _type;
  csm::network::Address_sptr _addr;
  csm::network::Address_sptr _saddr;
  csm::network::EndpointOptions_sptr _opts;
  int _iterations;
  int _size;
  int _flood;
};


void usage( const char* text, const int rc )
{
  std::cerr << text << std::endl
      << " Usage:  [-h]           print help" << std::endl
      << "   [-a <address>]       address to listen/connect" << std::endl
      << "   [-b <message size>]  size of exchanged messages" << std::endl
      << "   [-d ]                debugging output" << std::endl
      << "   [-f <max_inflight>]  flood testing (don't wait for responses)" << std::endl
      << "   [-n <iterations>]    number of test iterations (default: 10)" << std::endl
      << "   [-p <port>]          port to liston/connect" << std::endl
      << "   [-s ]                server mode" << std::endl
      << "   [-t <u|p>]           unix domain or point-to-point transport" << std::endl
      << "   [-C <CA-file> ]      CA file for SSL communication" << std::endl
      << "   [-S <SSLcert> ]      SSL Cert+Key in PEM format" << std::endl
      << std::endl;
  exit( rc );
}


config* parse_options( int argc, char **argv)
{
  config *conf = new config;
  int opt;
  std::string addrString = "/tmp/perftest";
  long port = SRV_PTP_PORT;
  csm::network::SSLFilesCollection ssl;  // intentionally left blank

  conf->_server = false;
  conf->_size = 1;
  conf->_iterations = 10;
  conf->_type = csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL;
  conf->_flood = 0;

  while ((opt = getopt(argc, argv, "a:C:b:df:hn:p:sS:t:")) != -1) {
    switch( opt ) {
      case 'a':
        addrString = std::string( optarg );
        break;
      case 'b':
        conf->_size = strtol( optarg, nullptr, 10 );
        break;
      case 'C':
        ssl._CAFile = std::string( optarg );
        break;
      case 'd':
        setLoggingLevel(csmd, debug );
        setLoggingLevel(csmnet, debug );
        break;
      case 'f':
        conf->_flood = strtol( optarg, nullptr, 10 );
        break;
      case 'h':
        usage( "", 0 );
        break;
      case 'n':
        conf->_iterations = strtol( optarg, nullptr, 10 );
        break;
      case 'p':
        port = strtol( optarg, nullptr, 10 );
        break;
      case 's':
        conf->_server= true;
        break;
      case 'S':
        ssl._CredPem = std::string( optarg );
        break;
      case 't':
      {
        switch( optarg[ 0 ] )
        {
          case 'u':
          case 'U':
            conf->_type = csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL;
            break;
          case 'p':
          case 'P':
            conf->_type = csm::network::AddressType::CSM_NETWORK_TYPE_PTP;
            break;
          default:
            usage( "Unknown endpoint type.", 1 );
            break;
        }
        break;
      }
      default:
        usage( "Unknown command line option.", 1 );
        return nullptr;
    }

  }

  switch( conf->_type )
  {
    case csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL:
      conf->_saddr = std::make_shared<csm::network::AddressUnix>( addrString.c_str() );
      if( conf->_server )
      {

        conf->_opts = std::make_shared<csm::network::EndpointOptionsUnix>( true,
                                                                           std::make_shared<csm::daemon::CSMIAuthList>("csmconf/csm_api.acl"),
                                                                           std::make_shared<csm::daemon::CSMAPIConfig>("csmconf/csm_api.cfg"));
        conf->_addr = conf->_saddr;
      }
      else
      {
        conf->_opts = std::make_shared<csm::network::EndpointOptionsUnix>( addrString );
        std::dynamic_pointer_cast<csm::network::EndpointOptionsUnix>( conf->_opts )->_AuthList =
            std::make_shared<csm::daemon::CSMIAuthList>("csmconf/csm_api.acl");
        std::dynamic_pointer_cast<csm::network::EndpointOptionsUnix>( conf->_opts )->_APIConfig =
            std::make_shared<csm::daemon::CSMAPIConfig>("csmconf/csm_api.cfg");
        conf->_addr = std::make_shared<csm::network::AddressUnix>( addrString.append("clt").c_str() );
      }
      break;

    case csm::network::AddressType::CSM_NETWORK_TYPE_PTP:
    {
      struct addrinfo hints, *addrlist;

      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = 0;
      hints.ai_canonname = NULL;
      hints.ai_addr = NULL;
      hints.ai_next = NULL;

      int s = getaddrinfo(addrString.c_str(), std::to_string( port ).c_str(), &hints, &addrlist);
      if( s != 0 )
        throw csm::network::Exception("Failed to get address information.");

      if( addrlist == nullptr )
        throw csm::network::Exception("No address entry for the given host:port available.");

      conf->_saddr = std::make_shared<csm::network::AddressPTP>( ntohl( ((sockaddr_in*)addrlist->ai_addr)->sin_addr.s_addr ), port );

      if( conf->_server )
      {
        conf->_opts = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( conf->_server, ssl );
        conf->_addr = conf->_saddr;
      }
      else
      {
        freeaddrinfo( addrlist );
        port++;
        int s = getaddrinfo("localhost", std::to_string( port ).c_str(), &hints, &addrlist);
        if( s != 0 )
          throw csm::network::Exception("Failed to get address information.");

        if( addrlist == nullptr )
          throw csm::network::Exception("No address entry for the given host:port available.");

        csm::network::AddressPTP_sptr ptp_addr = std::dynamic_pointer_cast<csm::network::AddressPTP>( conf->_saddr );
        conf->_opts = std::make_shared<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( ntohl( ptp_addr->_IP() ), ntohs( ptp_addr->_Port() ), ssl );
        conf->_addr = std::make_shared<csm::network::AddressPTP>( ntohl( ((sockaddr_in*)addrlist->ai_addr)->sin_addr.s_addr ), port );
      }
      freeaddrinfo( addrlist );
      break;
    }
    default:
      break;
  }
  return conf;
}

#define SERVER_TOPIC "PERFTEST_SRV"
#define CLIENT_TOPIC "PERFTEST_CLT"

int server( const config *conf )
{
  csm::network::ReliableMsg MEP;

  csm::network::EndpointOptions_sptr opts = conf->_opts;
  csm::network::Address_sptr addr = conf->_saddr;
  csm::network::Endpoint *ep = nullptr;

  ep = MEP.NewEndpoint( addr, opts );
  if( ep == nullptr )
  {
    LOG( csmd, error ) << "Failed to create endpoint.";
    return EBADFD;
  }

  LOG( csmd, always ) << "Waiting for client to connect...: " << ep->IsServerEndpoint();

  csm::daemon::RetryBackOff IdleLoopRetry( "PerfTest", csm::daemon::RetryBackOff::SleepType::SOCKET,
                                           csm::daemon::RetryBackOff::SleepType::SOCKET,
                                           1, 10000, 1, &MEP );

  bool idle;
  while( true )
  {
    idle = true;
    try
    {
      MEP.Sync( csm::network::SYNC_ACTION_ALL );

      for( csm::network::NetworkCtrlInfo_sptr info_itr = MEP.GetCtrlEvent();
          ( info_itr != nullptr );
          info_itr = MEP.GetCtrlEvent())
      {
        switch( info_itr->_Type )
        {
          case csm::network::NET_CTL_DISCONNECT:
            LOG( csmd, always ) << "Client disconnected: " << info_itr->_Address->Dump();
            break;
          default:
            break;
        }
      }

      csm::network::Endpoint *clt = MEP.Accept( false );
      if( clt != nullptr )
      {
        idle = false;
        LOG( csmd, always ) << "Accepted client: " << clt->GetRemoteAddr()->Dump();
      }
    }
    catch( ... )
    {
      LOG( csmd, error ) << "Unhandled maintenance exception.";
    }

    csm::network::MessageAndAddress msgAddr;

    //////////////////////////////////////////
    // Receive/inbound Activity
    try
    {
//      int n=0;
      idle &= (MEP.RecvFrom( msgAddr ) == 0);
//      while(( n<2000 ) && (MEP.RecvFrom( msgAddr ) == 0) ) ++n;
    }
    catch ( csm::network::ExceptionEndpointDown &e )
    {
      LOG( csmd, warning ) << "found client disconnected in recv.";
    }
    catch ( ... )
    {
      LOG( csmd, error ) << "recv activity problem. If client ran to completion, this is not an issue.";
    }

    //////////////////////////////////////////
    // Send/outbound activity
    try
    {
      if( msgAddr.GetAddr() != nullptr )
      {
        LOG(csmd, info ) << "****************recv complete*********************************";
        idle &= ( MEP.SendTo( msgAddr ) == 0 );
        LOG(csmd, info ) << "/////////////////send complete///////////////////////////////";
      }
    }
    catch ( csm::network::ExceptionEndpointDown &e )
    {
      LOG( csmd, warning ) << "found client disconnected in send";
    }
    catch ( ... )
    {
      LOG( csmd, error ) << "send activity problem. If client ran to completion, this is not an issue.";
    }

    if( !idle )
      IdleLoopRetry.Reset();
    else
      IdleLoopRetry.AgainOrWait( false );
  }

  MEP.Clear();

  return 0;
}

int client( const config *conf )
{
  int rc = 0;
  csm::network::ReliableMsg MEP;

  csm::network::SSLFilesCollection ssl;  // intentionally left blank
  csm::network::EndpointOptions_sptr opts = conf->_opts;

  csm::network::Address_sptr addr = conf->_addr;
  csm::network::Address_sptr saddr = conf->_saddr;

  if( conf->_flood == 0 )
    std::cout << "Starting synchronous Ping-Pong Test... " << std::endl;
  else
    std::cout << "Starting flooding Ping-Pong Test with depth= " << conf->_flood << std::endl;


//  csm::network::Endpoint *ep =
  csm::network::Endpoint *ep = MEP.NewEndpoint( addr, opts );
  if( ep == nullptr )
  {
    LOG(csmd, error ) << "Failed to create and connect endpoint.";
    return EBADFD;
  }

  csm::network::MessageAndAddress msgAddr;
  char *st = ( char* )malloc( conf->_size + 1 );
  for( int n=0; n<conf->_size; ++n )
    st[n] = random() % 128 + 32;
  st[ conf->_size ] = 0;
  std::string data(st);

  msgAddr._Msg.Init( CSM_CMD_node_attributes_query, 0, CSM_PRIORITY_NO_ACK,
                     1, 0353, 0371,
                     geteuid(), getegid(), data );
  msgAddr.SetAddr( saddr );

  int progress_step = conf->_iterations > 64 ? (conf->_iterations >> 8) : conf->_iterations;
  int in_flight = 0;  // keep track of in-flight msgs
  int max_inflight = 0;

  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  int i = conf->_iterations;
  while( (i > 0) || (in_flight > 0) )
  {
    //////////////////////////////////////////
    // Send/outbound activity
    try
    {
      if(( i > 0 ) && ( in_flight <= conf->_flood ))
      {
        msgAddr._Msg.SetMessageID( i + 10 );
        msgAddr._Msg.CheckSumUpdate();
        MEP.SendTo( msgAddr );
        --i;
        ++in_flight;
        if( in_flight > max_inflight )
          max_inflight = in_flight;
      }
    }
    catch( csm::network::Exception &e )
    {
      LOG(csmd, error ) << "Send: " << e.what();
    }
    catch ( ... )
    {
      LOG( csmd, error ) << "Unhandled send activity exception.";
      rc++;
    }

    LOG(csmd, info ) << "/////////////////send complete///////////////////////////////";
    //////////////////////////////////////////
    // Receive/inbound Activity
    try
    {
      ssize_t rlen = 0;
      do
      {
        rlen = MEP.RecvFrom( msgAddr );
      }
      while( ( conf->_flood == 0 ) && ( rlen == 0) );

      if( rlen )
      {
        if( (i > 0) && ( ! (i % progress_step)) )
        {
          std::cout << ".";
          std::flush(std::cout);
        }
        --in_flight;
      }
    }
    catch( csm::network::Exception &e )
    {
      LOG(csmd, error ) << "Recv: " << e.what();
    }
    catch ( ... )
    {
      LOG( csmd, error ) << "Unhandled recv activity exception.";
      rc++;
    }
    LOG(csmd, info ) << "****************recv complete*********************************";

  }
  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
  std::cout << std::endl;

  uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
  std::cout << "MsgSize[bytes]: " << conf->_size
      << " Iterations: " << conf->_iterations
      << " Time[us]: " << micros
      << " IOPS: " << (double)conf->_iterations / micros * 1000000.
      << " max_inflight: " << max_inflight
      << std::endl;

  MEP.Clear();
  if(st) free(st);
  return rc;
}


int main( int argc, char **argv )
{
  int rc = 0;

  boost::property_tree::ptree pt;
  initializeLogging("perftest", pt);
  setLoggingLevel(csmd, warning );
  setLoggingLevel(csmnet, warning );

  config *conf = parse_options( argc, argv );

  if( conf->_server )
    rc = server( conf );
  else
    rc = client( conf );


  if( conf != nullptr )
    delete conf;

  std::cout << "Exiting with rc=" << rc << std::endl;
  return rc;
}



