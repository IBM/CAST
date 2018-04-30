/*================================================================================

    csmnet/src/CPP/endpoint_unix.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_NETWORK_SRC_CPP_ENDPOINT_UNIX_H_
#define CSM_NETWORK_SRC_CPP_ENDPOINT_UNIX_H_

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <netinet/in.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <csm_network_config.h>
#include "address.h"
#include "endpoint_buffer.h"
#include "csm_network_msg_cpp.h"
#include "endpoint.h"

namespace csm {
namespace network {

class EndpointUnix : public Endpoint {
  int _Socket;
  char *_CtrlBuf;
  EndpointStateUnix _RecvBufferState;
  EndpointOptionsUnix_sptr _UnixOptions;
  size_t _MaxPayloadLen;

public:
  EndpointUnix( const Address_sptr aLocalAddr,
                const EndpointOptions_sptr aOptions );
  EndpointUnix( const std::string &aPath,
                const EndpointOptions_sptr aOptions );
  EndpointUnix( const Endpoint *aEP );
  virtual ~EndpointUnix();

  virtual int Connect( const Address_sptr aSrvAddr );
  virtual Endpoint* Accept( )
  {
    // not a connection-based socket, return nothing on Accept
    return nullptr;
  }

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                         const Address_sptr aRemoteAddr );
  virtual ssize_t Send( const csm::network::Message &aMsg );

  // message based receive
  virtual ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr );
  virtual ssize_t Recv( csm::network::Message &aMsg );

  virtual bool DataPending() const { return ! _RecvBufferState.IsEmpty(); }

  virtual NetworkCtrlInfo* Sync( const csm::network::SyncAction aSync = csm::network::SyncAction::SYNC_ACTION_ALL );
  virtual int GetSocket() const { return _Socket; }

private:
  bool IsServer() const { return _UnixOptions.get()->_IsServer; }
  ssize_t SendMsgWrapper( struct msghdr *aMsg, const int aFlags );
};

}  // namespace csm_network
}
#endif /* CSM_NETWORK_SRC_CPP_ENDPOINT_UNIX_H_ */
