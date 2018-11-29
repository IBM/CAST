/*================================================================================

    csmd/src/daemon/include/csm_bds_manager.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_BDS_MANAGER_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_BDS_MANAGER_H_

#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>

#include <signal.h>
#include <time.h>

#include <boost/thread.hpp>

#include "logging.h"

#include "include/csm_core_event.h"
#include "include/csm_bds_event.h"

#include "include/csm_event_sink.h"
#include "include/csm_event_source.h"
#include "include/csm_event_manager.h"

#include "include/csm_retry_backoff.h"
#include "throttle.h" // timetype definition

namespace csm {
namespace daemon {

typedef csm::daemon::TimeType BDSTimeType;

typedef struct
{
  BDSTimeType _TimeStamp;
  std::string _BDSMsg;
} BDSRetryEntry_t;

typedef std::deque<const BDSRetryEntry_t*> BDSRetryCacheQueue;

class EventManagerBDS : public EventManager
{
  BDS_Info _BDS_Info;
  boost::thread * _Thread;
  volatile std::atomic<bool> _KeepThreadRunning;
  std::mutex _ThreadGreenlightLock;
  std::condition_variable _ThreadGreenlightCondition;
  std::atomic_bool _ReadyToRun;
  csm::daemon::RetryBackOff _IdleRetryBackOff;

  BDSTimeType _LastConnect;
  unsigned _LastConnectInterval;
  int _Socket;

  BDSRetryCacheQueue _CachedMsgQueue;
  BDSTimeType _CurrentTime;

public:
  EventManagerBDS( const BDS_Info &i_BDS_Info,
                   RetryBackOff *i_MainIdleLoopRetry );
  virtual ~EventManagerBDS();

  virtual int Freeze()
  {
    LOG( csmd, trace ) << "BDSMgr: Freeze... state=" << _ReadyToRun;
    _ReadyToRun = false;
    _Thread->interrupt();
    return 0;
  }

  virtual int Unfreeze()
  {
    LOG( csmd, trace ) << "BDSMgr: Unfreeze...? " << _ReadyToRun;
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
      LOG( csmd, trace ) << "BDSMgr: GreenLightWait condition met..? " << _ReadyToRun;
    }
  }

  inline bool GetThreadKeepRunning() const { return _KeepThreadRunning; }
  inline csm::daemon::RetryBackOff *GetRetryBackoff() { return &_IdleRetryBackOff; }

  inline bool BDSActive() const { return _BDS_Info.Active(); }

  bool CheckConnectivity();
  bool Connect();
  bool SendData( const std::string data );


  bool AddToCache( const std::string bsd_msg );

private:
  inline void UpdateCurrentTime() { _CurrentTime = std::chrono::steady_clock::now(); }
  inline BDSTimeType GetTimestamp() { return _CurrentTime; };
  void CheckCachedData();
};

}   // namespace daemon
}  // namespace csm



#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_TIMER_MANAGER_H_ */
