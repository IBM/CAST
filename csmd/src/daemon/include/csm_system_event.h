/*================================================================================

    csmd/src/daemon/include/csm_system_event.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_SYSTEM_EVENT_H_
#define CSM_DAEMON_SRC_CSM_SYSTEM_EVENT_H_

#include <chrono>
#include <ctime>
#include <queue>
#include <mutex>

#include "include/csm_core_event.h"
#include "logging.h"

#include "csmnet/src/CPP/address.h"

namespace csm {
namespace daemon {

  
class SystemContent
{
public:
  enum SIGNAL_TYPE
  {
    CONNECTED,
    DISCONNECTED,
    RETRY_CONNECT,
    RESTARTED,
    FAILOVER,
    RESET_AGG,
    FATAL,
    JOB_START,
    JOB_END,
    DAEMONS_START,
    ACK_TIMEOUT,
    UNDEFINED
  };
  
  SystemContent(const SIGNAL_TYPE aSingle, const csm::network::Address_sptr aAddr)
  : _Signal(aSingle)
  {
    _Addr = csm::network::Address_sptr( aAddr );
  }
  
  SystemContent(const SIGNAL_TYPE aSingle)
  : _Signal(aSingle), _Addr(nullptr)
  { }
  
  SystemContent( const SystemContent &in )
  : _Signal( in._Signal ),
    _Addr( in._Addr )
  {
  }
  
  SIGNAL_TYPE GetSignalType() { return _Signal; };
  
  csm::network::Address_sptr GetAddr() { return _Addr; }
  
  virtual ~SystemContent() {}
  
private:
  SIGNAL_TYPE _Signal;
  csm::network::Address_sptr _Addr;

};

typedef CoreEventBase<csm::daemon::SystemContent> SystemEvent;


template<class stream>
static stream&
operator<<( stream &out, const SystemContent::SIGNAL_TYPE &aType )
{
  switch( aType )
  {
    case SystemContent::CONNECTED:    out << "CONNECTED";    break;
    case SystemContent::DISCONNECTED: out << "DISCONNECTED"; break;
    case SystemContent::RETRY_CONNECT: out << "RETRY_CONNECT"; break;
    case SystemContent::RESTARTED: out << "RESTARTED"; break;
    case SystemContent::FAILOVER: out << "FAILOVER"; break;
    case SystemContent::RESET_AGG: out << "RESET_AGG"; break;
    case SystemContent::FATAL: out << "FATAL"; break;
    case SystemContent::JOB_START: out << "JOB_START"; break;
    case SystemContent::JOB_END: out << "JOB_END"; break;
    case SystemContent::DAEMONS_START: out << "DAEMONS_START"; break;
    case SystemContent::ACK_TIMEOUT: out << "ACK_TIMEOUT"; break;
    case SystemContent::UNDEFINED: out << "UNDEFINED"; break;
    default:                 out << "SystemContent::SIGNAL_TYPE: ERROR: !!!OUT-OF-RANGE!!!";
  }
  return (out);
}


} //namespace daemon
} //namespace csm

#endif  // CSM_DAEMON_SRC_CSM_TIMER_EVENT_H_
