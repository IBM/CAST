/*================================================================================

    csmd/src/daemon/include/csm_event_sink_set.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_event_sinks.h
 *
 ******************************************/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_SINK_SET_H_
#define CSM_DAEMON_SRC_CSM_EVENT_SINK_SET_H_

// implements a set of event sources with priorities
// uses polymorphic event sinks to enable posting to various types of sinks

#include <map>
#include <deque>
#include <mutex>
#include <string>
#include <inttypes.h>

#include "include/csm_core_event.h"
#include "include/csm_event_type_definitions.h"
#include "include/csm_daemon_exception.h"
#include "include/csm_event_sink.h"
#include "include/csm_daemon.h"

namespace csm {
namespace daemon {

typedef std::map< csm::daemon::EventType, csm::daemon::EventSink* > SinkMap;

typedef std::exception EventSinkException;


class EventSinkSet
{
  // \todo: one set per priority
  SinkMap _Sinks;
  SinkMap::iterator _CurrentSink;
  std::mutex _Lock;

public:
  EventSinkSet();
  virtual ~EventSinkSet();

  // selects sink based on event type
  run_mode_reason_t PostEvent( const csm::daemon::CoreEvent &aEvent );

  // adds a new event sink
  int Add( const EventType aType,
           const csm::daemon::EventSink *aSink );

  // removes an event sink
  int Remove( const EventType aType );

  EventSink* operator[]( const CoreEvent& aEvent );

};

}  // namespace daemon
} // namespace csm

#endif /* CSM_DAEMON_SRC_CSM_EVENT_SINK_SET_H_ */
