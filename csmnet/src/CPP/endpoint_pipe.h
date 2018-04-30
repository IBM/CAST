/*================================================================================

    csmnet/src/CPP/endpoint_pipe.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_PIPE_H_
#define CSMNET_SRC_CPP_ENDPOINT_PIPE_H_

#include <unistd.h>
#include <fcntl.h>
#include "logging.h"

namespace csm {
namespace network {

class EndpointPipe : public Endpoint
{
  int _Sockets[2];
  mutable char _Buffer[8];

public:
  EndpointPipe( const csm::network::Address_sptr aLocalAddr,
                const csm::network::EndpointOptions_sptr aOptions )
  : csm::network::Endpoint( aLocalAddr, aOptions )
  {
    if( pipe2( _Sockets, O_NONBLOCK ) )
      throw csm::network::Exception( "Failed to create pipe endpoint.", errno );
  }

  EndpointPipe( const csm::network::Endpoint * aEndpoint )
  : csm::network::Endpoint( aEndpoint )
  {
    _Sockets[0] = dynamic_cast<const csm::network::EndpointPipe*>( aEndpoint )->_Sockets[0];
    _Sockets[1] = dynamic_cast<const csm::network::EndpointPipe*>( aEndpoint )->_Sockets[1];
    LOG( csmnet, debug ) << "Creating copy of pipe endpoint.";
  }

  virtual ~EndpointPipe( )
  {
    close( _Sockets[0] );
    close( _Sockets[1] );
    LOG(csmnet, debug ) << "Deleting Pipe endpoint: ";
  }

  /* Connects to a server and returns the address
   */
  virtual int
  Connect( const csm::network::Address_sptr aSrvAddr ) { return _Connected = true; }

  virtual csm::network::Endpoint*
  Accept( ) { return nullptr; }

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const csm::network::Address_sptr aRemoteAddr )
  { char buf = 'w'; return write( _Sockets[1], &buf, 1 ); }

  virtual ssize_t Send( const csm::network::Message &aMsg )
  { return SendTo( aMsg, _RemoteAddr ); }

  // message based receive
  virtual ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
  { return Recv( aMsgAddr._Msg ); }

  ssize_t Recv( csm::network::Message &aMsg )
  { return 0; }

  virtual bool DataPending() const
  {
    ssize_t rc = read( _Sockets[0], _Buffer, 1 );
    LOG(csmnet, trace ) << "DataPending. rc=" << rc << " errno= " << errno;
    return rc > 0;
  }

  /* data synchronization, e.g. flush any buffers */
  virtual NetworkCtrlInfo* Sync( const SyncAction aSync )
  { return nullptr; }

  virtual  int GetSocket() const
  { return _Sockets[0]; }
};



}   // network
}  // csm

#endif /* CSMNET_SRC_CPP_ENDPOINT_PIPE_H_ */
