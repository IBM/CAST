/*================================================================================

    csmnet/src/CPP/endpoint_utility.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_UTILITY_H_
#define CSMNET_SRC_CPP_ENDPOINT_UTILITY_H_

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <netinet/in.h>

#include <logging.h>
#include <csm_network_config.h>
#include "csm_network_exception.h"
#include "address.h"
#include "csm_network_msg_cpp.h"
#include "endpoint_buffer.h"
#include "csm_message_and_address.h"
#include "endpoint_options.h"
#include "endpoint.h"

namespace csm {
namespace network {

/* Notes: TCP endpoint
 *  * only covers a single endpoint (i.e. creates a separate endpoint in accept() )
 *  * IsServer() is true only for the listening socket
 *  * Server/listening socket is set to non-blocking
 *  * other sockets provide non-blocking operations via flags
 */

class EndpointUtility_plain : public virtual EndpointPTP_base
{

public:
  EndpointUtility_plain( const Address_sptr aLocalAddr,
                         const EndpointOptions_sptr aOptions );
  EndpointUtility_plain( const int aSocket,
                         const Address_sptr aLocalAddr,
                         const EndpointOptions_sptr aOptions )
  : csm::network::EndpointPTP_base( aSocket, aLocalAddr, aOptions )
  {}
  EndpointUtility_plain( const Endpoint * aEndpoint )
  : EndpointPTP_base( aEndpoint )
  {}
  virtual ~EndpointUtility_plain( ) {}

  /* Connects to a server and returns the address
   */
  virtual int Connect( const Address_sptr aSrvAddr );
  virtual Endpoint* Accept( );

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const Address_sptr aRemoteAddr );
};




class EndpointUtility_sec : public virtual EndpointPTP_sec_base
{

public:
  EndpointUtility_sec( const Address_sptr aLocalAddr,
                         const EndpointOptions_sptr aOptions );
  EndpointUtility_sec( const int aSocket,
                       SSL *aSSL,
                       BIO *aBIO,
                       const Address_sptr aLocalAddr,
                       const EndpointOptions_sptr aOptions )
  : EndpointPTP_sec_base( aSocket, aSSL, aBIO, aLocalAddr, aOptions )
  {}

  EndpointUtility_sec( const Endpoint * aEndpoint );
  virtual ~EndpointUtility_sec( ) {}

  /* Connects to a server and returns the address
   */
  virtual int Connect( const Address_sptr aSrvAddr );
  virtual Endpoint* Accept( );

  ssize_t SendTo( const csm::network::Message &aMsg,
                  const Address_sptr aRemoteAddr );
};



}  // namespace network
} // namespace csm

#endif /* CSMNET_SRC_CPP_ENDPOINT_UTILITY_H_ */
