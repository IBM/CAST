/*================================================================================

    csmd/src/daemon/include/bds_info.h

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_BDS_INFO_H_
#define CSMD_SRC_DAEMON_INCLUDE_BDS_INFO_H_

#include <string>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

namespace csm {
namespace daemon {


class BDS_Info
{
  std::string _Hostname;
  std::string _Port;

public:
  BDS_Info( const std::string host = "",
            const std::string port = "" )
  {
    if(( host != "" ) && ( port != "" ))
      Init( host, port );
    else
    {
      _Hostname = "";
      _Port = "";
    }
  }

  ~BDS_Info() {}

  std::string GetHostname() const { return _Hostname; }
  std::string GetPort() const { return _Port; }

  void Init( const std::string host,
            const std::string port )
  {
    _Hostname = host;

    if( _Hostname.length() > HOST_NAME_MAX )
      throw csm::daemon::Exception("Hostname found too long while initializing BDS_Info");

    // check if the port is valid
    errno = 0;
    int pn = strtoimax( port.c_str(), nullptr, 10 );
    if(( errno == ERANGE ) || ( pn == INTMAX_MAX ) || ( pn <= 0 ) || ( pn > 65535 ))
      throw csm::daemon::Exception("Invalid port found while initializing BDS_info");

    _Port = port;
  }

  bool Active() const { return ! ( _Hostname.empty() || _Port.empty() ) ; }
};

}  // daemon
} // csm


#endif /* CSMD_SRC_DAEMON_INCLUDE_BDS_INFO_H_ */
