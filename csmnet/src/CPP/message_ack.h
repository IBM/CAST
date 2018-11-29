/*================================================================================

    csmnet/src/CPP/message_ack.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_MESSAGE_ACK_H_
#define CSMNET_SRC_CPP_MESSAGE_ACK_H_

#include <chrono>
#include <ctime>
#include <queue>
#include "csmnet/include/csm_timing.h" // CSM_NETWORK_ACK_TIMEOUT

namespace csm {
namespace network {

typedef std::string (*PubSubTopicRewriteFunction)(const std::string i_Input);

#define REPLY_BIT_SHIFT ( 63 )

/*
 * need 2 data structures
 * - look up timeout (queue sorted by end timepoint)
 * - look up messageID (unordered map)
 */

class AckKeyType
{
  uint64_t _msgID;
  bool _resp;
public:
  AckKeyType( const uint64_t id = 0, const bool re = false )
  : _msgID( id ), _resp( re )
  {}

  AckKeyType( const AckKeyType &in )
  : _msgID( in._msgID ), _resp( in._resp )
  {}

  AckKeyType& operator=( const AckKeyType &in )
  { _msgID = in._msgID; _resp = in._resp; return *this; }

  bool operator==( const AckKeyType &in ) const
  { return (( _msgID == in._msgID ) && ( _resp == in._resp )); }

  bool operator!=( const AckKeyType &in ) const
  { return ! ( *this == in ); }

  inline uint64_t GetMsgID() const { return _msgID; }
  inline bool GetResp() const { return _resp; }
};

template<class stream>
static stream&
operator<<( stream &out, const AckKeyType &in )
{
  out << in.GetMsgID() << "|" << in.GetResp();
  return out;
}

struct AckKeyTypeHash
{
    std::size_t operator()( const AckKeyType &in ) const noexcept
    {
      return ((size_t)in.GetResp() << REPLY_BIT_SHIFT) + in.GetMsgID();
    }
};

class MessageACK
{
public:
  typedef std::unordered_set<AckKeyType, AckKeyTypeHash> AckSet;

private:
  typedef std::chrono::time_point< std::chrono::steady_clock > TimeType;

  class TimedMessageType
  {
  public:
    TimeType  _Timeout;
    AckKeyType  _Key;
    csm::network::Address_sptr _Addr;
    TimedMessageType( const TimeType &aTimeout,
                      const AckKeyType aKey,
                      const csm::network::Address_sptr aAddr )
    : _Timeout( aTimeout ), _Key( aKey ), _Addr( aAddr )
    {}
  };
  typedef std::queue<TimedMessageType> TimerQueueType;

  AckSet _AckSet;
  uint64_t _DefaultTimeout;
  TimerQueueType _TimerQueue;
  TimeType _CurrentClock;

public:
  MessageACK()
  : _DefaultTimeout( CSM_NETWORK_ACK_TIMEOUT )
  {}
  ~MessageACK() {}

  void SetDefaultTimeout( const uint64_t aTimeout ) { _DefaultTimeout = aTimeout; }
  void UpdateClock() { _CurrentClock = std::chrono::steady_clock::now(); }

  csm::network::MessageAndAddress* CheckCreateACKMsg( const csm::network::MessageAndAddress& aMsgAddr,
                                                      const csm::network::Address_sptr aDestAddr,
                                                      const bool KeepErrorFlag = false )
  {
    // parse msg header to determine ACK or not
    // if unknown message:
    //    - parse header (compare self-address with dst addr in header)
    //    - create msg ACK (consider coalescing later)
    //    -
    const csm::network::Message *msg = &aMsgAddr._Msg;

    if( msg->RequiresACK() )
    {
      csm::network::MessageAndAddress *ret = new csm::network::MessageAndAddress( aMsgAddr );
      if( ret == nullptr )
        throw csm::network::Exception("Failed to allocate ACK msg.");

      // if not a NACK
      if(ret->_Msg.GetErr() && !KeepErrorFlag)
        ret->_Msg.ClrErr();

      ret->_Msg.SetAck();
      ret->_Msg.SetPriority( CSM_PRIORITY_NO_ACK );
      if(!(ret->_Msg.GetErr()))
        ret->_Msg.SetData("");  // includes checksum update
      else
        ret->_Msg.CheckSumUpdate(); // don't overwrite NACK message
      ret->SetAddr( aDestAddr );

      return ret;
    }
    return nullptr;
  }

