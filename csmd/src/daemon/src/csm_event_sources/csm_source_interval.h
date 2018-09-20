/*================================================================================

    csmd/src/daemon/src/csm_event_sources/csm_source_interval.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_INTERVAL_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_INTERVAL_H_


#include "include/csm_event_source.h"
#include "include/csm_timer_event.h"

/*
 * This will be used to indicate an interval triggered timer event
 */
#define EVENT_CTX_INTERVAL_MAGIC (0x13734A77ull)

namespace csm {
namespace daemon {

class EventSourceInterval: public csm::daemon::EventSource
{
public:
  EventSourceInterval( RetryBackOff *aRetryBackoff )
  : csm::daemon::EventSource( aRetryBackoff, INTERVAL_SRC_ID, true )
  {}

  virtual ~EventSourceInterval() {}

  virtual csm::daemon::CoreEvent* GetEvent( const std::vector<uint64_t> &i_BucketList )
  {
    TimerContent content( EVENT_CTX_INTERVAL_MAGIC, 0 );
    return new TimerEvent( content, EVENT_TYPE_TIMER, nullptr );
  }

  // no-op for this type of source
  virtual bool QueueEvent( const csm::daemon::CoreEvent *i_Event )
  {
    WakeUpMainLoop();
    return true;
  }

private:
};

}  // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_INTERVAL_H_ */
