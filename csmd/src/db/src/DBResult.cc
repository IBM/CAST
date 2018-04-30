/*================================================================================

    csmd/src/db/src/DBResult.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "../include/DBResult.h"
#include "../include/DBConnectionPool.h"

namespace csm {
namespace db {

DBConnectionPool* DBConnectionPool::_instance = nullptr;

void DB_TupleFree(DBTuple *tuple)
{
  if (tuple) {
    if (tuple->data)
    {
      free(tuple->data[0]);
      free(tuple->data);
    }
    free(tuple);
  }
}

} //namespace db
} //namespace csm


