/*================================================================================

    csmd/src/daemon/include/csm_retry_backoff.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_RETRY_BACKOFF_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_RETRY_BACKOFF_H_

#include <unistd.h>
#include <signal.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <boost/thread.hpp>

#include "logging.h"
#include "csmnet/src/CPP/reliable_msg.h"
#include "include/csm_daemon_exception.h"

namespace csm {
namespace daemon {


class RetryBackOff
{
public:
  typedef enum {
    NOOP,
    MICRO_SLEEP,
    INTERRUPTIBLE_SLEEP,
    CONDITIONAL,
    SIGNAL,
    SOCKET
  } SleepType;
private:
  std::string _Identifier;
  uint32_t _SlowMeDown;
  uint32_t _MaxBackoff;
  uint32_t _SlowDownBaseFactor;
  uint32_t _SlowDownOffset;
  SleepType _SleepType;
  SleepType _JobSleepType;
  volatile SleepType _ActualSleep;

  std::mutex _GreenlightLock;
  std::condition_variable _GreenlightCondition;
  volatile std::atomic_bool _ReadyToGo;

  sigset_t _SignalSet;
  csm::network::ReliableMsg *_Network;

  boost::thread *_CreatorThread;

public:
  RetryBackOff()
  : _Identifier("empty"),
    _SlowMeDown( 0 ), _MaxBackoff( 1 ), _SlowDownBaseFactor( 10000 ), _SlowDownOffset( 0 ),
    _SleepType( MICRO_SLEEP ), _JobSleepType( INTERRUPTIBLE_SLEEP ), _ActualSleep( MICRO_SLEEP ),
    _ReadyToGo( false ),
    _Network( nullptr ),
    _CreatorThread( nullptr )
  {}

  RetryBackOff( const std::string & i_Identifier,
                const SleepType i_SleepType,
                const SleepType i_JobSleepType,
                const uint32_t i_MaxBackoff = 0,
                const uint32_t i_BaseSleep = 10000,
                const uint32_t i_PollLoops = 0,
                csm::network::ReliableMsg *i_Network = nullptr );

  ~RetryBackOff()
  {
    LOG( csmd, debug ) << "Destroying RetryBackOff: " << _Identifier;
  }

  inline void SetThread( boost::thread *i_CreatorThread )
  {
    _CreatorThread = i_CreatorThread;
  }

  bool AgainOrWait( const bool Running_Job_Mode = false );
  inline void Reset() { _SlowMeDown = 0; }
  void WakeUp( const bool i_Sighandler = false );

  inline uint32_t GetSleepTime() const
  {
    uint32_t slowFactor = _SlowMeDown - _SlowDownOffset;
    return _SlowDownBaseFactor * slowFactor * slowFactor;
  }

  // allows a caller-specified usleep based on the configured sleep type
  // returns true if sleep was interrupted
  bool uSleep( const uint64_t i_MicroSecs );

  bool JoinThread( boost::thread *i_Thread,
                   const int i_Index = -1 );

private:
  bool WaitConditional();
};

}
}

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_RETRY_BACKOFF_H_ */
