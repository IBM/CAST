/*================================================================================

    csmd/src/db/include/PostgreSql.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef _POSTGRESQL_H
#define _POSTGRESQL_H

#include <libpq-fe.h>

#define PQ_CONN_ERROR_MSG(conn) PQerrorMessage(conn)
#define PQ_CONN_STATUS_CODE(conn) PQstatus(conn)
#define PQ_CONN_SUCCESS(conn) (PQstatus(conn) == CONNECTION_OK)
#define PQ_CONN_FREE(conn) PQfinish(conn)

#define PQ_RES_ERROR_MSG(res) PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY)
//PQresultErrorMessage(res)
#define PQ_RES_ERROR_CODE(res) PQresultErrorField(res, PG_DIAG_MESSAGE_HINT)
#define PQ_RES_STATUS_CODE(res) PQresultStatus(res)
#define PQ_RES_SUCCESS(res) (PQresultStatus(res) == PGRES_COMMAND_OK \
                             || PQresultStatus(res) == PGRES_TUPLES_OK)
#define PQ_RES_FREE(res) PQclear(res)

#define PQ_RES_GET_NFIELDS(res) PQnfields(res)
#define PQ_RES_GET_NTUPLES(res) PQntuples(res)
#define PQ_RES_GET_FNAME(res, col) PQfname(res, col)
#define PQ_RES_GET_VALUE(res, row, col) PQgetvalue(res, row, col)

#define PQ_RES_GET_AFFECTED_NTUPLES(res) PQcmdTuples(res)

PGconn* PQ_CreateConn(const char *server, const char *user, const char *passwd, const char *dbName);
void PQ_DeleteConn( PGconn* db );

PGresult *PQ_Exec(PGconn *db, const char *command);

/** 
 * @brief Executes the libpq function PQexecParams.
 *
 * @param[in] db The postgresql connection to the database.
 * @param[in] command The parameterized command to send to the database.
 * @param[in] paramCount The number of parameters to send to the query (size of @ref paramValues)
 * @param[in] paramValues The values of the parameterized query.
 */
PGresult *PQ_Exec_Param( PGconn *db, const char *command, int paramCount, const char * const *paramValues );
#endif

