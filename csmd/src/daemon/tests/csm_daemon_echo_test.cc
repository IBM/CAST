/*================================================================================

    csmd/src/daemon/tests/csm_daemon_echo_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_network_engine_test.cc
 *
 ******************************************/

// test source, sink, and processor for network handling

#include <iostream>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>

#include <logging.h>

#include "csmutil/include/csm_test_utils.h"

#include "csmnet/src/CPP/csm_network_exception.h"
#include "csmnet/src/CPP/endpoint.h"
#include "csmnet/src/CPP/endpoint_unix.h"
#include "csmnet/src/CPP/endpoint_multi_unix.h"


#define NUMTESTS ( 4 )

#define DATA_BUFFER_SIZE ( 1024 )

// use char* as input to allow sending msg or raw data depending on flag
bool SendRecvSequence( csm::network::EndpointMultiUnix &ep,
                       csm::network::Message &in, const bool isMsg,
                       const size_t len )
{
  int rc = 0;

  rc = ep.Send( in );
  std::cout << "Client sent data: " << rc << std::endl;

  csm::network::Message data;
  do {
    rc = ep.Recv( data );
  } while ( rc == 0 );
  bool valid = data.Validate();
  if(( rc > 0 ) && ( valid ))
    std::cout << "Client: received. Msg: " << data << std::endl;
  else
  {
    std::cerr << "Client: receive error. rc=" << rc << " data valid=" << valid << std::endl;
    if( !rc ) rc = -1;
  }

  // compare message data if we didn't get an error response
  if( ( data.GetCommandType() != CSM_CMD_ERROR ) && ( rc > 0 ) )
  {
    csm::network::Message inmsg = in;
    if( inmsg.Validate() )
    {
      std::cout << "Client input data: " << inmsg.GetDataLen() << " data: " << inmsg.GetData() << std::endl;
      rc = strncmp( (const char*)data.GetData().c_str(),
                    (const char*)inmsg.GetData().c_str(),
                    std::min( data.GetDataLen(), inmsg.GetDataLen() ) );
    }
    else
      rc = -1;
  }

  return rc == 0;
}

int client( void )
{
  int rc = 0;
  std::cout << "Started client " << std::endl;
  try {
    csm::network::EndpointMultiUnix ep( std::make_shared<csm::network::AddressUnix>( CSM_NETWORK_LOCAL_CSOCKET ),
                                        std::make_shared<csm::network::EndpointOptionsUnix>( false ));
    std::cout << "Client endpoint created. Connecting..." << std::endl;

    std::string socket_name = std::string( CSM_NETWORK_LOCAL_SSOCKET );
    if (getenv("CSM_SSOCKET")) socket_name = std::string( getenv("CSM_SSOCKET") );

    csm::network::AddressUnix_sptr srvaddr = std::make_shared<csm::network::AddressUnix>( socket_name.c_str() );
    ep.Connect( srvaddr );
    std::cout << "Client connected " << std::endl;

    int test_count = 0;
    if( NUMTESTS > 0 )
    {
//      std::string teststr("Hello World, this is an initial test to see if any data arrives at the other end. But the test has to fail");
//      rc = TEST( SendRecvSequence( ep, teststr.c_str(), false, teststr.length() ),
//                 false );
//      std::cout << "================ rc=" << rc << " =======================================" << std::endl;
      ++test_count;
    }
    if( NUMTESTS > test_count )
    {
      csm::network::Message msg;
      bool hdrvalid = msg.Init(CSM_CMD_ECHO,
                               0,
                               CSM_PRIORITY_DEFAULT,
                               12345678,
                               0x01020304,
                               0x04030201,
                               geteuid(), getegid(),
                               "Hi Other World, this test should succeed." );
      std::cout << "Header correctness check: " << hdrvalid << std::endl;
      rc += TEST( SendRecvSequence( ep, msg, true, msg.GetDataLen() + sizeof(csm_network_header) ),
                  true );
      std::cout << "================ rc=" << rc << " =======================================" << std::endl;
      ++test_count;
    }
    if( NUMTESTS > test_count )
    {
      csm::network::Message msg;
      // will return all the nodes in the "nodes" db table without supplying arugments
      bool hdrvalid = msg.Init(CSM_CMD_node_attributes_query,
                               0,
                               CSM_PRIORITY_DEFAULT,
                               9876543,
                               0x01020304,
                               0x04030201,
                               geteuid(), getegid(),
                               "" );
      std::cout << "Header correctness check: " << hdrvalid << std::endl;
      rc += TEST( SendRecvSequence( ep, msg, true, msg.GetDataLen() + sizeof(csm_network_header) ),
                  true );
      std::cout << "================ rc=" << rc << " =======================================" << std::endl;
      ++test_count;
    }
    if( NUMTESTS > test_count )
    {
      csm::network::Message msg;
      //  trying to fail the daemon: will send a message that's correct but has unknown/not-implemented cmd
      bool hdrvalid = msg.Init(CSM_CTRL_reconfig,
                               0,
                               CSM_PRIORITY_DEFAULT,
                               9876543,
                               0x01020304,
                               0x04030201,
                               geteuid(), getegid(),
                               "" );
      std::cout << "Header correctness check: " << hdrvalid << std::endl;
      rc += TEST( SendRecvSequence( ep, msg, true, msg.GetDataLen() + sizeof(csm_network_header) ),
                  false );
      std::cout << "================ rc=" << rc << " =======================================" << std::endl;
    }
  }
  catch (csm::network::Exception &e) {
    std::cerr << "FAILED CLIENT" << e.what() << std::endl;
    rc += 1;
  }
  std::cout << "client exit: rc=" << rc << std::endl;
  return rc;
}

int main( int argc, char **argv )
{
  int rc = 0;

  // need a client that feeds network messages into a server
  rc += client( );

  std::cout << getpid() << " Exiting with rc=" << rc << std::endl;

  return rc;
}
