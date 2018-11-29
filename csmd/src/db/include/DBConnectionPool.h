/*================================================================================

    csmd/src/db/include/DBConnectionPool.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef _CSM_DB_SRC_DBCONNECTIONPOOL_H
#define _CSM_DB_SRC_DBCONNECTIONPOOL_H

#include <mutex>
#include <memory>
#include <string.h>
#include <deque>
#include <unordered_set>
#include <atomic>
#include <condition_variable>

#include "logging.h"
#include "DBConnection.h"

#define DEFAULT_CONNECTION_POOL_SIZE (1)
#define DB_HEARTBEAT 15
#define WAIT_INTERVAL 2

namespace csm {
namespace db {

struct DBConnInfo
{
  std::string server;
  std::string user;
  std::string passwd;
  std::string dbName;

  DBConnInfo(std::string aServer, std::string aUser, std::string aPasswd, std::string aDBName)
  {
    server = aServer;
    user = aUser;
    passwd = aPasswd;
    dbName = aDBName;
  };

  DBConnInfo() {}

};
  
class DBConnectionPool
{
public:
  
  static DBConnectionPool* Init(unsigned aNumConn, DBConnInfo &aInfo)
  {
    if (_instance == nullptr) _instance = new DBConnectionPool(aNumConn, aInfo);
    return _instance;
  }

  
  DBConnection_sptr AcquireDBConnection()
  {
    DBConnection_sptr ret = nullptr;
    
    _PoolLock.lock();
    if (!_FreeConnPool.empty() && !DrainingConnections())
    {
      ret = _FreeConnPool.front();
      _FreeConnPool.pop_front();
      _LockedConnPool.insert(ret);
    }

    _PoolLock.unlock();
    
    if (ret)
    {
      if( !ret->IsConnectionSuccess() )
      {
        LOG(csmdb, error) << "DBConnectionPool::AcquireDBConnection(): CONNECTION DOWN!";
        ReleaseDBConnection( ret );
        return nullptr;
      }

      LOG(csmd, debug) << "Use the " << ret->GetId() << "th connection in the pool...";
      if ( !ret->IsIdleConnection() )
      {
        LOG(csmdb, error) << "DBConnectionPool::AcquireDBConnection(): Acquire a non idle connection. Try to reset!";
        ret->ResetConnection();
      }
    }
    return ret;
  }
  
  void ReleaseDBConnection(DBConnection_sptr aDBConn)
  {
    if (aDBConn == nullptr) return;
    
    _PoolLock.lock();
    
    if (_LockedConnPool.erase(aDBConn) == 1)
    {
      // successfully remove it from _LockedConnPool. Add back to _FreeConnPool.
      // need to check the transaction status of the conn before return to the pool. reset if necessary
      aDBConn->ResetConnection();
      _FreeConnPool.push_back(aDBConn);
    }
    else
    {
      LOG(csmdb, error) << "Cannot find the connection in _LockedConnPool!";
    }

    _PoolLock.unlock();
    if(DrainingConnections() && _LockedConnPool.empty())
    {
      DrainedPoolCondition.notify_all();
    }
  }
  
  virtual ~DBConnectionPool()
  {
#if 0 // for debugging
    LOG(csmdb, info) << "In ~DBConnectionPool: FreeDBConn:";
    
    while ( !_FreeConnPool.empty() )
    {
      DBConnection_sptr top = _FreeConnPool.top();
      _FreeConnPool.pop();
      LOG(csmdb, info) << "Free: " << top->GetId();
    }
    
    LOG(csmdb, info) << "In ~DBConnectionPool: LockedDBConn:";
    for ( const DBConnection_sptr& x: _LockedConnPool )
    {
      LOG(csmdb, info) << "Locked: " << x->GetId();
    }
#endif
  }
  
  unsigned GetNumOfActiveDBConnections() const
  {
    int activeDBConn = 0;
    for (unsigned i=0; i<_ConnPool.size(); i++)
      if ( _ConnPool[i]->IsConnectionSuccess() ) activeDBConn++;
      
    return activeDBConn;
  }
  
  inline unsigned GetNumOfFreeDBConnections() const { return _FreeConnPool.size(); }
  inline unsigned GetNumOfLockedDBConnections() const { return _LockedConnPool.size(); }
  inline unsigned GetNumOfDBConnections() const { return _ConnPool.size(); }
  inline unsigned GetNumConfigConnections() const { return _ConfigNumConns; }
  inline std::string GetDBSchemaVersion() const { return _db_schema_version; }

  int Connect()
  {
    if (_ConnPool.size() > 0)
    {
      LOG(csmd, debug) << "DBConnectionPool: Previous DB connections already created. Will not try to create again!";
      return _ConnPool.size();
    };
    
    // first, create the db connection pool
    unsigned NumConns = 0;
    for (unsigned i=0; i<_ConfigNumConns; i++)
    {
      PGconn *pgconn = PQ_CreateConn(_DBInfo.server.c_str(), _DBInfo.user.c_str(), _DBInfo.passwd.c_str(), _DBInfo.dbName.c_str());
      if (pgconn && PQ_CONN_SUCCESS(pgconn))
      {
        DBConnection_sptr dbconn = std::make_shared<DBConnection>(pgconn, NumConns++);
        _FreeConnPool.push_front(dbconn);
        
        _ConnPool.push_back(dbconn);
      }
      else
      {
        if (pgconn)
        {
          LOG(csmdb, error) << "Fail to create a connection in the pool. " << PQ_CONN_ERROR_MSG(pgconn);

          // PQmakeEmptyPGresult works as expected here
          if(!_ErrResult) _ErrResult = std::make_shared<DBResult>(PQmakeEmptyPGresult(pgconn, PGRES_FATAL_ERROR));
          PQ_DeleteConn( pgconn );
          pgconn = nullptr;
        }
        else
        {
          LOG(csmdb, error) << "PGconn is null!";
        }
        break;
      }
    }
    
    // if success, use one of the db connection to query the db schema version and record it
    if (_ConnPool.size() > 0)
    {
      _ErrResult = nullptr;
      DBConnection_sptr dbconn = _ConnPool[0];
      DBResult_sptr dbres = dbconn->ExecSql("select version from csm_db_schema_version");
      if (dbres->GetResStatus() ==  DB_SUCCESS && dbres->GetNumOfTuples() > 0)
      {
        char *tuple = dbres->GetValue(0, 0);
        if (strlen(tuple) > 0) _db_schema_version = std::string(tuple);
      }
      //LOG(csmd, debug) << "DB ERROR: " << dbres->GetErrMsg();
      LOG(csmd, debug) << "DBConnectionPoool: _db_schema_version = " << _db_schema_version << "!";
      
    }
    
    return _ConnPool.size();
  }

  bool IsConnected() const
  {
    return (_FreeConnPool.size() + _LockedConnPool.size()) > 0;
  }

  std::chrono::time_point<std::chrono::steady_clock> GetHeartbeatTimer() const
  {
    return _HeartbeatTimer;
  }

  void Heartbeat()
  {
    if(!_TimerLock.try_lock())
    {
      return;
    }

    if(_ConnPool.empty())
    {
      Connect();
      _HeartbeatTimer += _HeartbeatInterval;
      _TimerLock.unlock();
      return;
    }

    // test a working connection
    // bad connections may not report that until the third step
    static auto _ping = [](DBConnection_sptr conn)
                          {
                            DBResult_sptr result;
                            return conn->IsConnectionSuccess() &&
                            (result = conn->ExecSql("")) &&
                            result->GetResStatusCode() == PGRES_EMPTY_QUERY;
                          };

    // test if a bad connection is working again
    // it shouldn't be necessary to send an empty query
    static auto _reconnect = [](DBConnection_sptr conn)
                              {
                                conn->ResetConnection();
                                return conn->IsConnectionSuccess();
                              };

    DBConnection_sptr heartbeat = AcquireDBConnection(),
                      retry = _DisconnectedPool.empty() ? nullptr : _DisconnectedPool.front();

    // don't update the timer if no free connections are available and no bad connections exist
    if(heartbeat || retry)
    {
      _HeartbeatTimer = std::chrono::steady_clock::now() + _HeartbeatInterval;
    }
    else
    {
      LOG(csmd,debug) << "DBConnectionPool::Heartbeat(): no connections available";
      _HeartbeatTimer = std::chrono::steady_clock::now() + _HeartbeatWait;
    }

    bool heartbeat_failed = false;
    if(heartbeat)
    {
      DBResult_sptr res;
      res = heartbeat->ExecSql("");
      ReleaseDBConnection(heartbeat); // remove from lockedpool before we try to check all free conns
      if(res && res->GetResStatusCode() == PGRES_EMPTY_QUERY)
      {
        LOG(csmd,debug) << "Received DB heartbeat";
      }
      else
      {
        heartbeat_failed = true; // don't try to reconnect
        if(res)
        {
          LOG(csmd,error) << "DB heartbeat failed: " << res->GetErrMsg();
          _ErrResult = res;
        }
        else
        {
          LOG(csmd,error) << "DB heartbeat failed: No Database Connection in Local Daemon";
        }

        if(!_LockedConnPool.empty()) // wait for any connections held by another thread
        {
          draining = true;
          LOG(csmd, info) << "Waiting on " << _LockedConnPool.size() << " locked connections...";

          std::mutex temp;
          std::unique_lock<std::mutex> templock(temp);
          DrainedPoolCondition.wait_for(templock, std::chrono::seconds(DB_HEARTBEAT/2));

          if(!_LockedConnPool.empty())
          {
            LOG(csmd, error) << "DBConnectionPool::Heartbeat(): "
              <<_LockedConnPool.size() << " locked connections could not be acquired";
          }
          draining = false;
        }
      }
    }
    else if(!_FreeConnPool.empty()) // free pool has bad conns
    {
      LOG(csmd, warning) << "Could not acquire connection for heartbeat, but free pool is nonempty";
      heartbeat_failed = true; // test free pool instead of reconnecting
    }
    if(heartbeat_failed)
    {
      CheckConnections(_FreeConnPool, _ping);
    }
    else if(!_DisconnectedPool.empty()) // heartbeat passed or there were no good connections
    {
      LOG(csmd,debug) << "DBConnectionPool::Heartbeat(): Attempting to reconnect...";
      // try one connection, if it succeeds try them all
      if(_reconnect(retry))
      {
        CheckConnections(_DisconnectedPool, _reconnect);
      }
      LOG(csmd,debug) << "Connections available: " << _FreeConnPool.size();
    }
    _TimerLock.unlock();
  }

  // postpone heartbeat if some other db request succeeds
  void UpdateHeartbeatTimer()
  {
    if(!_TimerLock.try_lock())
    {
      return;
    }
    if(_DisconnectedPool.empty())
    {
      _HeartbeatTimer = std::chrono::steady_clock::now() + _HeartbeatInterval;
    }
    _TimerLock.unlock();
  }

  DBResult_sptr GetErrorResult() const
  {
    return _ErrResult;
  }

  bool DrainingConnections() const
  {
    return draining;
  }

private:
  DBConnectionPool(unsigned aNumConn, DBConnInfo &aInfo)
  : _ConfigNumConns(aNumConn), _DBInfo(aInfo),
    _HeartbeatInterval(std::chrono::seconds(DB_HEARTBEAT)),
    _HeartbeatWait(std::chrono::seconds(WAIT_INTERVAL)),
    draining(false)
  {
    Connect();
    _HeartbeatTimer = std::chrono::steady_clock::now() + _HeartbeatInterval;
  }

  static DBConnectionPool *_instance;

  std::unordered_set<DBConnection_sptr> _LockedConnPool;
  std::deque<DBConnection_sptr> _FreeConnPool;
  std::deque<DBConnection_sptr> _DisconnectedPool;
  // _ConnPool = _LockedConnPool + _FreeConnPool. Will remain the same after constructor.
  std::vector<DBConnection_sptr> _ConnPool;
  
  std::mutex _PoolLock;
  std::mutex _TimerLock;
  
  unsigned _ConfigNumConns;
  DBConnInfo _DBInfo;
  
  std::string _db_schema_version;

  std::chrono::seconds _HeartbeatInterval;
  std::chrono::seconds _HeartbeatWait;
  std::chrono::time_point<std::chrono::steady_clock> _HeartbeatTimer;

  std::condition_variable DrainedPoolCondition;
  std::atomic<bool> draining;

  // save the error result when the heartbeat fails so dbmgr can respond while disconnected
  DBResult_sptr _ErrResult = nullptr;

  template <class F>
  void CheckConnections(std::deque<DBConnection_sptr>& connections, F TestConnection)
  {
    _PoolLock.lock();
    DBConnection_sptr conn;
    DBResult_sptr res;
    unsigned s = connections.size();
    for(unsigned i = 0; i < s; ++i)
    {
      conn = connections.front();
      connections.pop_front();
      if(TestConnection(conn))
      {
        _FreeConnPool.push_back(conn);
      }
      else
      {
        _DisconnectedPool.push_back(conn);
      }
    }
    if(_DisconnectedPool.empty())
    {
      _ErrResult = nullptr;
    }
    _PoolLock.unlock();
  }
};

} // end namespace db
} // end namespace csm

#endif
