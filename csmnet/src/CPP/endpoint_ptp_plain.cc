/*================================================================================

    csmnet/src/CPP/endpoint_ptp_plain.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <cstdio>
#include <exception>
#include <iostream>
#include <algorithm>

#include <sys/types.h>   // stat()

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <fcntl.h>       // setting non-blocking socket

#include <errno.h>

#include <logging.h>
#include "csm_network_config.h"
#include "csm_network_exception.h"
#include "csm_network_msg_cpp.h"
#include "address.h"
#include "endpoint_ptp.h"

/**
 *  constructor allows to pass in a local address to bind.
 *  endpoint creates a copy to use, so caller can destroy instance of address anytime
 */
csm::network::EndpointCompute_plain::EndpointCompute_plain( const csm::network::Address_sptr aLocalAddr,
                                        const csm::network::EndpointOptions_sptr aOptions )
: csm::network::EndpointPTP_base( aLocalAddr, aOptions )
{
  csm::network::EndpointOptionsPTP_sptr ptpOptions =
      std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP<csm::network::AddressPTP>>( _Options );

  if( IsServerEndpoint() )
  {
    PrepareServerSocket<csm::network::AddressPTP>();
    SetRemoteAddr( _LocalAddr );
  }
  else
    if( ! ptpOptions->_ServerAddr.IsEmpty() )
    {
      int rc = Connect( std::make_shared<csm::network::AddressPTP>( ptpOptions->_ServerAddr ) );
      if( rc )
        throw csm::network::ExceptionEndpointDown("Socket creation - connect");
    }

}

// Constructing/Initializing a new endpoint for an existing socket
csm::network::EndpointCompute_plain::EndpointCompute_plain( const int aSocket,
                                        const Address_sptr aLocalAddr,
                                        const EndpointOptions_sptr aOptions )
: csm::network::EndpointPTP_base( aSocket, aLocalAddr, aOptions )
{}

csm::network::EndpointCompute_plain::EndpointCompute_plain( const Endpoint *aEP )
: csm::network::EndpointPTP_base( aEP )
{}

csm::network::EndpointCompute_plain::~EndpointCompute_plain( )
{}

int csm::network::EndpointCompute_plain::Connect( const csm::network::Address_sptr aSrvAddr )
{
  int rc = ConnectPrep<csm::network::AddressPTP>( aSrvAddr );

  if( rc == 0 )
    _Connected = true;

  if( _Connected )
    rc = ConnectPost();

  return rc;
}

csm::network::Endpoint* csm::network::EndpointCompute_plain::Accept()
{
  return GenericPTPAccept<csm::network::AddressPTP, csm::network::EndpointCompute_plain>();
}


// message based send
ssize_t
csm::network::EndpointCompute_plain::SendTo( const csm::network::Message &aMsg,
                                   const csm::network::Address_sptr aRemoteAddr )
{
  const csm::network::AddressPTP *addr = dynamic_cast<const csm::network::AddressPTP*>( aRemoteAddr.get() );
  if( addr == nullptr )
    throw csm::network::ExceptionProtocol("Remote address has wrong type", ENOTCONN );

  if( *addr != *(csm::network::AddressPTP*)_RemoteAddr.get() )
    LOG( csmnet, warning ) << "EndpointPTP_plain::SendTo(): Request to send to different address than connected. Given address IGNORED!";

  return Send( aMsg );
}

