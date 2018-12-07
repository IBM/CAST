/*================================================================================

    csmd/src/daemon/include/virtual_network_channel.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_VIRTUAL_NETWORK_CHANNEL_H_
#define CSMD_SRC_DAEMON_INCLUDE_VIRTUAL_NETWORK_CHANNEL_H_

#include <deque>
#include <mutex>
#include <atomic>

#include <include/csm_core_event.h>
#include <include/csm_retry_backoff.h>

namespace csm {
namespace daemon {

#define DEFAULT_NET_REQUEST_TIMEOUT ( 60 )

class VirtualNetworkChannelPool;

class VirtualNetworkChannel {
  const int IDLE = 0;
  const int RESERVED = 1;
  const int RELEASING = 2;

  friend class VirtualNetworkChannelPool;

  typedef std::chrono::time_point< std::chrono::steady_clock > TimeType;
  typedef std::pair< csm::network::MessageAndAddress_sptr, csm::daemon::EventContext_sptr > MsgAndCtxType;
  typedef std::deque< csm::daemon::CoreEvent* > ConnectionEventQueue;
  typedef std::deque< MsgAndCtxType > ConnectionMessageQueue;

  ConnectionEventQueue _Events;   // system events for this channel (timeouts, errors, disconnects)
  ConnectionMessageQueue _Inbound;  // regular recv msg queue
  ConnectionMessageQueue _Outbound; // regular send msg queue

  std::mutex _QueueLock;
  std::atomic_int _State;

  uint64_t _Id;
  csm::daemon::RetryBackOff *_ManagerWakeup;

public:
  VirtualNetworkChannel( const uint64_t i_Id,
                         csm::daemon::RetryBackOff *i_ManagerWakeup )
  : _State( IDLE ), _Id( i_Id ),
    _ManagerWakeup( i_ManagerWakeup )
  { }
  ~VirtualNetworkChannel()
  {
    _Events.clear();
    _Inbound.clear();
  }

  // return -ENOMSG if there was nothing received within the timeout
  // returns datalen (might be 0 if just header) if msg was received
  ssize_t Recv( csm::network::MessageAndAddress &o_MsgAddr,
                csm::daemon::EventContext_sptr &o_Ctx,
                const uint32_t i_TimeoutMicroSeconds = DEFAULT_NET_REQUEST_TIMEOUT )
  {
    TimeType ts = std::chrono::steady_clock::now() + std::chrono::microseconds( i_TimeoutMicroSeconds );
    MsgAndCtxType msgCtx;

    if( _State < RESERVED )
    {
      LOG( csmd, error ) << "VChannel::Recv(): Channel "<< _Id << " not reserved by handler.";
      return -ENOTCONN;
    }

    do
    {
      _QueueLock.lock();
      if( ! _Inbound.empty() )
      {
        msgCtx = _Inbound.front();
        _Inbound.pop_front();
        _QueueLock.unlock();
        break;
      }
      _QueueLock.unlock();
      sched_yield();
    } while( std::chrono::steady_clock::now() < ts );

    if( msgCtx.first != nullptr )
    {
      o_MsgAddr = *( msgCtx.first );
      o_Ctx = msgCtx.second;
      LOG( csmd, debug ) << "Retrieving new inbound msg (msgID=" << o_MsgAddr._Msg.GetMessageID()
          << ", ctx=" << o_Ctx.get()
          << ") for VChan=" << _Id;
      return o_MsgAddr._Msg.GetDataLen();
    }
    return -ENOMSG;
  }

  ssize_t Send( const csm::network::MessageAndAddress_sptr i_MsgAddr,
                const csm::daemon::EventContext_sptr i_Ctx,
                const uint32_t i_TimeoutMicroSeconds = DEFAULT_NET_REQUEST_TIMEOUT )
  {
    {
      std::lock_guard<std::mutex> guard( _QueueLock );
      if( _State != RESERVED )
      {
        LOG( csmd, error ) << "VChannel::Send(): Channel "<< _Id << " not reserved by handler.";
        return -ENOTCONN;
      }
      LOG( csmd, debug ) << "VChannel::Send(): Channel "<< _Id
          << " new msg=" << i_MsgAddr->_Msg.GetMessageID()
          << " ctx=" << i_Ctx;
      _Outbound.push_back( std::make_pair( i_MsgAddr, i_Ctx ) );
    }
    _ManagerWakeup->WakeUp();
    // todo: timeout options... (right now missing forwarded successful ACK)
    // TimeType ts = std::chrono::steady_clock::now() + std::chrono::seconds( i_TimeoutMicroSeconds );
    return i_MsgAddr->_Msg.GetDataLen();
  }

  csm::daemon::CoreEvent* GetEvent( const uint32_t i_TimeoutSeconds = DEFAULT_NET_REQUEST_TIMEOUT )
  {
    std::lock_guard<std::mutex> guard( _QueueLock );
    if( ! _Events.empty() )
    {
      csm::daemon::CoreEvent *ret = _Events.front();
      _Events.pop_front();
      return ret;
    }
    return nullptr;
  }

  inline uint64_t GetId() const { return _Id; }

  bool QueueInboundMsg( const csm::network::MessageAndAddress_sptr i_MsgAddr,
                        const csm::daemon::EventContext_sptr i_Ctx )
  {
    std::lock_guard<std::mutex> guard( _QueueLock );
    if( _State != RESERVED )
    {
      LOG( csmd, debug ) << "QueueInboundMsg: vchannel " << _Id << " not reserved. Not queueing msg.";
      return false;
    }
    if( i_MsgAddr != nullptr )
    {
      _Inbound.push_back( std::make_pair( i_MsgAddr, i_Ctx ) );
      LOG( csmd, debug ) << "QueueInboundMsg: vchannel " << _Id
          << " new inbound msg (msgID=" << i_MsgAddr->_Msg.GetMessageID()
          << " ctx=" << i_Ctx << ": ctxS=" << _Inbound.back().second << ")";
      return true;
    }
    return false;
  }

  bool QueueInboundEvent( csm::daemon::CoreEvent * const i_Event )
  {
    std::lock_guard<std::mutex> guard( _QueueLock );
    if( _State != RESERVED )
      return false;
    _Events.push_back( i_Event );
    return true;
  }

private:
  csm::network::MessageAndAddress_sptr GetOutboundMsg( csm::daemon::EventContext_sptr &o_Context )
  {
    std::lock_guard<std::mutex> guard( _QueueLock );
    if( ! _Outbound.empty() )
    {
      MsgAndCtxType msgCtx = _Outbound.front();
      _Outbound.pop_front();
      o_Context = msgCtx.second;
      return msgCtx.first;
    }
    return nullptr;
  }

  inline void Reserve() { _State = RESERVED; }
  inline bool IsReserved() const { return _State == RESERVED; }
  inline void Release_Start() { _State = RELEASING; }
  inline void Release_Complete() { _State = IDLE; }

};

typedef std::shared_ptr<VirtualNetworkChannel> VirtualNetworkChannel_sptr;

}  // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_VIRTUAL_NETWORK_CHANNEL_H_ */
