/*================================================================================

    csmd/src/db/tests/test_csmdb_api.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "../include/DBConnectionPool.h"
#include "../include/PostgreSql.h"

#include <pthread.h>
#include <assert.h>

#define NUM_CONNS 10
#define NUM_TRANS 2
#define MAXLEN 512

using namespace csm::db;

struct THREAD_ARG
{
  DBConnection_sptr conn;
  int id;
};

void* do_transaction(void *arg) {
   int i, j;
   char cmd[MAXLEN];
   char table[128];
   
   // create connection
   THREAD_ARG *targ = (THREAD_ARG *)arg;
   DBConnection_sptr conn = targ->conn;
   sprintf(table, "cities_%d", targ->id);

   if (conn == nullptr) {
      fprintf(stderr, "DBERROR: Cannot get a DBConnection");
      exit(-1);
   }
   else {
      printf("Connected... table name=%s\n", table);
   }
   
   // example using the begin/commit transaction
   conn->ExecSql("begin");
   conn->ExecSql("savepoint step1");

   // execute a list of SQL commands
   DBResult_sptr res;
   
   sprintf(cmd, "drop table %s", table);
   res = conn->ExecSql(cmd);
   if (res && res->GetResStatus() != DB_SUCCESS) {
      fprintf(stderr, "DBERROR:%s!\n",
              res->GetErrMsg());
      // have to rollback (or restart) in order to continue the subsequent commands in the same transaction
      conn->ExecSql("rollback to step1");
   }

   sprintf(cmd, "create table %s (name varchar(80),location name)", table);
   res = conn->ExecSql(cmd);
   if (res && res->GetResStatus() != DB_SUCCESS) {
      fprintf(stderr, "DBERROR:%s cmd=%s!\n",
              res->GetErrMsg(), cmd);
      // don't continue if not able to create a table
      return 0;
   }
   
   // will assume no problem with the rest of the sql commands
   sprintf(cmd,"insert into %s values('NY', 'NY_LOCATION')", table);
   res = conn->ExecSql(cmd);
   assert(res && res->GetResStatus() == DB_SUCCESS);

   sprintf(cmd,"insert into %s values('SF', 'SF_LOCATION')", table);
   res = conn->ExecSql(cmd);
   assert(res && res->GetResStatus() == DB_SUCCESS);
    
   DBTuple *fields = res->GetAllFieldNames();
   assert(fields == NULL);

   // fetch the results from the select command
   sprintf(cmd,"SELECT * FROM %s", table);
   res = conn->ExecSql(cmd);
   assert(res && res->GetResStatus() == DB_SUCCESS);

   // print out the field names first
   fields = res->GetAllFieldNames();
   assert(fields != NULL);

   for (i=0; i<fields->nfields; i++) {
       printf("\t%s", fields->data[i]);
   }
   printf("\n==========================\n");
   
   DB_TupleFree(fields);
   
   // now print out the values for each row
   int nrows = res->GetNumOfTuples();
   assert(nrows != 0);

   for (i=0;i<nrows;i++) {
     fields = res->GetTupleAtRow(i);
     for (j=0;j<fields->nfields;j++) {
       printf("\t%s", fields->data[j]);
     }
     printf("\n");
     DB_TupleFree(fields);
   }
    
   printf("\nPrinting by each column...\n");
   int ncols = res->GetNumOfFields();
   assert(ncols != 0);
   for (i=0;i<ncols;i++) {
     fields = res->GetTupleAtCol(i);
     for (j=0;j<fields->nfields;j++) {
       printf("\t%s", fields->data[j]);
     }
     printf("\n");
     DB_TupleFree(fields);
   }

   // check for a non-valid row
   fields = res->GetTupleAtRow(nrows);
   assert(fields == NULL);
   // check for a non-valid column
   fields = res->GetTupleAtRow(-1);
   assert(fields == NULL);
   
   conn->ExecSql("commit");
   
   return 0;
}

int main(int argc, char *argv[])
{
   if (PQisthreadsafe() == 1)
   {
     LOG(csmdb,info) << "libpq is thread safe...";
   }
   else
   {
     LOG(csmdb, info) << "libpq is not thread safe...";
   }
   // create connection
   DBConnInfo dbinfo("10.4.0.223", "postgres", NULL, "template1");
   DBConnectionPool *pool = DBConnectionPool::Init(NUM_CONNS, dbinfo);
   
   pthread_t threads[NUM_TRANS];
   THREAD_ARG arg[NUM_TRANS];
   for (size_t i=0; i<NUM_TRANS && pool->GetNumOfDBConnections() == NUM_CONNS; i++)
   {
     DBConnection_sptr conn = pool->AcquireDBConnection();

     arg[i].conn = conn;
     arg[i].id = i;
     int rc = pthread_create( &threads[i], NULL, do_transaction, static_cast<void *>(&arg[i]) );
     if (rc)
     {
       std::cerr << "Fail to create a thread...";
       exit(-1);
     }
   }
   
   void *status;
   for (size_t i=0; i<NUM_TRANS && pool->GetNumOfDBConnections() == NUM_CONNS; i++)
   {
     int rc = pthread_join(threads[i], &status);
     if (rc)
     {
       std::cerr << "Fail to join a thread...";
       exit(-1);
     }

     pool->ReleaseDBConnection(arg[i].conn);
   }
   
   
   delete pool;
   
   exit(0);

}
