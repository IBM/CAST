/*================================================================================

    csmd/src/daemon/tests/csm_network_pool_test.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <mutex>
#include <thread>

#include "logging.h"
#include "csmutil/include/csm_test_utils.h"

#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/csm_message_and_address.h"
#include "csmnet/src/CPP/reliable_msg.h"

#include "include/virtual_network_channel.h"
#include "include/virtual_network_channel_pool.h"


#define POOL_SIZE ( 10 )
#define THREAD_COUNT ( 10 )
#define TEST_LOOPS ( 10000 )

csm::daemon::VirtualNetworkChannelPool *pool = nullptr;

void* connection_grabber( void * retcode)
{
  int rc = 0;

  for( unsigned int i = TEST_LOOPS; i > 0; --i )
  {
    csm::daemon::VirtualNetworkChannel_sptr chan = pool->AcquireNetworkChannel();
    if( chan != nullptr )
      pool->ReleaseNetworkChannel( chan );

    // if pool size is sufficient, we should never get nullptr...
    if(( chan == nullptr ) && ( POOL_SIZE >= THREAD_COUNT ))
    {
      ++rc;
      LOG( csmd, error ) << "Couldn't get channel from pool.";
    }
  }

  *(int*)retcode = rc;
  return retcode;
}

int main( int argc, char **argv )
{
  int ret = 0;
  srandom( time(0) );

  csm::network::ReliableMsg MEP;

  csm::daemon::RetryBackOff *IdleLoopRetry =
      new csm::daemon::RetryBackOff( "NetMgr", csm::daemon::RetryBackOff::SleepType::SOCKET,
                                     csm::daemon::RetryBackOff::SleepType::INTERRUPTIBLE_SLEEP,
                                     1, 10000, 1000, &MEP );

  pool = csm::daemon::VirtualNetworkChannelPool::Init( POOL_SIZE, IdleLoopRetry );

  ret += TEST( pool->GetNumOfLockedNetworkChannels(), 0 );
  ret += TEST( pool->GetNumOfFreeNetworkChannels(), POOL_SIZE );
  ret += TEST( pool->GetNumOfNetworkChannels(), POOL_SIZE );

  LOG(csmd, info) << "Init Pool: " << ret;

  csm::daemon::VirtualNetworkChannel_sptr cnl[ POOL_SIZE ];

  for( unsigned int i = 0; i < POOL_SIZE; ++i )
  {
    ret += TESTFAIL( cnl[ i ] = pool->AcquireNetworkChannel(), nullptr );
    ret += TEST( pool->GetNumOfLockedNetworkChannels(), i + 1 );
    ret += TEST( pool->GetNumOfFreeNetworkChannels(), POOL_SIZE - i - 1 );
    ret += TEST( pool->GetNumOfNetworkChannels(), POOL_SIZE );
  }

  LOG(csmd, info) << "Fill: " << ret;

  ret += TEST( pool->GetNumOfLockedNetworkChannels(), POOL_SIZE );
  ret += TEST( pool->GetNumOfFreeNetworkChannels(), 0 );
  ret += TEST( pool->GetNumOfNetworkChannels(), POOL_SIZE );

  csm::daemon::VirtualNetworkChannel_sptr onemore;
  ret += TEST( onemore = pool->AcquireNetworkChannel(), nullptr );

  LOG(csmd, info) << "Overcommit: " << ret;

  pool->ReleaseNetworkChannel( onemore );

  ret += TEST( pool->GetNumOfLockedNetworkChannels(), POOL_SIZE );
  ret += TEST( pool->GetNumOfFreeNetworkChannels(), 0 );
  ret += TEST( pool->GetNumOfNetworkChannels(), POOL_SIZE );

  for( unsigned int i = 0; i < POOL_SIZE; ++i )
  {
    pool->ReleaseNetworkChannel( cnl[ i ] );
    ret += TEST( pool->GetNumOfLockedNetworkChannels(), POOL_SIZE - i - 1 );
    ret += TEST( pool->GetNumOfFreeNetworkChannels(), i + 1 );
    ret += TEST( pool->GetNumOfNetworkChannels(), POOL_SIZE );
  }
  LOG(csmd, info) << "Flush: " << ret;

  csm::daemon::VirtualNetworkChannel_sptr first = pool->AcquireNetworkChannel();
  csm::daemon::VirtualNetworkChannel_sptr second = pool->AcquireNetworkChannel();
  pool->ReleaseNetworkChannel( first );
  csm::daemon::VirtualNetworkChannel_sptr third = pool->AcquireNetworkChannel();
  ret += TEST( first, third );
  LOG(csmd, info) << "Stack: " << ret;

  pool->ReleaseNetworkChannel( second );
  pool->ReleaseNetworkChannel( third );
  ret += TEST( pool->GetNumOfFreeNetworkChannels(), POOL_SIZE );


  std::thread *t[ THREAD_COUNT ];
  int rc[ THREAD_COUNT ];
  bzero( rc, sizeof( int ) * THREAD_COUNT );
  int active_clients = 0;

  for( int i=0; i<THREAD_COUNT; ++i)
  {
    t[i] = new std::thread( connection_grabber, &rc[ i ] );
    ++active_clients;
  }

  ret += ( THREAD_COUNT - active_clients );
  for( int i=0; i<active_clients; ++i)
  {
    t[i]->join();
    ret += rc[i];
  }

  delete IdleLoopRetry;

  LOG(csmd, info) << "Exit test: " << ret;

  return ret;
}


