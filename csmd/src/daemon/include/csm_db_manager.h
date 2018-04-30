/*================================================================================

    csmd/src/daemon/include/csm_db_manager.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_DB_MANAGER_H_
#define CSM_DAEMON_SRC_CSM_DB_MANAGER_H_

#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include <boost/thread.hpp>

#include "csmd/src/db/include/DBConnectionPool.h"
#include "csmd/src/db/include/csm_db_event_content.h"

#include "include/csm_retry_backoff.h"
#include "include/connection_handling.h"
#include "include/csm_event_manager.h"
#include "src/csm_event_sources/csm_source_db.h"

#include "include/thread_pool.h"

namespace csm {
namespace daemon {

class ConnectionManager;

class EventManagerDB : public csm::daemon::EventManager {

public:
  EventManagerDB( std::pair<unsigned, csm::db::DBConnInfo> info,
                  csm::daemon::ConnectionHandling *aConnMgr,
                  csm::daemon::RetryBackOff *i_MainIdleLoopRetry );
  virtual ~EventManagerDB();
  
  inline bool GetThreadKeepRunning() const { return _KeepThreadRunning; }

  inline csm::daemon::DBReqEvent * DequeueRequestEvent( )
  {
    return dynamic_cast<csm::daemon::DBReqEvent*>( _Sink->FetchEvent() );
  }
  inline void AckRequest( const int i_ConnectionID )
  {
    dynamic_cast<csm::daemon::EventSinkDB*>(_Sink)->AckEvent( i_ConnectionID );
  }
  
  // restore the event to the front queue
  inline void RestoreRequestEvent(csm::daemon::DBReqEvent *dbe)
  {
    dynamic_cast<csm::daemon::EventSinkDB*>( _Sink )->RestoreRequestEvent( dbe );
  }
  
  inline csm::db::DBConnectionPool* GetDBConnectionPool() { return _DBConnectionPool; }
  inline csm::daemon::ConnectionHandling *GetConnectionManager() { return _ConnMgr; }
  
  inline const boost::thread* GetThread() { return _Thread[0]; }
  inline csm::daemon::RetryBackOff* GetIdleRetry() { return &_IdleRetry; }

  void RegisterThreads( csm::daemon::ThreadPool *tp );

  inline bool QueueResponseEvent( csm::daemon::DBRespEvent *i_Response )
  {
    return _Source->QueueEvent( i_Response );
  }

private:
  csm::daemon::DBRespEvent* CreateErrorEvent( csm::db::DBRespContent &aData,
                                          const std::string &aMsg,
                                          int aRC )
  {
    LOG(csmd,info) << "Creating DB error event: " << aMsg;
    return new csm::daemon::DBRespEvent( aData, csm::daemon::EVENT_TYPE_DB_Response );
  }
  
private:
  csm::db::DBConnectionPool *_DBConnectionPool;
  csm::daemon::ConnectionHandling *_ConnMgr;

  volatile std::atomic<bool> _KeepThreadRunning;
  boost::thread ** _Thread;
  RetryBackOff _IdleRetry;
};

} // namespace daemon
} // namespace csm
#endif
