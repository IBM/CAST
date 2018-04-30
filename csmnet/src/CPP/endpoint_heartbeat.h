/*================================================================================

    csmnet/src/CPP/endpoint_heartbeat.h

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_HEARTBEAT_H_
#define CSMNET_SRC_CPP_ENDPOINT_HEARTBEAT_H_

#include <chrono>
#include <iostream>

#ifndef logprefix
#define logprefix "HEARTBEAT"
#define logprefix_local
#endif
#include "csm_pretty_log.h"

#include "csm_network_config.h"
#include "csm_network_msg_cpp.h"
#include "csmutil/include/csm_version.h"

namespace csm {
namespace network {

class EndpointHeartbeat_stateless
{
public:
  typedef std::chrono::time_point< std::chrono::system_clock > TimeType;

protected:
  TimeType _EndOfHappy; // at the end of the happy, we send a heartbeat
  uint32_t _Interval; // to be the approx. interval of sending msgs
  csm::network::Message _Msg;
  Address_sptr _Address;

public:
  EndpointHeartbeat_stateless( uint32_t aInterval = CSM_NETWORK_HEARTBEAT_INTERVAL,
                               uint8_t aMsgPrio = CSM_PRIORITY_NO_ACK )
  : _Interval( aInterval )
  {
    updateRecvSuccess();
    setMsg( aMsgPrio );
    _Address = nullptr;
    CSMLOG( csmnet, trace ) << "Creating heartbeat with initial interval: " << _Interval;
  }
  EndpointHeartbeat_stateless( const EndpointHeartbeat_stateless &hb )
  : _Interval( hb.getInterval() ),
    _Msg( hb.getMsg() )
  {
    updateRecvSuccess();
    CSMLOG( csmnet, trace ) << "Creating heartbeat with initial interval: " << _Interval;
  }
  virtual ~EndpointHeartbeat_stateless() {}

  EndpointHeartbeat_stateless& operator=( const EndpointHeartbeat_stateless &hb )
  {
    _Interval = hb.getInterval();
    _Msg = hb.getMsg();
    _EndOfHappy = hb.getDeadline();
    return *this;
  }

  const TimeType& getDeadline() const
  {
    return _EndOfHappy;
  }

  void setInterval( uint32_t interval )
  {
    if( interval == 0 )
    {
      CSMLOG( csmnet, warning ) << ( _Address == nullptr ? "" : _Address->Dump() )
          << " Daemon-to-daemon heartbeat interval attempt to set to 0s. Using default: "
          << CSM_NETWORK_HEARTBEAT_INTERVAL;
      _Interval = CSM_NETWORK_HEARTBEAT_INTERVAL;
    }
    else
      _Interval = interval;
    CSMLOG( csmnet, debug ) << ( _Address == nullptr ? "" : _Address->Dump() )
        << " Daemon-to-daemon heartbeat interval set to " << _Interval << " sec.";
    updateRecvSuccess();
    _Msg.SetReservedID( _Interval );
    _Msg.CheckSumUpdate();
  }

  uint32_t getInterval() const
  {
    return _Interval;
  }

  const csm::network::Message& getMsg() const
  {
    return _Msg;
  }

  void setMsg( uint8_t msgPrio )
  {
    _Msg.Init( CSM_CMD_HEARTBEAT,
               CSM_HEADER_INT_BIT | CSM_HEADER_CBK_BIT,
               msgPrio,
               random(),
               0x1234,
               0x1234,
               geteuid(), getegid(),
               std::string( CSM_VERSION ),
               _Interval );
  }

  void setAddr( const Address_sptr addr )
  {
    _Address = addr;
  }

  // is it time to send the next heartbeat
  virtual bool dueSend( TimeType & reference )
  {
    return ( _EndOfHappy < reference );
  }

  virtual void updateRecvSuccess()
  {
    // make the end of happy about 75% of the configured Interval
    TimeType current = std::chrono::system_clock::now();
    _EndOfHappy = current + std::chrono::seconds( _Interval );
  }

  // update the send deadline; e.g. after a successful send operation
  virtual void updateSendSuccess()
  {
    updateRecvSuccess();
  }


};



class EndpointHeartbeat : public EndpointHeartbeat_stateless
{
    // XXX @lars is this fine to make public?
public:
  typedef enum {
    UNINITIALIZED = 0,
    HAPPY = 1,
    UNSURE = 2,
    MISSING_RECV = 3,
    DEAD = 4
  } HeartbeatStatus_t;

private:

  TimeType _EndOfUnsure; // at the end of unsure we send the second heartbeat
  mutable HeartbeatStatus_t _Status;
  void StatusTransition()
  {
    switch( _Status )
    {
      case UNINITIALIZED: _Status = HAPPY; break;
      case HAPPY: _Status = UNSURE; break;
      case UNSURE: _Status = MISSING_RECV; break;
      case MISSING_RECV: _Status = DEAD; break;
      case DEAD: _Status = DEAD; break;
      default: break;
    }
  }

public:
  EndpointHeartbeat( uint32_t aInterval = CSM_NETWORK_HEARTBEAT_INTERVAL,
                     uint8_t aMsgPrio = CSM_PRIORITY_NO_ACK )
  : EndpointHeartbeat_stateless( aInterval, aMsgPrio ),
    _Status( UNINITIALIZED )
  {}
  EndpointHeartbeat( const EndpointHeartbeat &hb )
  : EndpointHeartbeat_stateless( dynamic_cast<const EndpointHeartbeat_stateless&>( hb )),
    _Status( hb._Status )
  {}
  virtual ~EndpointHeartbeat() {}
  
  EndpointHeartbeat& operator=( const EndpointHeartbeat &hb )
  {
    _Interval = hb.getInterval();
    _Msg = hb.getMsg();
    _EndOfHappy = hb.getDeadline();
    _Status = hb._Status;
    return *this;
  }

  // is it time to send the next heartbeat
  virtual bool dueSend( TimeType & reference )
  {
    if( ( (_Status == HAPPY) && (_EndOfHappy < reference ) ) ||
        ( ((_Status == UNSURE) || (_Status == MISSING_RECV)) && ( _EndOfUnsure < reference )) )  
    {
      HeartbeatStatus_t old = _Status;
      StatusTransition();
      CSMLOG( csmnet, debug ) << ( _Address == nullptr ? "" : _Address->Dump() ) << " send of heartbeat required. Status: "
          << old << " -> " << _Status;
      return true;
    }
    return false;
  }

  // consider remote end to be dead if last successful recv was more than 2x_Interval seconds ago
  const bool peerDead( TimeType & reference )
  {
    bool dead = (_EndOfUnsure < reference) && ( _Status == MISSING_RECV );
    if( dead )
    {
      StatusTransition();
      CSMLOG( csmnet, warning ) << ( _Address == nullptr ? "" : _Address->Dump() ) << " Pronounced: " << _Status;
    }
    return dead;
  }

  virtual void updateRecvSuccess()
  {
    // make the end of unsure about 125% of the configured Interval
    TimeType current = std::chrono::system_clock::now();
    _EndOfHappy = current + std::chrono::seconds( _Interval );
    _EndOfUnsure = current + std::chrono::seconds( (_Interval * 20) >> 4 );
    _Status = HAPPY;
    CSMLOG( csmnet, debug)  << ( _Address == nullptr ? "" : _Address->Dump() ) << " Received a message. Status: " << _Status;
  }

  // update the send deadline; e.g. after a successful send operation
  virtual void updateSendSuccess()
  {
    TimeType current = std::chrono::system_clock::now();
    _EndOfUnsure = current + std::chrono::seconds( (_Interval * 20) >> 4 );
    CSMLOG( csmnet, debug ) << ( _Address == nullptr ? "" : _Address->Dump() ) << " Successful send. Status: " << _Status;
  }

};

template<class stream>
static stream&
operator<<( stream &out, const EndpointHeartbeat::HeartbeatStatus_t &aType )
{
  switch( aType )
  {
    case EndpointHeartbeat::HeartbeatStatus_t::UNINITIALIZED:  out << "UNINITIALIZED";      break;
    case EndpointHeartbeat::HeartbeatStatus_t::HAPPY:          out << "HAPPY";              break;
    case EndpointHeartbeat::HeartbeatStatus_t::UNSURE:         out << "UNSURE";             break;
    case EndpointHeartbeat::HeartbeatStatus_t::MISSING_RECV:   out << "MISSING_RECV";       break;
    case EndpointHeartbeat::HeartbeatStatus_t::DEAD:           out << "DEAD";               break;
    default:
      out << "ERROR: !!!OUT-OF-RANGE!!!";
  }
  return (out);
}




}  // network
} // csm

#ifdef logprefix_local
#undef logprefix
#undef logprefix_local
#endif

#endif /* CSMNET_SRC_CPP_ENDPOINT_HEARTBEAT_H_ */
