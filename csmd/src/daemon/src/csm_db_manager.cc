/*================================================================================

    csmd/src/daemon/src/csm_db_manager.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifdef logprefix
#undef logprefix
#endif
#define logprefix "DBMGR"
#include "csm_pretty_log.h"

#include "csm_daemon_config.h"
#include "include/csm_db_manager.h"

enum DBManagerState {
  IDLE,
  REQUEST,
  EXECUTE,
  RESPONSE
};

void DBManagerMain( csm::daemon::EventManagerDB *aMgr )
{
  CSMLOG(csmd,debug) << "Starting Database manager threadID=" << boost::this_thread::get_id();
  CSMLOG(csmd,debug) << "Built for DB_SCHEMA version=" << std::string( DB_SCHEMA_VERSION );

  csm::daemon::RetryBackOff *idleRetry = aMgr->GetIdleRetry();

  DBManagerState MgrState = IDLE;

  csm::daemon::DBReqEvent *dbevent = aMgr->DequeueRequestEvent();
  csm::db::DBResult_sptr dbres = nullptr;
  csm::db::DBConnection_sptr dbConn = nullptr;
  csm::db::DBConnectionPool *dbConnPool = aMgr->GetDBConnectionPool();
  csm::db::DBConnection_sptr usrConn = nullptr;

  while( aMgr->GetThreadKeepRunning() )
  {
    switch( MgrState )
    {
      /* IDLE mode attempts to fetch an event from the dbrequest sink
       * or if nothing to do, then go idle-mode
       */
      case IDLE:
        dbevent = aMgr->DequeueRequestEvent();

        if (dbevent == nullptr )
        {
          if(dbConnPool->GetHeartbeatTimer() < std::chrono::system_clock::now())
          {
            // check db status and/or try to reconnect
            dbConnPool->Heartbeat();
          }
          idleRetry->AgainOrWait();
          continue;
        }
        else
        {
          // clean up before processing
          usrConn = nullptr;
          dbres = nullptr;
          dbConn = nullptr;

          MgrState = REQUEST;
        }

        break;

        /* REQUEST mode is entered when a new request is fetched from a queue
         * it parses the request and attempts to acquire a db connection
         * (either from the pool or from the user request)
         */
      case REQUEST:
      {
        CSMLOG( csmd, trace ) << "State REQUEST";
        if( dbevent == nullptr )
          throw csm::daemon::Exception( "DBMGR state machine error: REQUEST without event." );

        csm::db::DBReqContent dbcontent = dbevent->GetContent();

        usrConn = dbcontent.GetDBConnection();

        // handler does not try to use a dedicated db conn. Get one from the pool
        if (usrConn == nullptr)
          dbConn = dbConnPool->AcquireDBConnection();
        else
          dbConn = usrConn;

        if( dbConn != nullptr )
        {
          MgrState = EXECUTE;
        }
        else if(!dbConnPool->IsConnected())
        {
          // if no available connection because they're all down, respond w/ error
          dbres = dbConnPool->GetErrorResult();
          MgrState = RESPONSE;
        }
        else
        {
          CSMLOG( csmd , debug) << "No available DB Conn for Request: "
              << "ctx=" << dbevent->GetEventContext()->GetAuxiliaryId();
        }

        break;
      }

        /* CONNECTION mode is entered with an event and a connection to execute the SQL command
         * depending on the result, the next stage is RESPONSE or IDLE
         * if the connection was from the pool, or the pool is retrieving all locked connections, it is returned
         */
      case EXECUTE:
      {
        CSMLOG( csmd, trace ) << "DBMGR: State EXECUTE";
        if(( dbevent == nullptr ) || ( dbConn == nullptr ))
          throw csm::daemon::Exception( "DBMGR state machine error: CONNECTION without event." );

        CSMLOG(csmdb,debug) << "Executing DB Request: ctx=" << dbevent->GetEventContext()->GetAuxiliaryId() << " conn=" << dbConn->GetId();

        csm::db::DBReqContent dbcontent = dbevent->GetContent();

        if ( dbcontent.GetNumParams() == 0 )
          dbres = dbConn->ExecSql(dbcontent.GetSqlStmt().c_str());
        else
          dbres = dbConn->ExecParamSql(
              dbcontent.GetSqlStmt().c_str(),
              dbcontent.GetNumParams(),
              dbcontent.GetParamValues(),
              dbcontent.GetParamSizes(),
              dbcontent.GetParamFormats()
          );

        CSMLOG(csmdb,debug) << "Completed DB Request: ctx=" << dbevent->GetEventContext()->GetAuxiliaryId() << " conn=" << dbConn->GetId();

        if( dbres )
          MgrState = RESPONSE;
        else
        {
          CSMLOG( csmdb, warning ) << "DB Request failed. Restoring to queue and checking DB connection..."
              << " [ctx=" << dbevent->GetEventContext()->GetAuxiliaryId() << " conn=" << dbConn->GetId() << "]";

          // restore the request
          aMgr->RestoreRequestEvent(dbevent);
          // something is fishy here. check the db connection status
          MgrState = IDLE;
        }

        // if the db conn is acquired from the pool, need to release here.
        // handler will release the db conn if the db conn is a dedicated one.
        if( usrConn == nullptr || dbConnPool->DrainingConnections() )
        {
          if(dbConnPool->DrainingConnections())
          {
            LOG(csmd, debug) << "DBMGR: Draining locked connections";
            usrConn = nullptr;
          }
          dbConnPool->ReleaseDBConnection( dbConn );
        }

        // user-aquired connections are serialized
        // so we have to tell the event sink that this connection can be unblocked now
        try
        {
          aMgr->AckRequest( dbConn->GetId() );
        }
        catch (...)
        {
          CSMLOG( csmdb, error ) << "DBMgr failed to acknowledge request for connection " << dbConn->GetId();
        }

        break;
      }
        /* RESPONSE mode extracts the db response data and creates an event for the daemon main loop
         * then does clean up and return to IDLE mode
         */
      case RESPONSE:
      {
        CSMLOG( csmd, trace) << "DBMGR: State RESPONSE";
        // dbres shall not be nullptr here with a valid dbConn.
        if( dbres == nullptr )
          throw csm::daemon::Exception( "DBMGR state machine error: RESPONSE without event." );

        if(csm::db::DB_SUCCESS == dbres->GetResStatus())
        {
          dbConnPool->UpdateHeartbeatTimer();
        }

        csm::daemon::DBRespEvent *dbs = new csm::daemon::DBRespEvent( csm::db::DBRespContent( dbres, usrConn ),
                                                                      csm::daemon::EVENT_TYPE_DB_Response,
                                                                      dbevent->GetEventContext() );

        if( dbs == nullptr )
          throw csm::daemon::Exception( "DBMGR response event creation failed." );

        aMgr->QueueResponseEvent( dbs );

        // to-do: it's okay to delete the incoming db event now.
        delete dbevent;
        dbevent = nullptr;
        usrConn = nullptr;

        MgrState = IDLE;
        break;
      } // end if valid dbres

      default:
        throw csm::daemon::Exception( "DBMGR state machine error: UNRECOGNIZED STATE." );
        break;
    }

    idleRetry->Reset();
  } // while( aMgr->GetThreadKeepRunning() )
}


