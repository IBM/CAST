/*================================================================================

    csmd/src/daemon/include/virtual_network_channel_pool.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_VIRTUAL_NETWORK_CHANNEL_POOL_H_
#define CSMD_SRC_DAEMON_INCLUDE_VIRTUAL_NETWORK_CHANNEL_POOL_H_

#include <unordered_set>
#include <stack>
#include <mutex>

#include "logging.h"
#include "include/virtual_network_channel.h"

namespace csm {
namespace daemon {

class EventManagerNetwork;

class VirtualNetworkChannelPool {
  // make netmgr a friend to prevent handlers from accessing anything other than acquire/release
  friend class EventManagerNetwork;

public:
  static VirtualNetworkChannelPool* Init( unsigned i_PoolSize,
                                          csm::daemon::RetryBackOff *i_ManagerWakeup )
  {
    if( _Instance == nullptr )
      _Instance = new VirtualNetworkChannelPool( i_PoolSize, i_ManagerWakeup );
    return _Instance;
  }
  static VirtualNetworkChannelPool* GetInstance()
  {
    if( _Instance == nullptr )
      throw csm::daemon::Exception("VirtualNetworkChannelPool: need to call Init first.");
    return _Instance;
  }


  VirtualNetworkChannel_sptr AcquireNetworkChannel()
  {
    VirtualNetworkChannel_sptr ret = nullptr;
    std::lock_guard<std::mutex> guard( _PoolLock );

    if( !_FreeConnPool.empty() )
    {
      ret = _FreeConnPool.top();
      _FreeConnPool.pop();
      ret->Reserve();
      _LockedConnPool.insert( ret );
    }

    if( ret != nullptr )
      LOG(csmd, debug) << "Use ChannelID " << ret->GetId() << " from the pool...";

    return ret;
  }

  void ReleaseNetworkChannel( csm::daemon::VirtualNetworkChannel_sptr i_Channel )
  {
    if ( i_Channel == nullptr) return;

    std::lock_guard<std::mutex> guard( _PoolLock );

    // drain the channel recv queue to the main/dedicated queue
    csm::network::MessageAndAddress msgAddr;
    csm::daemon::EventContext_sptr ctx;
    i_Channel->Release_Start();
    while( i_Channel->Recv( msgAddr, ctx, 0 ) >= 0 )
    {
      LOG( csmd, debug ) << "Draining Released Recv-Queue: " << i_Channel->GetId() << " msgId=" << msgAddr._Msg.GetMessageID();
      _DedicatedChannel->QueueInboundMsg( std::make_shared<csm::network::MessageAndAddress>( msgAddr ), ctx );
    }
    csm::network::MessageAndAddress_sptr msgAddr_sptr;
    while( ( msgAddr_sptr = i_Channel->GetOutboundMsg( ctx )) != nullptr )
    {
      LOG( csmd, debug ) << "Draining Released Send-Queue: " << i_Channel->GetId() << " msgId=" << msgAddr._Msg.GetMessageID();
      _DedicatedChannel->Send( msgAddr_sptr, ctx );
    }

    if (_LockedConnPool.erase( i_Channel ) == 1)
    {
      // successfully remove it from _LockedConnPool. Add back to _FreeConnPool.
      _FreeConnPool.push( i_Channel );
      i_Channel->Release_Complete();
      LOG(csmd, debug) << "Returning ChannelID " << i_Channel->GetId() << " to the pool...";
    }
    else
    {
      LOG(csmd, error) << "Cannot find the network channelID " << i_Channel->GetId() << " in _LockedConnPool!";
    }
  }

  unsigned GetNumOfNetworkChannels() const { return _PoolSize; }
  unsigned GetNumOfFreeNetworkChannels() const { return _FreeConnPool.size(); }
  unsigned GetNumOfLockedNetworkChannels() const { return _LockedConnPool.size(); }

  virtual ~VirtualNetworkChannelPool()
  {
    // lock all channels and then clear the list
    while( AcquireNetworkChannel() != nullptr ) {}
    _LockedConnPool.clear();
  }

private:
  static VirtualNetworkChannelPool* _Instance;

  std::unordered_set<csm::daemon::VirtualNetworkChannel_sptr> _LockedConnPool;
  std::stack<csm::daemon::VirtualNetworkChannel_sptr> _FreeConnPool;
  mutable csm::daemon::VirtualNetworkChannel_sptr _DedicatedChannel;

  std::mutex _PoolLock;

  unsigned _PoolSize;

  VirtualNetworkChannelPool( unsigned int i_InitialPoolSize,
                             csm::daemon::RetryBackOff *i_ManagerWakeup )
  {
    _DedicatedChannel = std::make_shared<csm::daemon::VirtualNetworkChannel>( 0xFFFF, i_ManagerWakeup );
    if( _DedicatedChannel == nullptr )
      throw csm::daemon::Exception("Failed to allocate dedicated network channel in network connection pool.");

    _PoolSize = 0;
    for( unsigned i = 0; i < i_InitialPoolSize; ++i )
    {
      VirtualNetworkChannel_sptr chan = std::make_shared<csm::daemon::VirtualNetworkChannel>( i, i_ManagerWakeup );
      if( chan == nullptr )
        throw csm::daemon::Exception("Failed to allocateo network channel in connection pool.");

      _FreeConnPool.push( chan );
      _PoolSize++;
    }
  }

  void AddChannel( csm::daemon::VirtualNetworkChannel_sptr i_Channel )
  {
    std::lock_guard<std::mutex> guard( _PoolLock );
    _FreeConnPool.push( i_Channel );
    ++_PoolSize;
  }

  csm::daemon::VirtualNetworkChannel_sptr GetDedicatedChannel() const
  {
    if( _DedicatedChannel->IsReserved() )
      return nullptr;
    _DedicatedChannel->Reserve();
    return _DedicatedChannel;
  }

  void ReleaseDedicatedChannel()
  {
    _DedicatedChannel->Release_Start();
    // todo: drain the send/recv queues and create events?
    _DedicatedChannel->Release_Complete();
  }

  bool GetOutboundActivity( csm::daemon::VirtualNetworkChannel_sptr &o_VChannel,
                            csm::daemon::EventContext_sptr &o_Context,
                            csm::network::MessageAndAddress_sptr &o_MsgAddr )
  {
    csm::network::MessageAndAddress_sptr msgPtr = nullptr;
    std::lock_guard<std::mutex> guard( _PoolLock );

    msgPtr = _DedicatedChannel->GetOutboundMsg( o_Context );
    if( msgPtr != nullptr )
    {
      o_VChannel = _DedicatedChannel;
      o_MsgAddr = msgPtr;
      return true;
    }

    for( auto it =_LockedConnPool.begin(); it != _LockedConnPool.end(); ++it )
    {
      msgPtr = (*it)->GetOutboundMsg( o_Context );
      if( msgPtr != nullptr )
      {
        o_VChannel = *it;
        o_MsgAddr = msgPtr;
        return true;
      }
    }
    return false;
  }

};

}   // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_VIRTUAL_NETWORK_CHANNEL_POOL_H_ */
