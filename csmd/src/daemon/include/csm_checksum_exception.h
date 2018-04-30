/*================================================================================

    csmd/src/daemon/include/csm_checksum_exception.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_checksum_exception.h
 *
 ******************************************/

#ifndef CSM_DAEMON_SRC_CSM_CHECKSUM_EXCEPTION_H_
#define CSM_DAEMON_SRC_CSM_CHECKSUM_EXCEPTION_H_


#include <string>
#include <exception>

namespace csm {
namespace daemon {

class ChecksumException : public std::exception {
  std::string _Msg;

public:
  ChecksumException( const std::string &aMsg = "") : std::exception()
  {
    _Msg = aMsg;
  }
  virtual const char* what() const throw()
  {
    std::string rs = "CSM Checksum Error: " + _Msg;
    return rs.c_str();
  }
};

}  // namespace daemon
} // namespace csm

#endif /* CSM_DAEMON_SRC_CSM_CHECKSUM_EXCEPTION_H_ */
