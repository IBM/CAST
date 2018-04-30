/*================================================================================

    csmnet/src/CPP/endpoint_multi_unix.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_MULTI_UNIX_H_
#define CSMNET_SRC_CPP_ENDPOINT_MULTI_UNIX_H_

#include <map>
#include <deque>
#include <chrono>

#include "logging.h"

#include "address.h"
#include "endpoint_heartbeat.h"
#include "endpoint.h"
#include "endpoint_unix.h"
#include "endpoint_dual_unix.h"

namespace csm {
namespace network {

// keeps a list of known unix endpoints and puts unknown endpoints into an "ACCEPT"-list

class EndpointVirtualUnix;

typedef std::map<csm::network::AddressCode, csm::network::EndpointVirtualUnix*> EndpointVirtualMap;
typedef std::deque<csm::network::EndpointVirtualUnix*> EndpointVirtualFifo;

// this is holding the master Unix socket
// if it's a server socket, it's going to be unconnected (and will stay unconnected)
class EndpointMultiUnix : public EndpointDualUnix {
  typedef std::chrono::time_point< std::chrono::system_clock > TimeType;
  EndpointVirtualMap _Known;
  EndpointVirtualFifo _ToAccept;
  EndpointVirtualFifo _Removed;
  EndpointHeartbeat_stateless _Heartbeat;
  EndpointOptionsUnix_sptr _UnixOptions;

public:
  EndpointMultiUnix( const Address_sptr aLocalAddr = nullptr,
                     const EndpointOptions_sptr aOptions = nullptr );
  EndpointMultiUnix( const std::string &aPath,
                     const EndpointOptions_sptr aOptions );
  EndpointMultiUnix( const Endpoint *aEP );
  virtual ~EndpointMultiUnix();

  /* Connects to a server and returns the address
   */
  virtual int Connect( const Address_sptr aSrvAddr );
  virtual Endpoint* Accept( );

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const Address_sptr aRemoteAddr );
  virtual ssize_t Send( const csm::network::Message &aMsg );

  // message based receive
  virtual ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr );
  virtual ssize_t Recv( csm::network::Message &aMsg );

  virtual bool DataPending() const { return EndpointDualUnix::DataPending(); }

  /* data synchronization, e.g. flush any buffers */
  virtual NetworkCtrlInfo* Sync( const csm::network::SyncAction aSync = csm::network::SYNC_ACTION_ALL );

  Endpoint* GetEndpoint( const csm::network::Address_sptr aAddr ) const;
  Endpoint* GetEndpoint( const csm::network::AddressCode aKey ) const;

  bool CheckRemoteAddress( const csm::network::Address_sptr aAddr ) const;
private:
  // check if remote endpoint is already in the endpoint list (otherwise: create and add new endpoint)
  bool CheckAndAddRemoteAddress( const csm::network::Address_sptr aAddr,
                                 const bool UpdateHeartbeat = false );

  void RemoveFromClientRegistry( const csm::network::Address_sptr aAddr );
};




/*
 * virtual unix endpoint only contains a reference to an actual Unix endpoint
 * - it's used to prevent creation and destruction of the unix socket file
 *   when working with multiple unix endpoints
 * - always comes connected (requires remote and local address to be known)
 * - will send disconnect message on destruction (if still connected)
 */

class EndpointVirtualUnix : public Endpoint
{
  EndpointMultiUnix *_Master;
  EndpointHeartbeat_stateless _Heartbeat;  // need a separate heartbeat here because

public:
  EndpointVirtualUnix( const csm::network::Address_sptr aLocalAddr,
                       const csm::network::EndpointOptions_sptr aOptions )
  : csm::network::Endpoint( aLocalAddr, aOptions ),
    _Master( nullptr ),
    _Heartbeat( CSM_NETWORK_HEARTBEAT_INTERVAL * 3, CSM_PRIORITY_NO_ACK ) // x3 to allow 3 fails before terminating
  {}

  EndpointVirtualUnix( const csm::network::Endpoint * aEndpoint )
  : csm::network::Endpoint( aEndpoint ),
    _Master( dynamic_cast<const csm::network::EndpointVirtualUnix*>( aEndpoint )->GetMaster() ),
    _Heartbeat( dynamic_cast<const csm::network::EndpointVirtualUnix*>( aEndpoint )->_Heartbeat )
  {}

