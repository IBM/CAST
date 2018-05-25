/*================================================================================

    csmd/src/daemon/include/csm_event_sink.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_SINK_H_
#define CSM_DAEMON_SRC_CSM_EVENT_SINK_H_

#include "include/csm_core_event.h"

namespace csm {
namespace daemon {

class EventSink
{
public:
  EventSink() {}
  virtual ~EventSink() {}

  virtual int PostEvent( const csm::daemon::CoreEvent &aEvent ) = 0;
  virtual csm::daemon::CoreEvent* FetchEvent() = 0;
};

}  // namespace daemon
} // namespace csm

// include all implemented sinks
#include "src/csm_event_sinks/csm_sink_db.h"
#include "src/csm_event_sinks/csm_sink_network.h"
#include "src/csm_event_sinks/csm_sink_timer.h"
#include "src/csm_event_sinks/csm_sink_system.h"
#include "src/csm_event_sinks/csm_sink_bds.h"

#endif /* CSM_DAEMON_SRC_CSM_EVENT_SINK_H_ */
