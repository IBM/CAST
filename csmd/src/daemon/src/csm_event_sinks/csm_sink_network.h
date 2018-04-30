/*================================================================================

    csmd/src/daemon/src/csm_event_sinks/csm_sink_network.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_NETWORK_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_NETWORK_H_

#include <mutex>
#include <deque>

#include "include/csm_network_event.h"

namespace csm {
namespace daemon {

class EventSinkNetwork : public csm::daemon::EventSink
{
  csm::daemon::RetryBackOff *_ManagerWakeup;
  NetworkEventQueue _Outbound;
  std::mutex _OutboundLock;

public:
  EventSinkNetwork(  csm::daemon::RetryBackOff *i_MgrWakeup )
  : _ManagerWakeup( i_MgrWakeup )
  { }
  virtual ~EventSinkNetwork()
  { }
  virtual int PostEvent( const csm::daemon::CoreEvent &aEvent )
  {
    _OutboundLock.lock();
    _Outbound.push_back( &aEvent );
    _OutboundLock.unlock();
    _ManagerWakeup->WakeUp();
    return 0;
  }

  virtual csm::daemon::CoreEvent* FetchEvent()
  {
    csm::daemon::NetworkEvent *nwe = nullptr;
    std::lock_guard<std::mutex> guard( _OutboundLock );
    if( ! _Outbound.empty() )
    {
      nwe = (csm::daemon::NetworkEvent *) _Outbound.front();
      _Outbound.pop_front();
    }

    return nwe;
  }
};

}   // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_NETWORK_H_ */
