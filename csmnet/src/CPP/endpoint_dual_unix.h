/*================================================================================

    csmnet/src/CPP/endpoint_dual_unix.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_DUAL_UNIX_H_
#define CSMNET_SRC_CPP_ENDPOINT_DUAL_UNIX_H_

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <netinet/in.h>

#include <csm_network_config.h>
#include "address.h"
#include "csm_network_msg_cpp.h"
#include "endpoint_buffer.h"
#include "endpoint.h"
#include "endpoint_unix.h"

namespace csm {
namespace network {

class EndpointDualUnix : public Endpoint {
  csm::network::EndpointUnix *_Default;
  csm::network::EndpointUnix *_Callback;
  csm::network::EndpointOptionsDual_sptr _DualOptions;
  int _CallBackRefCount;

public:
  EndpointDualUnix( const Address_sptr aLocalAddr,
                    const EndpointOptions_sptr aOptions );
  EndpointDualUnix( const std::string &aPath,
                    const EndpointOptions_sptr aOptions );
  EndpointDualUnix( const Endpoint *aEP );
  virtual ~EndpointDualUnix();

  virtual int Connect( const Address_sptr aSrvAddr );
  virtual Endpoint* Accept( )
  {
    // not a connection-based socket, return nothing
    return nullptr;
  }

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const Address_sptr aRemoteAddr );
  virtual ssize_t Send( const csm::network::Message &aMsg );

  // message based receive
  virtual ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr );
  virtual ssize_t Recv( csm::network::Message &aMsg );

  virtual bool DataPending() const { return _Default->DataPending() | _Callback->DataPending(); }

  virtual NetworkCtrlInfo* Sync( const csm::network::SyncAction aSync = csm::network::SyncAction::SYNC_ACTION_ALL );

  virtual int GetSocket() const { return _Default->GetSocket(); }

private:

  inline std::string GenerateSecondaryAddress( const std::string &aPath )
  {
    return ((std::string)aPath).append( CSM_SECONDARY_SOCKET_APPEND );
  }
  inline csm::network::AddressUnix_sptr GenerateSecondaryAddress( const csm::network::Address_sptr aAddr )
  {
    const csm::network::AddressUnix_sptr uaddr = std::dynamic_pointer_cast<csm::network::AddressUnix>( aAddr );
    char cbPath[ UNIX_PATH_MAX ];
    bzero( cbPath, UNIX_PATH_MAX );
    size_t start_pos = 0;
    if( uaddr->_SockAddr.sun_path[0] == 0 )
      start_pos = 1;
    std::string new_addr = GenerateSecondaryAddress( std::string( &( uaddr->_SockAddr.sun_path[ start_pos ] ) ) );
    size_t length = std::min( new_addr.length(), UNIX_PATH_MAX - start_pos );
    memcpy( &( cbPath[ start_pos ] ),
            new_addr.c_str(),
            length );
    LOG( csmnet, debug ) << "GenerateSecondaryAddress: " << std::string( &( cbPath[start_pos ] ));
    return std::make_shared<csm::network::AddressUnix>( cbPath );
  }
  inline csm::network::AddressUnix_sptr AddressToCBChannel( const csm::network::Address_sptr aAddr )
  {
    const csm::network::AddressUnix_sptr uaddr = std::dynamic_pointer_cast<csm::network::AddressUnix>(aAddr);
    size_t start_pos = 0;
    if( uaddr->_SockAddr.sun_path[0] == 0 )
      start_pos = 1;
    std::string path = std::string( &( uaddr->_SockAddr.sun_path[ start_pos ] ) );
    if( path.find( CSM_SECONDARY_SOCKET_APPEND ) != std::string::npos )
      return std::const_pointer_cast<csm::network::AddressUnix>(uaddr);  // nothing to do if already CB address

    return GenerateSecondaryAddress( aAddr );
  }
};

}  // namespace csm_network
}

#endif /* CSMNET_SRC_CPP_ENDPOINT_DUAL_UNIX_H_ */