  std::pair< AckKeyType, csm::network::Address_sptr> CheckTimeout()
  {
    if( _TimerQueue.empty() )
      return std::make_pair( AckKeyType(), nullptr );

    TimedMessageType timeData = _TimerQueue.front();
    if( timeData._Timeout <= _CurrentClock )
    {
      AckKeyType key = timeData._Key;
      csm::network::Address_sptr addr = timeData._Addr;
      _TimerQueue.pop();

      AckSet::iterator pos_itr = _AckSet.find( key );
      if( pos_itr != _AckSet.end() )
      {
        AckKeyType ret = *pos_itr;
        _AckSet.erase( key );
        LOG( csmnet, debug ) << "Timeout ACK for msgID|resp:" << ret
            << " TimeoutTS: " << timeData._Timeout.time_since_epoch().count();
        if( ret != key )
        {
          LOG( csmnet, error ) << "ACK KEY MISMATCH:" << ret << " != " << key;
          throw csm::network::ExceptionProtocol("ACK KEY MISMATCH:");
        }
        return std::make_pair( ret, timeData._Addr );
      }
      else
        LOG( csmnet, trace ) << "Cleaning entry for completed ACK msgID|resp: " << key
        << " TimeoutTS: " << timeData._Timeout.time_since_epoch().count();
    }
    return std::make_pair( AckKeyType(), nullptr );
  }

  bool RegisterAck( const csm::network::MessageAndAddress &aMsgAddr )
  {
    bool rc = true;
    // everything's fine if we don't need to register an ACK
    if( ! aMsgAddr._Msg.RequiresACK() )
      return true;

    AckKeyType key = MakeKey( aMsgAddr._Msg.GetMessageID(), aMsgAddr._Msg.GetResp() );
    _AckSet.insert( key );
    TimeType ts = CalculateEndTime(_DefaultTimeout);
    _TimerQueue.push( TimedMessageType( ts,
                                        key,
                                        aMsgAddr.GetAddr() ) );
    LOG( csmnet, debug ) << "Registering ACK for msgID|resp: " << key
        << " TimeoutTS: " << ts.time_since_epoch().count();

    return rc;
  }

  bool AckReceived( const csm::network::Message &aMsg )
  {
    // any acks that are either known or caused by STATUS msgs are considered "good"
    bool rc = (( 1 == _AckSet.erase( MakeKey( aMsg.GetMessageID(), aMsg.GetResp() ) ) ) ||
        ( aMsg.GetCommandType() == CSM_CMD_STATUS ));
    LOG( csmnet, debug ) << "Received ACK for msgID|resp:" << aMsg.GetMessageID() << "|" << aMsg.GetResp() << " : known=" << rc;
    // timerqueue will be cleaned up as timers expire (no need to traverse queue for items to delete)

    return rc;
  }

  uint64_t GetAckCount() const
  {
    return _AckSet.size();
  }

private:
  TimeType CalculateEndTime( const uint64_t aTimeout ) const
  {
    TimeType end = std::chrono::steady_clock::now() + std::chrono::seconds( aTimeout );
    return end;
  }
  inline AckKeyType MakeKey( const uint64_t aMsgId, const uint32_t aReply, const uint32_t aRefCount = 1 )
  {
    return AckKeyType( aMsgId, aReply>0 );
  }
};

}    // network
}   // csm

#endif /* CSMNET_SRC_CPP_MESSAGE_ACK_H_ */
