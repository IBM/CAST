/*================================================================================

    csmd/src/db/include/DBResult.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef _CSM_DB_SRC_DBRESULT_H
#define _CSM_DB_SRC_DBRESULT_H

#include "PostgreSql.h"

#include <libpq-fe.h>
#include <memory>
#include <string.h>
#include <cstdlib>

#include "logging.h"

namespace csm {
namespace db {

/**
 \brief CSM Database flags for DB interaction
 */
enum BooleanEnum {
  DB_SUCCESS = 1,  ///< Command has completed
  DB_ERROR,        ///< Command failed
};
typedef enum BooleanEnum DB_BOOL;


/**
 \brief Table results from the database
 */
typedef struct {
  int nfields;   ///< the number of strings in data
  char **data;   ///< an array of string 
} DBTuple;


class DBResult
{
public:
  DBResult(PGresult *aRes)
  :_pgres(nullptr)
  {
    _pgres = aRes;
  }
  
  virtual ~DBResult()
  {
    if (_pgres) PQ_RES_FREE(_pgres);
  }
  
  DB_BOOL GetResStatus()
  {
    if ( _pgres && PQ_RES_SUCCESS(_pgres) ) return DB_SUCCESS;
    else return DB_ERROR;
  }

  ExecStatusType GetResStatusCode()
  {
    // if !_pgres, returns PGRES_NONFATAL_ERROR
    return PQ_RES_STATUS_CODE(_pgres);
  }
  
  char *GetErrMsg() {
    if (_pgres) return PQ_RES_ERROR_MSG(_pgres);
    else return nullptr;
  }
  
  size_t GetNumOfFields()
  {
    if (_pgres) return PQ_RES_GET_NFIELDS(_pgres);
    else return 0;
  }
  
  char* GetFieldName(size_t col)
  {
    if (_pgres) return PQ_RES_GET_FNAME(_pgres, col);
    return nullptr;
  }

  // note: callers should use DB_TupleFree() to free the returned pointer
  DBTuple *GetAllFieldNames()
  {
    int len;
    char *longstr=NULL;
    char **dataptr=NULL;
    DBTuple *tuple;
    size_t i;
    
    size_t nfields = GetNumOfFields();
    if (nfields == 0) return NULL;

    tuple = (DBTuple *) malloc(sizeof(DBTuple));
    tuple->nfields = nfields;
    
    len = 0;
    for (i = 0; i < nfields; i++) {
      char *name = GetFieldName(i);
      len += strlen(name) + 1;
    }

    if (len > 0) {
      longstr = (char *) malloc(sizeof(char)*len);
      dataptr = (char **) malloc(sizeof(char *)*nfields);
      for (i = 0; i < nfields; i++) {
        strcpy(longstr, GetFieldName(i));
        dataptr[i] = longstr;
        longstr += strlen(longstr) + 1;
      }
    }

    tuple->data = dataptr;
    return tuple;
  }
    
  size_t GetNumOfTuples()
  {
    if (_pgres) return PQ_RES_GET_NTUPLES(_pgres);
    else return 0;
  }
  
  char *GetValue(int row, int col)
  {
  
    if (_pgres) return PQ_RES_GET_VALUE(_pgres, row, col);
    return nullptr;
  }
  
  size_t GetNumOfAffectedRows()
  {
    size_t n = 0;
    if (_pgres)
    {
      char *s = PQ_RES_GET_AFFECTED_NTUPLES(_pgres);
      if (strlen(s) > 0) n = strtoull(s, NULL, 10);
    }
    return n;
  }
  
  // note: callers should use DB_TupleFree() to free the returned pointer
  DBTuple *GetTupleAtRow(size_t row)
  {
    int len;
    char *longstr=NULL;
    char **dataptr=NULL;
    DBTuple *tuple;
    size_t i;

    size_t nrows = GetNumOfTuples();
    if (nrows == 0 || row >= nrows) return NULL;

    tuple = (DBTuple *) malloc(sizeof(DBTuple));

    size_t nfields = GetNumOfFields();
    tuple->nfields = nfields;

    len = 0;
    for (i = 0; i < nfields; i++) {
    char *name = GetValue(row, i);
      len += strlen(name) + 1;
    }

    if (len > 0) {
      longstr = (char *) malloc(sizeof(char)*len);
      dataptr = (char **) malloc(sizeof(char *)*nfields);
      for (i = 0; i < nfields; i++) {
        strcpy(longstr, GetValue(row, i));
        dataptr[i] = longstr;
        longstr += strlen(longstr) + 1;
      }
    }

    tuple->data = dataptr;
    return tuple;
  }

  // note: callers should use DB_TupleFree() to free the returned pointer
  DBTuple* GetTupleAtCol(size_t col)
  {
    int len;
    char *longstr=NULL;
    char **dataptr=NULL;
    DBTuple *tuple;
    size_t i;
    
    size_t nfields = GetNumOfFields();
    if (nfields == 0 || col >= nfields) return NULL;

    tuple = (DBTuple *) malloc(sizeof(DBTuple));

    size_t nrows = GetNumOfTuples();
    tuple->nfields = nrows;

    len = 0;
    for (i = 0; i < nrows; i++) {
      char *name = GetValue(i, col);
      len += strlen(name) + 1;
    }

    if (len > 0) {
      longstr = (char *) malloc(sizeof(char)*len);
      dataptr = (char **) malloc(sizeof(char *)*nfields);
      for (i = 0; i < nrows; i++) {
        strcpy(longstr, GetValue(i, col));
        dataptr[i] = longstr;
        longstr += strlen(longstr) + 1;
      }
    }

    tuple->data = dataptr;
    return tuple;
  }
  
private:
  PGresult *_pgres;
};

typedef std::shared_ptr<DBResult> DBResult_sptr;

/** 
  \brief Free the DBTuple object
  */
void DB_TupleFree(DBTuple *tuple);

} // end namespace db
} // end namespace csm

#endif
