/*================================================================================

    csmd/src/daemon/include/csm_daemon_exception.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_daemon_exception.h
 *
 ******************************************/

#ifndef __CSM_DAEMON_EXCEPTION_H__
#define __CSM_DAEMON_EXCEPTION_H__

#include <string>
#include <exception>

namespace csm {
namespace daemon {

class Exception : public std::exception {
  std::string _Msg;
  int _Error;

public:
    Exception( const std::string &aMsg = "", const int aError = errno ) : std::exception(), _Error(aError)
    {
      _Msg = aMsg;
    }
    virtual const char* what() const throw()
    {
        std::string rs = std::string( "CSM Daemon Error: " );
        rs.append( _Msg );
        rs.append( std::string( " rc=" ) );
        rs.append( std::to_string( _Error ) );
        return rs.c_str();
    }

};

}  // namespace daemon
} // namespace csm

#endif // __CSM_DAEMON_EXCEPTION_H__
