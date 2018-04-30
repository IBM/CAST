/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_daemon_clock.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_daemon_clock.h"
#include "logging.h"

void CSM_DAEMON_CLOCK::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  if ( aEvent.GetEventType() == csm::daemon::EVENT_TYPE_INITIAL )
  {
    if (_active_timer_count > 0)
    {
      LOG(csmd, info) << "In CSM_DAEMON_CLOCK:: Already have one active timer!";
      return;
    }
    else _active_timer_count++;
  }
  LOG(csmd, trace) << "In CSM_DAEMON_CLOCK: active_timer_count = " << _active_timer_count << "...";
  
  // create the next timer event
  csm::daemon::TimerEvent *event = CreateTimerEvent(_timer, (void *) this);
  
  // update the global state
  csm::daemon::DaemonState *daemonState = GetDaemonState();
  if (daemonState)
  {
    // update the timestamp in the daemon state
    daemonState->UpdateTimestamp(event->GetContent().GetStartTime());
    
    // update the load in the daemon state and the internal _prev_event_count
    daemonState->UpdateEventLoadAndCount(_prev_event_count, _timer_inverse);

  }
  
  // now ready to push the event out
  postEventList.push_back( event );  
}
