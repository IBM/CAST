/*================================================================================

    csmnet/src/CPP/endpoint_ptp.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_PTP_H_
#define CSMNET_SRC_CPP_ENDPOINT_PTP_H_

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>       // setting non-blocking socket

#include <logging.h>
#include "csm_version.h"
#include "csm_version_msg.h"
#include <csm_network_config.h>
#include "csm_network_exception.h"
#include "address.h"
#include "csm_network_msg_cpp.h"
#include "endpoint_buffer.h"
#include "endpoint_heartbeat.h"
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

class EndpointPTP_base : public Endpoint {
protected:
  int _Socket;
  EndpointBuffer _RecvBufferState;
  EndpointHeartbeat _Heartbeat;

  EndpointPTP_base()
  : _Socket(0),
    _RecvBufferState(),
    _Heartbeat()
  {}
public:
  EndpointPTP_base( const Address_sptr aLocalAddr,
                    const EndpointOptions_sptr aOptions );
  EndpointPTP_base( const int aSocket,
                    const Address_sptr aLocalAddr,
                    const EndpointOptions_sptr aOptions );
  EndpointPTP_base( const Endpoint * aEndpoint );
  virtual ~EndpointPTP_base( );


  /* Connects to a server and returns the address
   */
  virtual int Connect( const Address_sptr aSrvAddr ) = 0;
  virtual Endpoint* Accept( ) = 0;

  // message based send
  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const Address_sptr aRemoteAddr ) = 0;
  virtual ssize_t Send( const csm::network::Message &aMsg );

  // message based receive
  virtual ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr );
  virtual ssize_t Recv( csm::network::Message &aMsg );

  virtual bool DataPending() const { return ! _RecvBufferState.IsEmpty(); }

  /* data synchronization, e.g. flush any buffers */
  virtual NetworkCtrlInfo* Sync( const SyncAction aSync = SYNC_ACTION_ALL );

  virtual int GetSocket() const { return _Socket; }