csm::daemon::EventManagerDB::~EventManagerDB()
{
  _KeepThreadRunning = false;

  if(( _DBConnectionPool != nullptr ) && ( _Thread != nullptr ))
  {
    CSMLOG( csmd, info ) << "Exiting. Terminating: " << _DBConnectionPool->GetNumConfigConnections() << " db threads";
    for( unsigned tid = 0; tid < _DBConnectionPool->GetNumConfigConnections(); ++tid )
    {
      _IdleRetry.JoinThread( _Thread[ tid ], tid );
      delete _Thread[ tid];
    }
    delete [] _Thread;
  }
  if( _DBConnectionPool != nullptr )
    delete _DBConnectionPool;

  LOG( csmd, debug ) << "Exiting... Worker-Threads ended.";
}

csm::daemon::EventManagerDB::EventManagerDB( csm::daemon::DBDefinitionInfo info,
                                             csm::daemon::ConnectionHandling *aConnMgr,
                                             csm::daemon::RetryBackOff *i_MainIdleLoopRetry )
:_DBConnectionPool(csm::db::DBConnectionPool::Init(info.first, info.second)),
 _ConnMgr(aConnMgr),
 _Thread(nullptr),
 _IdleRetry( "DBMgr", csm::daemon::RetryBackOff::SleepType::CONDITIONAL, csm::daemon::RetryBackOff::SleepType::CONDITIONAL, 1, 1000000, 1000 )
{
  _Source = new csm::daemon::EventSourceDB( i_MainIdleLoopRetry );
  _Sink = new csm::daemon::EventSinkDB( &_IdleRetry, _DBConnectionPool->GetNumConfigConnections() );
  _KeepThreadRunning = true;

  if(!_DBConnectionPool || _DBConnectionPool->GetNumOfDBConnections() <= 0)
  {
    CSMLOG(csmd,error) << "Fail to connect to DB";
  }

  if (_DBConnectionPool)
  {
    std::string dbVersion = _DBConnectionPool->GetDBSchemaVersion();
    std::string codeVersion = std::string( DB_SCHEMA_VERSION );

    // extract the major version number
    size_t dotpos_db = dbVersion.find('.');
    size_t dotpos_code = codeVersion.find('.');
    std::string dbMajorVersion = std::string( dbVersion, 0, dotpos_db );
    std::string codeMajorVersion = std::string( codeVersion, 0, dotpos_code );

    // explode if major version doesn't match
    if( dbMajorVersion != codeMajorVersion )
    {
      CSMLOG( csmd, critical ) << "DB schema major version mismatch, DB=" << dbMajorVersion << "; Build=" << codeMajorVersion;
      throw csm::daemon::Exception( "DB schema major version mismatch, DB=" + dbVersion + "; Build=" + codeVersion );
    }

    // still warn if version mismatches
    if( dbVersion != codeVersion )
    {
      CSMLOG(csmd, info) << "configured with " << _DBConnectionPool->GetNumConfigConnections() << " connections";
      CSMLOG(csmd, warning) << "DB schema version mismatch detected. Code built for version: " << codeVersion << "; DB has " << dbVersion << "!";
    }
    else
    {
      CSMLOG(csmd,info) << "configured with " << _DBConnectionPool->GetNumConfigConnections() << " connections; "
          << "built for DB_SCHEMA version=" << codeVersion;
    }


    _Thread = new boost::thread*[ _DBConnectionPool->GetNumConfigConnections() ];
    for( unsigned tid = 0; tid < _DBConnectionPool->GetNumConfigConnections(); ++tid )
    {
      _Thread[ tid ] = new boost::thread( DBManagerMain, this );
      CSMLOG( csmd, debug ) << "Creating DBThread: " << tid << " ptr=" << (void*)_Thread[ tid ];
    }
    csm::daemon::Configuration::Instance()->SetDBConnectionPool(_DBConnectionPool);
  }
}

void
csm::daemon::EventManagerDB::RegisterThreads( csm::daemon::ThreadPool *tp )
{
  for( unsigned tid = 0; tid < _DBConnectionPool->GetNumConfigConnections(); ++tid )
  {
    tp->MarkHandler( _Thread[ tid ]->get_id(), std::string("DBManager"+std::to_string(tid) ) );
    CSMLOG( csmd, debug ) << "Registering DBThread: " << tid << " ptr=" << (void*)_Thread[ tid ];
  }
}
