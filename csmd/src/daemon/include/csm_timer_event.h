/*================================================================================

    csmd/src/daemon/include/csm_timer_event.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_TIMER_EVENT_H_
#define CSM_DAEMON_SRC_CSM_TIMER_EVENT_H_

#include <chrono>
#include <ctime>
#include <list>
#include <mutex>

#include "include/csm_core_event.h"
#include "logging.h"

namespace csm {
namespace daemon {

class TimerContent
{
public:
  typedef std::chrono::time_point< std::chrono::system_clock > TimeType;
  
  TimerContent(
    const uint64_t& aMilliSeconds, 
    uint64_t aTargetState = UINT64_MAX ) : 
        _timerInterval(aMilliSeconds),
        _targetStateId(aTargetState)
  {
    _startTime = std::chrono::system_clock::now();
    _endTime =  _startTime + std::chrono::milliseconds(_timerInterval);
  }
  
  TimerContent( const TimerContent &in )
  : _timerInterval( in._timerInterval ),
    _targetStateId( in._targetStateId ),
    _endTime( in._endTime ),
    _startTime( in._startTime )
  { }
  
  // keep this for debugging purpose
  uint64_t GetTimerInterval() const { return _timerInterval; }

  uint64_t GetTargetStateId() const { return _targetStateId; }
  
  TimeType GetEndTime() const
  {
    return _endTime;
  }
  
  TimeType GetStartTime() const
  {
    return _startTime;
  }

  bool TimerExpired() const
  {
#if 0
     if (std::chrono::system_clock::now() >= _endTime)
     {
       std::time_t tt_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
       std::time_t tt_end = std::chrono::system_clock::to_time_t(_endTime);
       std::time_t tt_start = std::chrono::system_clock::to_time_t(_startTime);
       LOG(csmd, debug) << "now: " << std::ctime(&tt_now) << " end: " << std::ctime(&tt_end) << " start: " << std::ctime(&tt_start) << " interval: " << _timerInterval;
     }
#endif     
     return ( std::chrono::system_clock::now() >= _endTime );
  }
  
  int64_t RemainingMicros() const
  {
    return std::chrono::duration_cast<std::chrono::microseconds>( _endTime - std::chrono::system_clock::now() ).count();
  }
  
  virtual ~TimerContent()
  { }
  
private:
  uint64_t _timerInterval; ///< The timer length in miliseconds
  uint64_t _targetStateId; ///< The target state id for the timer ( used in handler).
  TimeType _endTime;       ///< The computed end time of this timer.
  TimeType _startTime;     ///< The start time of this timer.

};

typedef CoreEventBase<csm::daemon::TimerContent> TimerEvent;

class timer_comparator {
public:
  bool operator() (csm::daemon::TimerEvent *a, csm::daemon::TimerEvent *b)
  {
    int64_t timediff = std::chrono::duration_cast<std::chrono::microseconds>(
        a->GetContent().GetEndTime() - b->GetContent().GetEndTime() ).count();

    return ( timediff < 0 );
  }
};


typedef std::list<TimerEvent *> TimerQueueType;

} //namespace daemon
} //namespace csm

#endif  // CSM_DAEMON_SRC_CSM_TIMER_EVENT_H_
