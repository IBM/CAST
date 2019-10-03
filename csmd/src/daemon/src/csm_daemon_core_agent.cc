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

}  // namespace daemon
} // namespace csm
