/*================================================================================

    csmd/src/db/include/csm_db_exception.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_network_exception.h
 *
 ******************************************/

#ifndef __CSM_DB_EXCEPTION_H__
#define __CSM_DB_EXCEPTION_H__

#include <string>
#include <exception>

namespace csm {
namespace db {

class Exception : public std::exception {
  std::string _Msg;

public:
    Exception( const std::string &aMsg = "") : std::exception()
    {
      _Msg = aMsg;
    }
    virtual const char* what() const throw()
    {
        std::string rs = "CSM Database Error: " + _Msg + " rc=" + std::to_string( errno );
        return rs.c_str();
    }

};

}  // namespace db
} // namespace csm

#endif // __CSM_DB_EXCEPTION_H__
