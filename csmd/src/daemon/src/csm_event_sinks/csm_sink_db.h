/*================================================================================

    csmd/src/daemon/src/csm_event_sinks/csm_sink_db.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_DB_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_DB_H_

#include <mutex>
#include <deque>

#include "include/csm_db_event_content.h"
#include "include/csm_event_sink.h"
#include "include/csm_retry_backoff.h"

namespace csm {
namespace daemon {

typedef CoreEventBase<csm::db::DBReqContent> DBReqEvent;
typedef std::deque<const DBReqEvent*> DBReqEventQueue;

/*
 * How is head of line blocking possible:
 *   Assume 10 connections, 10 db threads, 3 connections acquired by handlers
 *   now, 10 requests without dedicated connections arrive in queue
 *   plus 3 requests with dedicated connections
 *   The first 10 requests will start to occupy the 10 threads
 *   only 7 can acquire a connection (the other 3 connections are owned by handlers)
 *   now the other 3 threads need to wait for the connections
 *   while there are 3 requests waiting with an idle connection
 *   -> Head of line blocking...
 */

class EventSinkDB : public csm::daemon::EventSink
{
  csm::daemon::RetryBackOff *_ManagerWakeup;  ///< to wake up the db managers as requests arrive
  DBReqEventQueue *_Request;                  ///< list of event queues for each db connection
  bool *_QueueBusy;                           ///< to serialize requests of dedicated queues
  int _NumQueues;                             ///< number of request queues (connections + 1)
  int _DefaultQueueIndex;                     ///< queue number for requests without reserved connection
  int _ActiveQueue;                           ///< currently active queue for event scheduling
  std::mutex _RequestLock;                    ///< for concurrent access to queues
  std::atomic_int _RequestCount;              ///< total pending request count

public:
  EventSinkDB( csm::daemon::RetryBackOff *i_MgrWakeup,
               const int i_NumQueues = 1 )
  : _ManagerWakeup( i_MgrWakeup ),
    _NumQueues( i_NumQueues + 1 ),
    _DefaultQueueIndex( i_NumQueues ),
    _ActiveQueue( 0 )
  {
    _Request = new DBReqEventQueue[ _NumQueues ];
    _QueueBusy = new bool[ _NumQueues ];
    memset( _QueueBusy, 0, _NumQueues * sizeof( bool ) );
    _RequestCount = 0;
  }
  virtual ~EventSinkDB()
  {
    for( int i = 0; i < _NumQueues; ++i )
      _Request[ i ].clear();

    delete [] _Request;
    delete [] _QueueBusy;
  }
  virtual int PostEvent( const csm::daemon::CoreEvent &aEvent )
  {
    // todo: cast event to DBRequest
    // check for requested connection and select the corresponding queue for the event
    // need an assignment between queue and db connection to prevent head-of-line blocking
    if(( aEvent.GetEventType() != csm::daemon::EVENT_TYPE_DB_Request ) ||
        ( ! aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::db::DBReqContent> ) )))
      throw csm::daemon::Exception("EventSinkDB: Wrong event type posted to DB Request Sink.");

    const csm::daemon::DBReqEvent* dbEvent = dynamic_cast<const csm::daemon::DBReqEvent*>( &aEvent );
    if( dbEvent == nullptr )
      throw csm::daemon::Exception("EventSinkDB: Nullptr event detected.");
    csm::db::DBReqContent dbcontent = dbEvent->GetContent();
    csm::db::DBConnection_sptr conn = dbcontent.GetDBConnection();

    // requests without connection get their own queue
    unsigned connectionID = _DefaultQueueIndex;
    if( conn != nullptr )
      connectionID = conn->GetId();

    _RequestLock.lock();
    _Request[ connectionID ].push_back( dynamic_cast<const csm::daemon::DBReqEvent*>( &aEvent ));
    LOG( csmd, trace ) << "DBSink: push new request to connection: " << connectionID;
    ++_RequestCount;
    _RequestLock.unlock();

    _ManagerWakeup->WakeUp();
    return 0;
  }
  virtual csm::daemon::CoreEvent* FetchEvent()
  {
    csm::daemon::DBReqEvent *dbe = nullptr;
    int startQueue = _ActiveQueue;

    _RequestLock.lock();
    do
    {
      // only fetch from queues that are non-empty and don't have requests in progress
      if(( ! _Request[ _ActiveQueue ].empty() ) && ( _QueueBusy[ _ActiveQueue ] == false ))
      {
        dbe = const_cast<csm::daemon::DBReqEvent*>( _Request[ _ActiveQueue ].front() );
        _Request[ _ActiveQueue ].pop_front();
        --_RequestCount;
        if( _ActiveQueue != _DefaultQueueIndex )
          _QueueBusy[ _ActiveQueue ] = true;
        LOG( csmd, trace) << "DBMGR Sink queue: " << _ActiveQueue;
        break;
      }
      _ActiveQueue = ( _ActiveQueue + 1 ) % ( _NumQueues );
    } while ( _ActiveQueue != startQueue );
    _RequestLock.unlock();
    return dbe;
  }

  // special for db event sink: return event to front of queue
  // in case db connection was lost and we need to retry later
  inline void RestoreRequestEvent(csm::daemon::DBReqEvent *dbe)
  {
    csm::db::DBConnection_sptr conn = dbe->GetContent().GetDBConnection();

    // requests without connection get their own queue
    unsigned connectionID = _DefaultQueueIndex;
    if( conn != nullptr )
      connectionID = conn->GetId();

    _RequestLock.lock();
    _Request[ connectionID ].push_front( dbe );
    _QueueBusy[ connectionID ] = false;
    ++_RequestCount;
    _RequestLock.unlock();
  }

  // signal the completion of an event - required for serialization in dedicated queues
  inline void AckEvent( const int i_ConnectionID )
  {
    if( i_ConnectionID >= _NumQueues ) // >= because count starts at 0
      throw csm::daemon::Exception( "DB Request Sink, Queue Index out of range." );
    _QueueBusy[ i_ConnectionID ] = false;
  }
};

} // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_SRC_CSM_EVENT_SINKS_CSM_SINK_DB_H_ */
