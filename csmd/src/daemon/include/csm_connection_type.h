/*================================================================================

    csmd/src/daemon/include/csm_connection_type.h

  © Copyright IBM Corporation 2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_CONNECTION_TYPE_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_CONNECTION_TYPE_H_

#include "csm_daemon_exception.h"

namespace csm {
namespace daemon {

namespace ConnectionType {

enum CONN_TYPE
{
  PRIMARY,
  SECONDARY,
  SINGLE,
  ANY
};

template<class stream>
static stream&
operator<<( stream &out, const enum CONN_TYPE type )
{
  switch( type )
  {
    case PRIMARY:
      out << "PRIMARY";
      break;
    case SECONDARY:
      out << "SECONDARY";
      break;
    case SINGLE:
      out << "SINGLE";
      break;
    case ANY:
      out << "ANY";
      break;
    default:
      throw csm::daemon::Exception("Connection Type out of range");
  }
  return out;
}

} //   ConnectionType

} //  daemon
} // csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_CONNECTION_TYPE_H_ */
