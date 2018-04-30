/*================================================================================

    csmd/src/daemon/src/csm_event_sources/csm_source_network.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_NETWORK_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_NETWORK_H_

#include "include/csm_network_event.h"

namespace csm {
namespace daemon {

class EventSourceNetwork : public csm::daemon::EventSource
{
  NetworkEventQueue _Inbound;
  std::mutex _InboundLock;

public:
  EventSourceNetwork( RetryBackOff *aRetryBackoff )
  : csm::daemon::EventSource( aRetryBackoff, NETWORK_SRC_ID, false)
  {
  }
  virtual ~EventSourceNetwork() {}

  virtual csm::daemon::CoreEvent* GetEvent( const std::vector<uint64_t> &i_BucketList )
  {
    csm::daemon::CoreEvent *nwe = nullptr;
    _InboundLock.lock();
    if( !_Inbound.empty() )
    {
      nwe = const_cast<csm::daemon::CoreEvent*>( _Inbound.front() );
      _Inbound.pop_front();
    }
    _InboundLock.unlock();

    return nwe;
  }

  virtual bool QueueEvent( const csm::daemon::CoreEvent *i_Event )
  {
    _InboundLock.lock();
    _Inbound.push_back( i_Event );
    _InboundLock.unlock();
    return WakeUpMainLoop();
  }

};

}   // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_NETWORK_H_ */
