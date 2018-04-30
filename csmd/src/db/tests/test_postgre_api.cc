/*================================================================================

    csmd/src/db/tests/test_postgre_api.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include "../include/PostgreSql.h"
#include <assert.h>

int main() {
   int i, j;

   PGconn *conn = PQ_CreateConn("10.4.0.223", "postgres", NULL, "template1");

   if (!PQ_CONN_SUCCESS(conn)) {
      fprintf(stderr, "Connection to database failed: %d %s\n",
              PQ_CONN_STATUS_CODE(conn), PQ_CONN_ERROR_MSG(conn));
      PQ_CONN_FREE(conn);
      exit(-1);
   }
   else {
      printf("Connected...\n");
   }

   PGresult *res = PQ_Exec(conn, "drop table cities");

   if (!PQ_RES_SUCCESS(res)) {
      fprintf(stderr, "Drop Table failed: %d %s\n",
              PQ_RES_STATUS_CODE(res), PQ_RES_ERROR_MSG(res));
      PQ_RES_FREE(res);
   } 

   res = PQ_Exec(conn, "create table cities (name varchar(80),location name)");
   assert(PQ_RES_SUCCESS(res));
   PQ_RES_FREE(res);

   res = PQ_Exec(conn, "insert into cities values('NY', 'NY_LOCATION')");
   assert(PQ_RES_SUCCESS(res));
   PQ_RES_FREE(res);

   res = PQ_Exec(conn, "insert into cities values('SF', 'SF_LOCATION')");
   assert(PQ_RES_SUCCESS(res));

   int nfields = PQ_RES_GET_NFIELDS(res);
   assert(nfields == 0);
   PQ_RES_FREE(res);

   res = PQ_Exec(conn, "SELECT * FROM cities");
   assert(PQ_RES_SUCCESS(res));
   
   nfields = PQ_RES_GET_NFIELDS(res);
   assert(nfields > 0);

   for (i=0;i<nfields;i++) {
     printf("\t%s", PQ_RES_GET_FNAME(res, i));
   }
   printf("\n");

   int nrows = PQ_RES_GET_NTUPLES(res);
   assert(nrows != 0);

   for (i=0;i<nrows;i++) {
     for (j=0;j<nfields;j++) {
       printf("\t%s", PQ_RES_GET_VALUE(res, i, j));
     }
     printf("\n");
   }

   PQ_RES_FREE(res);

   PQ_CONN_FREE(conn);

   return 0;
}
