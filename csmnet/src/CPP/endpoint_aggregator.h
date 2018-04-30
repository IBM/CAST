/*================================================================================

    csmnet/src/CPP/endpoint_aggregator.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_AGGREGATOR_H_
#define CSMNET_SRC_CPP_ENDPOINT_AGGREGATOR_H_

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

class EndpointAggregator_plain : public EndpointPTP_base
{

public:
  EndpointAggregator_plain( const Address_sptr aLocalAddr,
                            const EndpointOptions_sptr aOptions );
  EndpointAggregator_plain( const int aSocket,
                            const Address_sptr aLocalAddr,
                            const EndpointOptions_sptr aOptions )
  : EndpointPTP_base( aSocket, aLocalAddr, aOptions )
  {}
  EndpointAggregator_plain( const Endpoint * aEndpoint )
  : EndpointPTP_base( aEndpoint )
  {}
  virtual ~EndpointAggregator_plain( ) {}

  /* Connects to a server and returns the address
   */
  virtual int Connect( const Address_sptr aSrvAddr );
  virtual Endpoint* Accept( );

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const Address_sptr aRemoteAddr );
};




class EndpointAggregator_sec : public virtual EndpointPTP_sec_base
{
protected:
  EndpointAggregator_sec()
  : EndpointPTP_sec_base()
  {}

public:
  EndpointAggregator_sec( const Address_sptr aLocalAddr,
                          const EndpointOptions_sptr aOptions );
  EndpointAggregator_sec( const int aSocket,
                          SSL *aSSL,
                          BIO *aBIO,
                          const Address_sptr aLocalAddr,
                          const EndpointOptions_sptr aOptions )
  : EndpointPTP_sec_base( aSocket, aSSL, aBIO, aLocalAddr, aOptions )
  {}

  EndpointAggregator_sec( const Endpoint * aEndpoint )
  : EndpointPTP_sec_base( aEndpoint )
  {}
  virtual ~EndpointAggregator_sec( ) {}

  virtual int Connect( const csm::network::Address_sptr aSrvAddr );
  virtual Endpoint* Accept( );

  ssize_t SendTo( const Message &aMsg,
                  const Address_sptr aRemoteAddr );
};



}  // namespace network
} // namespace csm

#endif /* CSMNET_SRC_CPP_ENDPOINT_AGGREGATOR_H_ */
