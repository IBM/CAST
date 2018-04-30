/*================================================================================

    csmd/src/db/include/DBConnection.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef _CSM_DB_SRC_DBCONNECTION_H
#define _CSM_DB_SRC_DBCONNECTION_H
#define STRING_SQL 0

#include <libpq-fe.h>
#include <memory>
#include <string.h>

#include "logging.h"
#include "PostgreSql.h"
#include "DBResult.h"

namespace csm {
namespace db {

enum DBCONN_STATE {
  DBCONN_FREE = 1,
  DBCONN_LOCK,
};

/**
 \brief Connection Object to the database
 */
class DBConnection {

public:
  DBConnection(PGconn *aConn, unsigned aId)
  : _pg_conn(nullptr),
    _id(aId)
  {
    _pg_conn = aConn;
    _state = DBCONN_FREE;
  }
  
  inline void SetState(DBCONN_STATE aState) { _state = aState; }
  inline DBCONN_STATE GetState() { return _state; }
  
  inline unsigned GetId() { return _id;}
  
    /** @brief Executes a parameterized sql query and generates the shared pointer.
     *
     * @param[in] command The paramerterized query.
     * @param[in] paramCount The number of parameters handled by the query.
     * @param[in] paramValues An array of parameters.
     * const csm::db::DBReqContent &reqContent
     */
    inline csm::db::DBResult_sptr ExecParamSql(
        const char *command,
        int paramCount,
        const char * const *paramValues,
        const int * paramSizes,
        const int * paramFormats )
    {
        csm::db::DBResult_sptr ret = nullptr;

        PGresult *res = PQexecParams( 
            _pg_conn, 
            command,
            paramCount,
            NULL, 
            paramValues,
            paramSizes,
            paramFormats,
            0 );

        if ( res )
        {
            ret = std::make_shared<csm::db::DBResult>(res);
        }

        return ret;
    }

  inline csm::db::DBResult_sptr ExecSql(const char *sql)
  {
    csm::db::DBResult_sptr ret = nullptr;
    
    PGresult *res = PQ_Exec(_pg_conn, sql);
    if (res)
      ret = std::make_shared<csm::db::DBResult>(res);
      
    return ret;
  }
  
  inline bool IsIdleConnection()
  {
    bool ret = (_pg_conn == nullptr)? false: (PQtransactionStatus(_pg_conn) == PQTRANS_IDLE);
    return ret;
  }
 
  inline bool IsConnectionSuccess()
  {
    return ( _pg_conn && PQ_CONN_SUCCESS(_pg_conn) );
  }
   
  void ResetConnection()
  {
    if (!_pg_conn) return;
    
    PGTransactionStatusType status = PQtransactionStatus(_pg_conn);
    //note: PQTRANS_IDLE should be the expected state
    if (status == PQTRANS_ACTIVE || status == PQTRANS_INTRANS)
    {
      LOG(csmdb, trace) << "DBConnecion::ResetConnection(): Connection is still in a valid transaction.";
    }
    else if (status == PQTRANS_UNKNOWN)
    {
      LOG(csmdb, trace) << "DBConnecion::ResetConnection(): Connection is in bad state.";
    }
    
    if (status != PQTRANS_IDLE)
    {
      PQreset(_pg_conn);
    }
  }
  
  ~DBConnection()
  {
    //LOG(csmdb, trace) << "Calling PQ_CONN_FREE...";
    if (_pg_conn) PQ_CONN_FREE(_pg_conn);
  }
  
private:
  PGconn* _pg_conn;
  unsigned _id;
  DBCONN_STATE _state;
};

typedef std::shared_ptr<DBConnection> DBConnection_sptr;

} // end namespace db
} // end namespace csm

#endif
