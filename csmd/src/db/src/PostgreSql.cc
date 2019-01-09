/*================================================================================

    csmd/src/db/src/PostgreSql.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "../include/PostgreSql.h"

PGconn* PQ_CreateConn(const char *server, const char *user, 
	const char *passwd, const char *dbName)
{

  if (server == NULL || strlen(server) <= 0) {
    LOG(csmdb, error) << "NULL or Empty in the server parameter in PQ_CreateConn";
    return NULL;
  }

  // for NULL char
  size_t len=1;
  len += strlen(server)+strlen("host=");
  if (user != NULL && strlen(user) > 0) len += strlen(user) + 1 + strlen("user=");
  if (passwd != NULL && strlen(passwd) > 0) len += strlen(passwd) + 1 + strlen("password=");
  if (dbName != NULL && strlen(dbName) > 0) len += strlen(dbName) + 1 + strlen("dbname=");

  char *info = NULL;
  info = (char *) malloc(sizeof(char)*len);

  // If the info wasn't initialized return.
  if (!info ) return NULL;

  char *tmp = info;
  
  sprintf(info, "host=%s", server);
  tmp += strlen(info);

  if (user != NULL && strlen(user) > 0) {
    sprintf(tmp," user=%s", user);
    tmp += strlen(tmp);
  }

  if (passwd != NULL && strlen(passwd) > 0) {
    sprintf(tmp," password=%s", passwd);
    tmp += strlen(tmp);
  }
  
  if (dbName != NULL && strlen(dbName) > 0) {
    sprintf(tmp," dbname=%s", dbName);
    tmp += strlen(tmp);
  }

  // make sure we construct the info right
  if (strlen(info) != (len - 1))
  {
    LOG(csmdb, error) << "Fail to construct the parameters for PGconnectdb...";
    free(info);

    return NULL;
  }

  PGconn *conn =  PQconnectdb(info);
  if (conn == NULL) {
    LOG(csmdb, error) << "Return NULL from PQConnectdb";
  }

  free(info);

  return conn;
}

PGresult *PQ_Exec(PGconn *conn, const char *command)
{
  if (conn == NULL) {
    LOG(csmdb, error) << "NULL in one of the parameters in PQ_Exec";
    return NULL;
  }
  if (command == NULL) {
    //LOG(csmdb, error) << "NULL in the command parameter in PQ_Exec";
    return NULL;
  }

  PGresult *pgres = PQexec(conn, command);
  if (pgres == NULL) {
    LOG(csmdb, error) << "Return NULL from PQexec";
  }

  return pgres;
}

PGresult *PQ_Exec_Param( PGconn *db, const char *command, int paramCount, const char * const *paramValues )
{
    // Verify that the supplied values were valid.
    if ( db == NULL )
    {
        LOG(csmdb, error) << "No connection was supplied to PQ_EXEC_PARAM";
        return NULL;
    }

    if ( command == NULL )
    {
        LOG(csmdb, error) << "No command was supplied to PQ_EXEC_PARAM";
        return NULL;
    }

    if ( paramValues == NULL )
    {
        LOG(csmdb, error) << "Parameters were not properly set for PQ_EXEC_PARAM";
        return NULL;
    }

    PGresult *pgres = PQexecParams( db, command, paramCount, NULL, paramValues, NULL, NULL, 0 );
    if ( pgres == NULL )
    {
        LOG(csmdb, error) << "PQ_EXEC_PARAM Failed";
    }

    return pgres;

}
 
void PQ_DeleteConn( PGconn* db )
{
  if( db != nullptr )
    PQfinish( db );
}