protected:
  virtual ssize_t SendMsgWrapper( const struct msghdr *aMsg, const int aFlags );
  int SetGeneralSockopts();
  void SetHeartbeatAddress( const Address_sptr &aAddr ) { _Heartbeat.setAddr( aAddr ); }

  template<typename AddressClass>
  int PrepareServerSocket()
  {
    int value = 1;
    int rc = setsockopt( _Socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof( int ) );
    if( rc )
      throw csm::network::Exception("setsocket: SO_REUSEADDR");

    rc = bind( _Socket,
               (sockaddr*)&( dynamic_cast<AddressClass*>(_LocalAddr.get())->_SockAddr ),
               sizeof( sockaddr_in ) );
    if( rc )
      throw csm::network::ExceptionEndpointDown("Bind error");

    LOG(csmnet, debug) << "PrepareServerSocket(): bind to " << *dynamic_cast<AddressClass*>(_LocalAddr.get());

    rc = listen( _Socket, 128 );
    if( rc )
      throw csm::network::Exception("Listen");
    LOG(csmnet, debug) << "PrepareServerSocket(): Now listening on addr: "
       << dynamic_cast<AddressClass*>(_LocalAddr.get())->Dump();

    return rc;
  }

  // checks if there's any available data on a socket
  // and retrieves and returns any errors via getsockopt
  int CheckConnectActivity( int aSocket, bool aWithRead = false )
  {
    int rc = 0;

    fd_set fdsW;
    FD_ZERO( &fdsW );
    FD_SET( aSocket, &fdsW );

    fd_set fdsR;
    FD_ZERO( &fdsR );
    FD_SET( aSocket, &fdsR );

    struct timeval timeout;
    timeout.tv_sec = 1; timeout.tv_usec = 0;

    if( aWithRead )
      rc = select( aSocket+1, &fdsR, &fdsW, NULL, &timeout );
    else
      rc = select( aSocket+1, NULL, &fdsW, NULL, &timeout );

    if( rc > 0 )
    {
      socklen_t vallen = sizeof( int );
      if( getsockopt( aSocket, SOL_SOCKET, SO_ERROR, &rc, &vallen ) != 0 )
        throw csm::network::ExceptionEndpointDown( "Failed to retrieve error code from socket after connect." );
      LOG( csmnet, trace) << "ConnectionActivity: status returned: " << rc;
    }
    else
      throw csm::network::ExceptionEndpointDown("Connection Timeout will not retry.", rc );

    return rc;
  }


  template<typename AddressClass>
  int ConnectPrep( const csm::network::Address_sptr aSrvAddr )
  {
    int rc = 0;
    const AddressClass* AddrPTP = dynamic_cast<const AddressClass*>( aSrvAddr.get() );
    sockaddr_in* SrvAddr = (sockaddr_in*) ((&(AddrPTP->_SockAddr)));
    LOG(csmnet, info) << "Connecting socket: " << _Socket << " to " << AddressClass( *SrvAddr ).Dump();

    if( IsServerEndpoint() )
      throw csm::network::ExceptionEndpointDown( "Trying to call connect() on server endpoint.", EBADFD );

    if(( _RemoteAddr != nullptr ) &&
        ( *dynamic_cast<const AddressClass*>( _RemoteAddr.get() ) ==
            *dynamic_cast<const AddressClass*>( aSrvAddr.get() ) ))
    {
      LOG(csmnet, debug) << "Endpoint already connected to destination: " << aSrvAddr->Dump()
          << ". skipping without error."<< std::endl;
      return 0;
    }

    if( _Socket && !IsServerEndpoint() )
    {
      // make/check the socket is non-blocking for the time of the connect
      // we cannot afford a long timeout of a blocking socket here
      long current_setting = fcntl( _Socket, F_GETFL, NULL );
      if( (current_setting & O_NONBLOCK) == 0 )
      {
        current_setting |= O_NONBLOCK;
        rc = fcntl(_Socket, F_SETFL, current_setting );
        if( rc )
          throw csm::network::Exception("fcntl: NONBLOCK");
      }

      rc = 1;
      for( int retries = 1; ((rc > 0) && (retries < 5)); ++retries )
      {
        rc = connect( _Socket, (struct sockaddr *) SrvAddr, sizeof(sockaddr_in) );
        int stored_errno = errno;
        if( rc )
        {
          // check if the non-blocking socket completed the connect() or not, if not, use select+getsockopt to go check completion or error
          if( stored_errno == EINPROGRESS )
          {
            --retries; // this is an incomplete connect, so we cannot count it as a "retry"
            stored_errno = CheckConnectActivity( _Socket );
            if( stored_errno == 0 )
              rc = 0;
          }
          // there are some error cases where we just shouldn't retry immediately
          if(( stored_errno == EALREADY ) || ( stored_errno == EHOSTUNREACH ))
            throw csm::network::ExceptionEndpointDown("Error while connecting. Not retrying immediately", stored_errno );

          LOG(csmnet, debug ) << "Connection attempt " << retries << " to :" << aSrvAddr->Dump()
              << " failed: rc/errno=" << rc << "/" << errno
              << " Retrying after "
              << retries * 100 << "ms.";

          usleep( retries * 100000 );
          rc = 1;
        }
      }

      // return the socket to blocking after completion of the connect
      current_setting = fcntl( _Socket, F_GETFL, NULL );
      current_setting &= (~O_NONBLOCK);
      fcntl( _Socket, F_SETFL, current_setting );
    }
    else
    {
      throw csm::network::Exception( "Connect of non-client socket" );
    }

    if( rc == 0 )
    {
      struct sockaddr_in local;
      socklen_t locallen = sizeof( struct sockaddr_in );
      if( getsockname( _Socket, (struct sockaddr *)&local, &locallen ) == 0 )
      {
        std::shared_ptr<AddressClass> locAddr = std::make_shared<AddressClass>( local );
        SetLocalAddr( locAddr );
      }
    }
    else
      LOG(csmnet, warning ) << "Connection to :" << aSrvAddr->Dump() << " failed: rc/errno=" << rc << "/" << errno;

    SetRemoteAddr( aSrvAddr );
    return rc;
  }

  int
  ConnectPost()
  {
    int rc = 0;

    // send version message to the connected destination
    csm::network::Message Msg;

    Msg.Init(
      CSM_CMD_STATUS,
      CSM_HEADER_INT_BIT,
      CSM_PRIORITY_WITH_ACK,
      0, 0x1, 0x1,
      geteuid(),
      getegid(),
      VersionMsg::ConvertToBytes( VersionMsg::Get() ) );

    LOG( csmnet, debug ) << "ConnectPost: Sending version message to peer: " << _RemoteAddr->Dump() << ": " << Msg;
    if( Send( Msg ) <= 0 )
    {
      throw csm::network::ExceptionFatal("Failed to send version message. Can't continue.");
    }
    SetVerified();
    _Heartbeat.setAddr( GetRemoteAddr() ); // set the heartbeat address for better logging
    return rc;
  }

  template<typename AddressClass, typename EndpointClass>
  csm::network::Endpoint*
  GenericPTPAccept( )
  {
    if(( _Socket ) && ( !IsServerEndpoint() ))
      throw csm::network::Exception( "Accept on non-server socket" );

    sockaddr_in CltAddr;
    unsigned int addrsize = sizeof( sockaddr_in );
    bzero( &CltAddr, addrsize );
    int newsock = accept( _Socket, (sockaddr*)&CltAddr, &addrsize );

    EndpointClass *ret = nullptr;
    if( newsock > 0 )
    {
      std::shared_ptr<AddressClass> raddr = std::make_shared<AddressClass>( htonl( CltAddr.sin_addr.s_addr ),
                                                                            htons( CltAddr.sin_port ) );
      uint32_t interval = std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP_base>(GetOptions())->getHeartbeatInterval();
      LOG(csmnet, debug) << "GenericAccept(): new client: " << (raddr->Dump());
      std::shared_ptr<csm::network::EndpointOptionsPTP<AddressClass>> opts =
          std::make_shared<csm::network::EndpointOptionsPTP<AddressClass>>( false, false, interval );

      ret = new EndpointClass( newsock, _LocalAddr, opts );
      ret->SetRemoteAddr( raddr );
      ret->SetHeartbeatAddress( raddr );
    }
    else
    {
      if(( errno == EAGAIN ) || ( errno == EWOULDBLOCK ))
        return nullptr;
      else
        throw csm::network::Exception( "Accept error" );
    }

    return ret;
  }

  ssize_t RecvVerify( csm::network::Message &aMsg );
};

}  // namespace network
} // namespace csm

#include "endpoint_ptp_plain.h"
#include "endpoint_ptp_sec.h"

#endif /* CSMNET_SRC_CPP_ENDPOINT_PTP_H_ */
