/*================================================================================

    csmd/src/daemon/src/csm_daemon_core_agent.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_daemon_core_agent.cc
 *
 ******************************************/

#include <logging.h>

#include "include/csm_daemon_core.h"

namespace csm {
namespace daemon {

// dummy routine for now
// later should check if the time limit for the active jitter window is exceeded
bool TimeIsUp( int aActiveJitterWindow )
{
  return (( random() % 2 ) != 0 );
}

CoreAgent::CoreAgent()
: CoreGeneric()
{
  _JitterWindow = new JitterWindowType(
      std::chrono::duration<long, std::micro>( _Config->GetTimerInterval() ),
      std::chrono::duration<long, std::micro>( _Config->GetWindowDuration() ),
      "µs",
      _Config->GetWindowExtensionFactor() );

  _EventRouting = new csm::daemon::EventRoutingAgent();
  
  // call after initializing EvnetRouting
  //InitInfrastructure();
  _envSource = new csm::daemon::EventSourceEnvironmental( GetRetryBackOff() );
  AddEventSource(_envSource, EVENT_TYPE_ENVIRONMENTAL);

}

CoreAgent::~CoreAgent()
{
  // Agent daemon destruction
  delete _JitterWindow;
}

int CoreAgent::JitterWindow( const csm::daemon::JitterWindowAction i_Action )
{
  switch( i_Action )
  {
    case WINDOW_ADVANCE:
      // advance means, we're opening a new window, so we can unlock the daemon
      CoreGeneric::JitterWindow( i_Action );
      _IdleLoopRetry.WakeUp();

      _JitterWindow->Open();
      LOG(csmd, trace) << "SCHED: New WindowIdx=" << _ActiveWindow;
      break;
    case WINDOW_NO_ADVANCE:
    {
      // no-advance means, we're in an open window and need to check whether we have to close it
      bool closed = _JitterWindow->CheckClose( _ActiveWindow );
      if( closed )
      {
        //Do the callback with (disable = true)
        if (_threadMgr) _threadMgr->StopThreads();
        LOG(csmd, trace) << "JITTER: Closing Jitter Window" << _ActiveWindow;

        // block the process until we receive wakeup condition
        _IdleLoopRetry.AgainOrWait( true );
        LOG(csmd, trace) << "JITTER: woke up from suspend() with JitterWindow.Closed=" << _JitterWindow->IsClosed();

        //Do the callback with (disable = false)
        if (_threadMgr) _threadMgr->StartThreads();
        LOG(csmd, trace) << "JITTER: Opened Jitter Window" << _ActiveWindow;
      }
      break;
    }
    case WINDOW_RESET:
      _JitterWindow->Reset();
      break;
    default:
      throw csm::daemon::Exception("Unrecognized JitterWindowAction in CoreAgent::JitterWindow.");
  }
  return _ActiveWindow;
}

#if 0 // obsolete
int CoreAgent::Process( const csm::daemon::CoreEvent &aEvent )
{
  int rc = 0;

  // Check the jitter window and don't return control to master loop until we hit the next window
  if( TimeIsUp( _ActiveJitterWindow ) )
  {
    _ActiveJitterWindow = ( _ActiveJitterWindow + 1 ) % _JitterWindowMax;
    // go to sleep until the next window
  }

  // todo: feed events into actual state machine
  if (_EventRouting == nullptr) {
    LOG(csmd, error) << "CoreAgent::Process(): Fail to access _EventRouting";
    return 0;
  }
  
  csm::daemon::EventProcessor *EP = _EventRouting->GetEventProcessor(aEvent);
  //csm::daemon::EventProcessor *EP = mEventProcessors[ aEvent.GetEventType() ];

  LOG(csmd,info) << "Event: " << aEvent.GetEventType();
  LOG(csmd,info) << "Procr: " << (void*)EP;

  // whatever the master has to do with the event
  if( ! EP )
    throw new csm::daemon::EventProcessorException();

  EP->Process( aEvent );

  // whatever the agent has to do...
  return rc;
}
#endif

}  // namespace daemon
} // namespace csm
