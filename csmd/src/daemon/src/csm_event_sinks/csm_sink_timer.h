/*================================================================================

    csmd/src/daemon/src/csm_event_sinks/csm_sink_timer.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_TIMER_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_TIMER_H_

#include <queue>
#include <mutex>

#include "include/csm_timer_event.h"
#include "include/csm_event_sink.h"

#include "logging.h"

namespace csm {
namespace daemon {

class EventSinkTimer : public csm::daemon::EventSink
{

public:
  EventSinkTimer( csm::daemon::RetryBackOff *aMgrWakeup )
  : _MgrWakeup( aMgrWakeup )
  { }
  
  virtual ~EventSinkTimer()
  {
    // make sure we drain the queue when exiting
    {
      std::lock_guard<std::mutex> guard( _TimerQueueLock );
      _TimerQueue.clear();
    }
  }
  
  virtual int PostEvent( const csm::daemon::CoreEvent &aEvent )
  {
    //LOG(csmd, info) << "EventSinkTimer: PostEvent...";
    csm::daemon::TimerEvent *event = (csm::daemon::TimerEvent *)&aEvent;
    LOG(csmd, trace ) << "TimerQueue: Queuing event. end="
        << event->GetContent().GetEndTime().time_since_epoch().count()
        << " queuelen=" << _TimerQueue.size();
    
    Insert( event );
//    Dump();
    _MgrWakeup->WakeUp();
    return 0;
  }

  virtual csm::daemon::CoreEvent* FetchEvent()
  {
    std::lock_guard<std::mutex> guard( _TimerQueueLock );
    if( ! _TimerQueue.empty() )
    {
      csm::daemon::TimerEvent *tev = const_cast<csm::daemon::TimerEvent*>( _TimerQueue.front() );
      _TimerQueue.pop_front();
      LOG(csmd, trace ) << "TimerQueue: Fetching event. end="
          << tev->GetContent().GetEndTime().time_since_epoch().count()
          << " queuelen=" << _TimerQueue.size();
//      Dump();
      return tev;
    }
    else
      return nullptr;
  }

  // special for timer event sink: return event to front of timer queue
  // in case a new timer got queued while waiting for another one
  inline void RestoreEvent(csm::daemon::TimerEvent *tev)
  {
    LOG(csmd, trace ) << "TimerQueue: restoring event, timer not yet expired. end="
        << tev->GetContent().GetEndTime().time_since_epoch().count();
    Insert( tev );
//    Dump();
  }

private:
  void Insert( csm::daemon::TimerEvent *tev )
  {
    std::lock_guard<std::mutex> guard( _TimerQueueLock );

    int64_t evTime = tev->GetContent().GetEndTime().time_since_epoch().count();

    auto it = _TimerQueue.begin();

    // check if new timer would be the only or the last entry
    if(( it == _TimerQueue.end() ) ||
        ( _TimerQueue.back()->GetContent().GetEndTime().time_since_epoch().count() < evTime ))
    {
      _TimerQueue.push_back( tev );
      return;
    }

    // otherwise scan for the correct position
    while(( it != _TimerQueue.end() &&
          ( (*it)->GetContent().GetEndTime().time_since_epoch().count() < evTime )) )
      it++;

    if( it == _TimerQueue.end() )
      _TimerQueue.push_back( tev );
    else
      _TimerQueue.insert( it, tev );
  }
  void Dump() const
  {
    for( auto it : _TimerQueue )
    {
      LOG( csmd, always ) << "TimerQueue: Entry=" << it->GetContent().GetEndTime().time_since_epoch().count()
          << " context=" << it->GetEventContext();
    }
  }


  csm::daemon::TimerQueueType _TimerQueue;
  std::mutex _TimerQueueLock;
  csm::daemon::RetryBackOff *_MgrWakeup;
};

}  // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_TIMER_H_ */
