/*================================================================================

    csmnet/src/CPP/endpoint_ptp_plain.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_PTP_PLAIN_H_
#define CSMNET_SRC_CPP_ENDPOINT_PTP_PLAIN_H_

// only special includes here since it's not directly included



namespace csm {
namespace network {

/* Notes: TCP endpoint
 *  * only covers a single endpoint (i.e. creates a separate endpoint in accept() )
 *  * IsServer() is true only for the listening socket
 *  * Server/listening socket is set to non-blocking
 *  * other sockets provide non-blocking operations via flags
 */

class EndpointCompute_plain : public EndpointPTP_base {

protected:
  EndpointCompute_plain()
  {}
public:
  EndpointCompute_plain( const Address_sptr aLocalAddr,
               const EndpointOptions_sptr aOptions );
  EndpointCompute_plain( const int aSocket,
               const Address_sptr aLocalAddr,
               const EndpointOptions_sptr aOptions );
  EndpointCompute_plain( const Endpoint * aEndpoint );
  virtual ~EndpointCompute_plain( );

  /* Connects to a server and returns the address
   */
  virtual int Connect( const Address_sptr aSrvAddr );
  virtual Endpoint* Accept( );

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const Address_sptr aRemoteAddr );
};


}  // namespace network
} // namespace csm

#endif /* CSMNET_SRC_CPP_ENDPOINT_PTP_H_ */
