/*================================================================================

    csmd/src/daemon/src/csm_event_sources/csm_source_db.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_DB_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_DB_H_

#include <deque>

#include "include/csm_core_event.h"
#include "include/csm_db_event_content.h"

#include "include/csm_db_exception.h"
#include "include/csm_event_source.h"
#include "include/csm_retry_backoff.h"

namespace csm {
namespace daemon {

typedef CoreEventBase<csm::db::DBRespContent> DBRespEvent;
typedef std::deque<const DBRespEvent*> DBRespEventQueue;

class EventSourceDB: public csm::daemon::EventSource
{
  std::mutex _ResponseLock;
  DBRespEventQueue _Response;

public:
  EventSourceDB( RetryBackOff *aRetryBackoff )
  : csm::daemon::EventSource( aRetryBackoff, DB_SRC_ID, false )
  {  }
  virtual ~EventSourceDB() {}

  virtual csm::daemon::CoreEvent* GetEvent( const std::vector<uint64_t> &i_BucketList )
  {
    csm::daemon::DBRespEvent *dbe = NULL;
    try {
      _ResponseLock.lock();
      if( !_Response.empty() )
      {
        dbe = const_cast<DBRespEvent*>( _Response.front() );
        _Response.pop_front();
      }
      _ResponseLock.unlock();
    }
    catch ( csm::db::Exception &e ) {
      LOG(csmd,info) << "GetEvent Error: " << e.what();
      if( dbe )
      {
        delete dbe;
        dbe = nullptr;
      }
    }

    return dbe;
  }

  virtual bool QueueEvent( const csm::daemon::CoreEvent *i_Event )
  {
    _ResponseLock.lock();
    _Response.push_back( dynamic_cast<const csm::daemon::DBRespEvent*>(i_Event) );
    _ResponseLock.unlock();
    return WakeUpMainLoop();
  }
};




}  // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_DB_H_ */
