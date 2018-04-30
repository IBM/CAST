/*================================================================================

    csmd/src/daemon/include/csm_timer_manager.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_TIMER_MANAGER_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_TIMER_MANAGER_H_

#include <mutex>
#include <condition_variable>
#include <memory>

#include <signal.h>
#include <time.h>

#include <boost/thread.hpp>

#include "logging.h"

#include "include/csm_core_event.h"
#include "include/csm_timer_event.h"

#include "include/csm_event_sink.h"
#include "include/csm_event_source.h"
#include "include/csm_event_manager.h"

#include "include/csm_retry_backoff.h"

namespace csm {
namespace daemon {

class EventManagerTimer : public EventManager
{
  csm::daemon::TimerQueueType _TimerQueue;
  boost::thread * _Thread;
  volatile std::atomic<bool> _KeepThreadRunning;
  std::mutex _ThreadGreenlightLock;
  std::condition_variable _ThreadGreenlightCondition;
  std::atomic_bool _ReadyToRun;
  csm::daemon::RetryBackOff _IdleRetryBackOff;

public:
  EventManagerTimer( csm::daemon::RetryBackOff *i_MainIdleLoopRetry );
  virtual ~EventManagerTimer();

  virtual int Freeze()
  {
    LOG( csmd, trace ) << "TimerMgr: Freeze... state=" << _ReadyToRun;
    _ReadyToRun = false;
    _Thread->interrupt();
    return 0;
  }

  virtual int Unfreeze()
  {
    LOG( csmd, trace ) << "TimerMgr: Unfreeze...? " << _ReadyToRun;
    std::unique_lock<std::mutex> glock( _ThreadGreenlightLock );
    _ReadyToRun = true;
    _ThreadGreenlightCondition.notify_all();
    return 0;
  }

  void GreenLightWait()
  {
    std::unique_lock<std::mutex> glock( _ThreadGreenlightLock );
    while( !_ReadyToRun )
    {
      _ThreadGreenlightCondition.wait( glock );
      LOG( csmd, trace ) << "TimerMgr: GreenLightWait condition met..? " << _ReadyToRun;
    }
  }

  inline bool GetThreadKeepRunning() const { return _KeepThreadRunning; }
  inline csm::daemon::RetryBackOff *GetRetryBackoff() { return &_IdleRetryBackOff; }

};

}   // namespace daemon
}  // namespace csm



#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_TIMER_MANAGER_H_ */