  EndpointVirtualUnix( const csm::network::EndpointMultiUnix *aMaster,
                       const csm::network::AddressUnix_sptr aRemoteAddress )
  : csm::network::Endpoint( aMaster->GetLocalAddr(), nullptr ),
    _Master( const_cast<csm::network::EndpointMultiUnix*>( aMaster ) ),
    _Heartbeat( CSM_NETWORK_HEARTBEAT_INTERVAL * 3, CSM_PRIORITY_NO_ACK ) // x3 to allow 3 fails before terminating
  {
    SetOptions( std::make_shared<csm::network::EndpointOptionsUnix>( false ) );
    _Connected = ( false );
    SetRemoteAddr( aRemoteAddress );
    LOG(csmnet, debug ) << "Creating Client endpoint: " << _RemoteAddr->Dump();
  }

  virtual ~EndpointVirtualUnix( )
  {
    if( IsConnected() )
      Disconnect();
    if( _RemoteAddr != nullptr )
      LOG(csmnet, debug ) << "Deleting Client endpoint: " << _RemoteAddr->Dump();
  }

  /* Since we're only implementing the server side here
   * all we need is to set the remote addr (client) and
   * update the connection status to true
   */
  virtual int
  Connect( const csm::network::Address_sptr aSrvAddr )
  {
    SetRemoteAddr( aSrvAddr );
    _Connected = true;
    return 0;
  }

  virtual int
  Disconnect()
  {
    csm::network::Message DisconnectMsg;
    if(( _RemoteAddr == nullptr ) || ( _LocalAddr == nullptr ))
    {
      LOG( csmnet, warning ) << "Trying to disconnect a Local Endpoint that has no peer address.";
      _Connected = false;
      return ENOTCONN;
    }
    LOG( csmnet, debug ) << "Client connectionstatus " << IsConnected() <<
        " addr: " << GetRemoteAddr()->Dump();
    if(( IsConnected() ) && ( GetRemoteAddr()->MakeKey() != GetLocalAddr()->MakeKey() ))
    {
      DisconnectMsg.Init( CSM_CMD_STATUS,
                          CSM_HEADER_INT_BIT | CSM_HEADER_CBK_BIT,
                          CSM_PRIORITY_NO_ACK,
                          1, 0x1234, 0x1234,
                          geteuid(), getegid(),
                          std::string( CSM_DISCONNECT_MSG ));
      try
      {
        Send( DisconnectMsg );
        DisconnectMsg.ClrCbk();
        DisconnectMsg.CheckSumUpdate();
        Send( DisconnectMsg );
      }
      catch( csm::network::Exception &e )
      {
        LOG( csmnet, debug ) << "Client already down. Disconnect message could not be sent to " << GetRemoteAddr()->Dump();
        _Connected = false;
        return e.GetErrno();
      }
    }
    _Connected = false;
    return 0;
  }

  virtual csm::network::Endpoint*
  Accept( ) { return nullptr; }

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const csm::network::Address_sptr aRemoteAddr )
  { return _Master->SendTo( aMsg, aRemoteAddr ); }

  virtual ssize_t Send( const csm::network::Message &aMsg )
  { return _Master->SendTo( aMsg, _RemoteAddr ); }

  // message based receive
  virtual ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
  { return _Master->RecvFrom( aMsgAddr ); }

  ssize_t Recv( csm::network::Message &aMsg )
  { return _Master->Recv( aMsg ); }

  virtual bool DataPending() const { return _Master->DataPending(); }

  /* data synchronization, e.g. flush any buffers */
  virtual NetworkCtrlInfo* Sync( const SyncAction aSync )
  { return _Master->Sync(); }

  virtual  int GetSocket() const
  { return _Master->GetSocket(); }

  inline csm::network::EndpointMultiUnix* GetMaster() const
  {
    return _Master;
  }
  inline csm::network::EndpointHeartbeat_stateless* GetHeartbeat()
  {
    return &_Heartbeat;
  }

};


}   // namespace network
}  // namespace csm

#endif /* CSMNET_SRC_CPP_ENDPOINT_MULTI_UNIX_H_ */
