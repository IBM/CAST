/*================================================================================

    csmd/src/daemon/include/csm_event_type_definitions.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_TYPE_DEFINITIONS_H_
#define CSM_DAEMON_SRC_CSM_EVENT_TYPE_DEFINITIONS_H_

#include "include/csm_daemon_exception.h"

namespace csm {
namespace daemon {

enum EventType {
  EVENT_TYPE_UNKOWN = 0,
  
  EVENT_TYPE_INITIAL,
  EVENT_TYPE_TIMER,
  EVENT_TYPE_SYSTEM,
  EVENT_TYPE_DB_Request,
  EVENT_TYPE_DB_Response,
  EVENT_TYPE_NETWORK,
  EVENT_TYPE_ENVIRONMENTAL,
  EVENT_TYPE_BDS,
  EVENT_TYPE_SENSOR_Data,

  EVENT_TYPE_MAX
};

static inline const std::string
EventTypeToString( const enum EventType aType )
{
  switch( aType )
  {
    case EVENT_TYPE_UNKOWN: return "EVENT_TYPE_UNKOWN";
    case EVENT_TYPE_INITIAL: return "EVENT_TYPE_INITIAL";
    case EVENT_TYPE_TIMER: return "EVENT_TYPE_TIMER";
    case EVENT_TYPE_SYSTEM: return "EVENT_TYPE_SYSTEM";
    case EVENT_TYPE_DB_Request: return "EVENT_TYPE_DB_Request";
    case EVENT_TYPE_DB_Response: return "EVENT_TYPE_DB_Response";
    case EVENT_TYPE_NETWORK: return "EVENT_TYPE_NETWORK";
    case EVENT_TYPE_ENVIRONMENTAL: return "EVENT_TYPE_ENVIRONMENTAL";
    case EVENT_TYPE_BDS: return "EVENT_TYPE_BDS";
    case EVENT_TYPE_SENSOR_Data: return "charEVENT_TYPE_SENSOR_Data";

    case EVENT_TYPE_MAX:
    default:
      throw csm::daemon::Exception("Unrecognized event type.");
  }
}

}  // namespace daemon
} // namespace csm


#endif /* CSM_DAEMON_SRC_CSM_EVENT_TYPE_DEFINITIONS_H_ */
