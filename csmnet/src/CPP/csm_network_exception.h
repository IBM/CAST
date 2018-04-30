/*================================================================================

    csmnet/src/CPP/csm_network_exception.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSM_NETWORK_EXCEPTION_H__
#define __CSM_NETWORK_EXCEPTION_H__

#include <string>
#include <exception>
#include <errno.h>
#include <string.h>

namespace csm {
namespace network {

class Exception : public std::exception {
  std::string _Msg;
  int _ErrorCode;   // storing error code in Exception since errno might get overwritten by other threads or calls

public:
  Exception( const std::string &aMsg = "", const int aRC = errno )
  : std::exception(),
    _ErrorCode( aRC )
  {
    _Msg = "CSM Network Error: " + aMsg + " rc= " + std::to_string( _ErrorCode ) + "(" + strerror( _ErrorCode ) + ")";
  }
  virtual const char* what() const throw()
  {
    return _Msg.c_str();
  }
  int GetErrno() const { return _ErrorCode; }
};

class ExceptionRecv : public Exception {
public:
  ExceptionRecv( const std::string &aMsg = "", const int aRC = errno )
  : Exception( std::string( "Recv: " ) + aMsg, aRC )
  {}
};

class ExceptionSend: public Exception {
public:
  ExceptionSend( const std::string &aMsg = "", const int aRC = errno )
  : Exception( std::string( "Send: " + aMsg ), aRC )
  {}
};

class ExceptionEndpointDown : public Exception {
public:
  ExceptionEndpointDown( const std::string &aMsg = "", const int aRC = errno )
  : Exception( aMsg, aRC )
  {}
};

class ExceptionProtocol : public Exception {
public:
  ExceptionProtocol( const std::string &aMsg = "", const int aRC = errno)
  : Exception( aMsg, aRC )
  {}
};

class ExceptionFatal : public Exception {
public:
  ExceptionFatal( const std::string &aMsg, const int aRC = errno )
  : Exception( aMsg, aRC )
  {}
};

}  // namespace network
} // namespace csm

#endif // __CSM_NETWORK_EXCEPTION_H__
