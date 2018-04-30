/*================================================================================

    csmd/src/daemon/src/csm_event_sources/csm_source_timer.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_TIMER_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_TIMER_H_

#include <mutex>

#include "include/csm_timer_event.h"
#include "include/csm_event_source.h"

namespace csm {
namespace daemon {

typedef std::deque<const csm::daemon::TimerEvent*> TimerEventQueue;

class EventSourceTimer: public csm::daemon::EventSource
{
public:
  EventSourceTimer( RetryBackOff *aRetryBackoff )
  : csm::daemon::EventSource( aRetryBackoff, TIMER_SRC_ID, false )
  { }
  
  virtual ~EventSourceTimer() {}

  virtual csm::daemon::CoreEvent* GetEvent( const std::vector<uint64_t> &i_BucketList )
  {
    std::lock_guard<std::mutex> guard( _QueueLock );
     
    csm::daemon::TimerEvent *timerEvent = nullptr;
    if ( !_TimerQueue.empty() )
    {
      timerEvent = const_cast<csm::daemon::TimerEvent*>( _TimerQueue.front() );
      if( timerEvent == nullptr )
        return nullptr;

      _TimerQueue.pop_front();
    }
    return timerEvent;
  }

  virtual bool QueueEvent( const csm::daemon::CoreEvent *i_Event )
  {
    std::lock_guard<std::mutex> guard( _QueueLock );
    _TimerQueue.push_back( dynamic_cast<const csm::daemon::TimerEvent*>( i_Event ) );
    return WakeUpMainLoop();
  }

private:
  TimerEventQueue _TimerQueue;
  std::mutex _QueueLock;
};

}    // namespace daemon
}   // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_TIMER_H_ */
