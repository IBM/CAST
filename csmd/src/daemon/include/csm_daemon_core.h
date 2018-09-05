/*================================================================================

    csmd/src/daemon/include/csm_daemon_core.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_DAEMON_CORE_H_
#define CSM_DAEMON_SRC_CSM_DAEMON_CORE_H_

#include <strings.h>
#include <vector>
#include <unordered_set>

#include "csm_daemon_config.h"

#include "include/csm_core_event.h"
#include "include/csm_timer_event.h"
#include "include/csm_event_source.h"
#include "include/csm_event_source_set.h"
#include "include/csm_event_sink.h"
#include "include/csm_event_sink_set.h"
#include "include/csm_event_routing.h"

#include "include/jitter_window.h"
#include "include/csm_event_manager.h"
#include "include/csm_daemon_network_manager.h"
#include "include/csm_db_manager.h"
#include "include/csm_timer_manager.h"
#include "include/csm_bds_manager.h"

#include "src/csmi_request_handler/csmi_base.h"

#include "csmi/src/common/include/csmi_cmds.h"


#include "include/thread_pool.h"
#include "include/thread_manager.h"

#include "src/csm_event_sources/csm_source_environmental.h"
#include "src/csm_event_sources/csm_source_interval.h"

namespace csm {
namespace daemon {

class CoreGeneric
{
  
  // set up queues, event sources and sinks, runmodes, jitter windows
protected:
  csm::daemon::EventSourceSet _EventSources;
  csm::daemon::EventSinkSet _EventSinks;

  csm::daemon::EventManagerNetwork *_netMgr;
  csm::daemon::EventManagerDB *_dbMgr;
  csm::daemon::EventManagerTimer *_timerMgr;
  csm::daemon::EventManagerBDS *_bdsMgr;

  csm::daemon::EventSourceEnvironmental* _envSource;
  csm::daemon::EventSourceInterval* _intervalSrc;
  
  csm::daemon::EventRouting *_EventRouting;

  csm::daemon::ThreadManager *_threadMgr;

  std::unordered_set< int > _cmdsByMainThread;
  csm::daemon::Configuration *_Config;
  struct itimerval _WindowInterval;
  int _WindowMax;       // master will likely have only one window and no time limits
  volatile std::atomic_int _ActiveWindow;
  csm::daemon::RetryBackOff _IdleLoopRetry;

  sigset_t _SignalSet;
  
public:
  CoreGeneric();
  virtual ~CoreGeneric() { DestroyInfrastructure(); };


public:
  // call this only after _EventRouting is initialized by the daemons
  void InitInfrastructure(const csm::daemon::EventManagerNetwork *netMgr,
                          const csm::daemon::EventManagerDB *dbMgr,
                          const csm::daemon::EventManagerTimer *timerMgr,
                          const csm::daemon::EventManagerBDS *bdsMgr );
  inline bool AssignToWorkerThread(csmi_cmd_t aCmd)
  { return (_cmdsByMainThread.find((int) aCmd) == _cmdsByMainThread.end()); }
  
protected:
  void DestroyInfrastructure();
  void AddEventSource( const csm::daemon::EventSource *aNewSource,
                       const uint64_t aIdentifier,
                       const uint8_t aBucketID = 0,
                       const uint64_t aInterval = 1 );
  
public:
  csm::daemon::EventSinkSet& GetEventSinks()
  { return _EventSinks; }
  
  virtual csm::daemon::CoreEvent* Fetch( )
  {
    return _EventSources.Fetch( _ActiveWindow );
  }
  
  // The default JitterWindow function is just indicates "no job" for all daemon types but the compute agent
  // Other daemon types should also never get into a runmode that calls this function
  virtual int JitterWindow( const csm::daemon::JitterWindowAction i_Action = csm::daemon::WINDOW_NO_ADVANCE )
  {
    _ActiveWindow = ( _ActiveWindow + i_Action ) % _WindowMax;
    _IdleLoopRetry.WakeUp();
    return _ActiveWindow;
  }
  int GetWindowIndex() const { return _ActiveWindow; }
  
  virtual void manage_thread_RegisterThreadManager( csm::daemon::ThreadManager *i_ThreadMgr )
  { }
  
  inline void AddPublicAPIHandlers(csmi_cmd_t aCmd)
  {
    if (_EventRouting) _EventRouting->AddPublicAPIHandlers(aCmd);
  }
  
  inline csm::daemon::EventRouting* GetEventRouting()
  { return _EventRouting; }
 
  inline CSMI_BASE *GetErrorEventHandler()
  {
    if (_EventRouting) return _EventRouting->GetErrorEventHandler();
    else return nullptr;
  }
  
  inline CSMI_BASE *GetDefaultEventHandler()
  {
    if (_EventRouting) return _EventRouting->GetDefaultEventHandler();
    else return nullptr;
  }

  inline CSMI_BASE *GetEnvironmentalHandler()
  {
    if (_EventRouting) return _EventRouting->GetEnvironmentalHandler();
    else return nullptr;
  }
    
  inline CSMI_BASE *GetIntervalHandler()
  {
    if (_EventRouting) return _EventRouting->GetIntervalHandler();
    else return nullptr;
  }

  virtual CSMI_BASE *GetEventHandler(const csm::daemon::CoreEvent &aEvent);

  // call this if the daemon does not need to do anything at the startup
  virtual int InitProcess( std::vector<csm::daemon::CoreEvent *>& postEventList )
  {
    if (_EventRouting) return ( _EventRouting->InitProcess(postEventList) );
    else return 0;
  }

  struct itimerval* GetWindowInterval()
  {
    return &_WindowInterval;
  }

  sigset_t* GetSignalSet()
  {
    return &_SignalSet;
  }

  csm::daemon::RetryBackOff* GetRetryBackOff() { return &_IdleLoopRetry; }
  inline void IdleLoopReset()
  {
    _IdleLoopRetry.Reset();
  }
  inline bool IdleLoopAgainWait( const bool i_RunningJob )
  {
    // never attempt to sleep or wait for conditions while running a job
    // the jitter window management has to suspend the main loop
    if( i_RunningJob )
      return true;
    else
      return _IdleLoopRetry.AgainOrWait( i_RunningJob );
  }

};

class CoreMaster : public csm::daemon::CoreGeneric
{
public:
  CoreMaster();
  ~CoreMaster();
  
};

class CoreUtility: public csm::daemon::CoreGeneric
{
public:
  CoreUtility();
  ~CoreUtility();

};

class CoreAggregator: public csm::daemon::CoreGeneric
{
public:
  CoreAggregator();
  ~CoreAggregator();
  
};

class CoreAgent: public csm::daemon::CoreGeneric
{
  typedef csm::daemon::JitterWindow<std::chrono::microseconds, std::micro, std::chrono::high_resolution_clock> JitterWindowType;

  JitterWindowType *_JitterWindow;

public:
  CoreAgent();
  virtual ~CoreAgent();
  virtual int JitterWindow( const csm::daemon::JitterWindowAction i_Action = csm::daemon::WINDOW_NO_ADVANCE );
  
  virtual void manage_thread_RegisterThreadManager( csm::daemon::ThreadManager *i_ThreadMgr )
  { _threadMgr = i_ThreadMgr; }

  virtual csm::daemon::CoreEvent* Fetch( )
  {
    csm::daemon::CoreEvent *ev = _EventSources.Fetch( _ActiveWindow );
    if((_JitterWindow != nullptr) &&
        ( ev != nullptr ) &&
        ( ev->GetEventType() == csm::daemon::EVENT_TYPE_ENVIRONMENTAL ))
      _JitterWindow->ExtendWindow();
    return ev;
  }

};

}  // namespace daemon
} // namespace csm

#endif /* CSM_DAEMON_SRC_CSM_DAEMON_CORE_H_ */
