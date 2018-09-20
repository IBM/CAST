/*================================================================================

    csmd/src/daemon/include/csm_event_source.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_SOURCE_H_
#define CSM_DAEMON_SRC_CSM_EVENT_SOURCE_H_

#define NETWORK_SRC_ID     ( 123456 )
#define TIMER_SRC_ID       ( 123457 )
#define DB_SRC_ID          ( 123458 )
#define ENVIRONMENT_SRC_ID ( 123459 )
#define INTERVAL_SRC_ID    ( 123460 )

namespace csm {
namespace daemon {


class EventSource
{
  RetryBackOff *_RetryBackoff;
  uint64_t _Identifier;
  bool _OncePerWindow;

public:
  EventSource( RetryBackOff *aRetryBackOff,
               const uint64_t aIdentifier = 0,
               const bool aOncePerWindow = false )
  : _RetryBackoff( aRetryBackOff ),
    _Identifier( aIdentifier ),
    _OncePerWindow( aOncePerWindow )
  {}
  virtual ~EventSource() {}

  virtual csm::daemon::CoreEvent* GetEvent( const std::vector<uint64_t> &i_BucketList ) = 0;
  virtual bool QueueEvent( const csm::daemon::CoreEvent *i_Event ) = 0;
  inline bool OncePerWindow() const { return _OncePerWindow; }
  inline uint64_t GetIdentifier() const { return _Identifier; }
  inline bool WakeUpMainLoop() { _RetryBackoff->WakeUp(); return true; }
};

}  // namespace daemon
} // namespace csm


// include all implemented sources
#include "src/csm_event_sources/csm_source_db.h"
#include "src/csm_event_sources/csm_source_network.h"
#include "src/csm_event_sources/csm_source_timer.h"
//#include "src/csm_event_sources/csm_source_environmental.h"

#endif /* CSM_DAEMON_SRC_CSM_EVENT_SOURCE_H_ */
