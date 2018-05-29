/*================================================================================

    csmd/src/daemon/src/csm_event_sinks/csm_sink_system.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_SYSTEM_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_SYSTEM_H_

#include <queue>
#include <mutex>

#include "include/csm_system_event.h"
#include "include/csm_event_sink.h"
#include "include/connection_handling.h"

#include "logging.h"

namespace csm {
namespace daemon {

class EventSinkSystem: public csm::daemon::EventSink
{

public:
  EventSinkSystem( csm::daemon::ConnectionHandling *aConnHdlg )
  : _ConnHandling( aConnHdlg )
  { }
  
  virtual ~EventSinkSystem()
  { }
  
  virtual int PostEvent( const csm::daemon::CoreEvent &aEvent )
  {
    //LOG(csmd, info) << "EventSinkTimer: PostEvent...";
    const SystemEvent *event = dynamic_cast<const SystemEvent *>( &aEvent );
    if( event == nullptr )
      return 0;

    csm::daemon::SystemContent sysdata = event->GetContent();

    LOG(csmd, trace ) << "EventSinkSystem: Processing event " << sysdata.GetSignalType();
    
    switch( sysdata.GetSignalType() )
    {
      case csm::daemon::SystemContent::RESET_AGG:
      {
        csm::daemon::ConnectionHandling_compute *comp_ch = dynamic_cast<csm::daemon::ConnectionHandling_compute*>( _ConnHandling );
        if( comp_ch != nullptr )
          comp_ch->ResetPrimary();
        break;
      }
      default:
        break;
    }

    return 0;
  }

  virtual csm::daemon::CoreEvent* FetchEvent()
  {
    return nullptr;
  }

private:
  csm::daemon::ConnectionHandling *_ConnHandling;
};

}  // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_SYSTEM_H_ */
