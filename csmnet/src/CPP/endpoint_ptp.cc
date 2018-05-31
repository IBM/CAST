/*================================================================================

    csmnet/src/CPP/endpoint_ptp.cc

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
csm::network::EndpointPTP_base::EndpointPTP_base( const csm::network::Address_sptr aLocalAddr,
                                             const csm::network::EndpointOptions_sptr aOptions )
: csm::network::Endpoint( aLocalAddr, aOptions ),
  _RecvBufferState(),
  _Heartbeat()
{
  _Socket = socket(AF_INET, SOCK_STREAM, 0);

  if( _Socket < 0 )
    throw csm::network::Exception("Socket creation");

  SetGeneralSockopts();

  csm::network::EndpointOptionsPTP_base_sptr ptpOptions =
      std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP_base>( _Options );
  uint32_t interval = ptpOptions->getHeartbeatInterval();
  _Heartbeat.setInterval( interval );
}

// Constructing/Initializing a new endpoint for an existing socket
csm::network::EndpointPTP_base::EndpointPTP_base( const int aSocket,
                                             const Address_sptr aLocalAddr,
                                             const EndpointOptions_sptr aOptions )
: csm::network::Endpoint( aLocalAddr, aOptions ),
  _RecvBufferState(),
  _Heartbeat()
{
  _Socket = aSocket;
  SetGeneralSockopts();

  csm::network::EndpointOptionsPTP_base_sptr ptpOptions =
      std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP_base>( _Options );
  uint32_t interval = ptpOptions->getHeartbeatInterval();
  _Heartbeat.setInterval( interval );
}

csm::network::EndpointPTP_base::EndpointPTP_base( const Endpoint *aEP )
: csm::network::Endpoint( aEP ),
  _Socket( dynamic_cast<const EndpointPTP_base*>(aEP)->_Socket ),
  _RecvBufferState( dynamic_cast<const EndpointPTP_base*>(aEP)->_RecvBufferState ),
  _Heartbeat( dynamic_cast<const EndpointPTP_base*>(aEP)->_Heartbeat )
{
  csm::network::EndpointOptionsPTP_base_sptr ptpOptions =
      std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP_base>( _Options );
  uint32_t interval = ptpOptions->getHeartbeatInterval();
  _Heartbeat.setInterval( interval );
}

csm::network::EndpointPTP_base::~EndpointPTP_base( )
{
  if( _Socket >= 0 )
    close( _Socket );

  std::string local("empty");
  std::string remote("empty");
  if( _LocalAddr != nullptr ) local = _LocalAddr->Dump();
  if( _RemoteAddr != nullptr ) remote = _RemoteAddr->Dump();
  if( local == remote )
  {
    LOG(csmnet, debug) << "~EndpointPTP(): Deleting endpoint: " << local;
  }
  else
  {
    LOG(csmnet, debug) << "~EndpointPTP(): Deleting connection : " << local << "<->" << remote;
  }
}

int csm::network::EndpointPTP_base::SetGeneralSockopts()
{
  int value = 1;
  if( setsockopt( _Socket, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof( value ) ) )
    throw csm::network::Exception( "Socket settings: SO_KEEPALIVE" );

  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  if( setsockopt( _Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof( timeout )) )
    throw csm::network::Exception("Socket settings: SO_RCVTIMEO");
  if( setsockopt( _Socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof( timeout )) )
    throw csm::network::Exception("Socket settings: SO_SNDTIMEO");

  value = 15;
  if( setsockopt( _Socket, IPPROTO_TCP, TCP_KEEPIDLE, &value, sizeof( value ) ) )
    throw csm::network::Exception( "Socket settings: TCP_KEEPIDLE" );

  value = 2;
  if( setsockopt( _Socket, IPPROTO_TCP, TCP_KEEPCNT, &value, sizeof( value ) ) )
    throw csm::network::Exception( "Socket settings: TCP_KEEPCNT" );

  value = 15;
  if( setsockopt( _Socket, IPPROTO_TCP, TCP_KEEPINTVL, &value, sizeof( value ) ) )
    throw csm::network::Exception( "Socket settings: TCP_KEEPINTVL" );

  value = 1;
  if( setsockopt( _Socket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof (value)) )
    throw csm::network::Exception("Socket settings: TCP_NODELAY");

  return 0;
}

csm::network::NetworkCtrlInfo* csm::network::EndpointPTP_base::Sync( const csm::network::SyncAction aSync )
{
  // nothing to do yet for listening endpoints
  if( IsServerEndpoint() )
    return nullptr;

  if( aSync != SYNC_ACTION_MAINTENANCE )
    return nullptr;

  csm::network::NetworkCtrlInfo *ctrlInfo = nullptr;
  EndpointHeartbeat::TimeType ref = std::chrono::system_clock::now();
  if( _Heartbeat.peerDead( ref ) )
  {
    throw csm::network::ExceptionEndpointDown("Heartbeat: Peer not responding.", ENETDOWN );
  }

  if( _Heartbeat.dueSend( ref ) )
  {
    try
    {
      Send( _Heartbeat.getMsg() );
    }
    catch ( csm::network::Exception &e )
    {
      LOG( csmnet, debug ) << "Heartbeat send failed: endpoint down." << GetRemoteAddr()->Dump();
      throw;
    }
    _Heartbeat.updateSendSuccess();
  }
  return ctrlInfo;
}

ssize_t csm::network::EndpointPTP_base::RecvVerify( csm::network::Message &aMsg )
{
  if(( ! aMsg.Validate() ) && ( aMsg.GetCommandType() >= CSM_CMD_MAX ))
    throw csm::network::ExceptionRecv( "Invalid Header/Checksum", EBADMSG );

  ssize_t rlen = aMsg.GetDataLen() + sizeof( csm_network_header_t );

  // for all unverified connections, turn every msg into the version cmd for verification
  if( ! IsVerified() )
  {
    if(( aMsg.GetCommandType() != CSM_CMD_STATUS ) || ( ! aMsg.GetInt() ))
    {
      LOG( csmnet, info ) << "Connection not yet verified. First expected msg is Version check."
          << " Changing cmd: " << cmd_to_string( aMsg.GetCommandType() ) << " to CSM_CMD_STATUS.";
      aMsg.SetReservedID( aMsg.GetCommandType() ); // store the original cmd in the reserved field
      aMsg.SetCommandType( CSM_CMD_STATUS );
      aMsg.SetInt();
      aMsg.CheckSumUpdate();
    }
  }
  else // if verified connection:
  {
    if( ! csmi_cmd_is_valid( aMsg.GetCommandType() ))
      throw csm::network::ExceptionProtocol( "Invalid/Unrecognized API call. Version mismatch?");

    if( aMsg.GetCommandType() == CSM_CMD_HEARTBEAT )
    {
      if( _Heartbeat.getInterval() != aMsg.GetReservedID() )
      {
        uint32_t newInt = std::min( _Heartbeat.getInterval(), aMsg.GetReservedID() );
        if( newInt != _Heartbeat.getInterval() )
        {
          _Heartbeat.setInterval( newInt );
          CSMLOGp( csmnet, info, "HEARTBEAT" ) << GetRemoteAddr()->Dump() << " Updating interval to sync with peer. New interval=" << newInt;
        }
      }
      rlen = 0;  // empty this message - upper layers should not see it
    }
  }

  // we have received a message, therefore updating the interval here
  // but only if the message requested an ACK (otherwise we would prevent msgs to get sent to the peer
  if(( aMsg.GetPriority() >= CSM_PRIORITY_WITH_ACK) || ( aMsg.GetAck() ) || ( aMsg.GetCommandType() == CSM_CMD_HEARTBEAT ))
    _Heartbeat.updateRecvSuccess();

  return rlen;
}

ssize_t
csm::network::EndpointPTP_base::SendMsgWrapper( const struct msghdr *aMsg, const int aFlags )
{
  ssize_t rc = sendmsg( _Socket, aMsg, aFlags );
  if( rc < 0 )
  {
    rc = errno;
    switch( rc )
    {
      case ENOTCONN:
        throw csm::network::ExceptionEndpointDown("Send: Socket not connected.");
        break;
      default:
        LOG( csmnet, debug ) << "PTP::SendMsg: Send error: " << std::to_string(rc);
        throw csm::network::ExceptionSend("sendmsg errno=" + std::to_string(rc), rc );
    }
  }

  LOG(csmnet, debug) << "PTP::SendMsg: "
      << " total_len=" << aMsg->msg_iov[0].iov_len + aMsg->msg_iov[1].iov_len
      << " rc=" << rc << " errno=" << errno;

  return rc;
}

ssize_t csm::network::EndpointPTP_base::Send( const csm::network::Message &aMsg )
{
  struct iovec iov[2];
  iov[0].iov_base = aMsg.GetHeaderBuffer();
  iov[0].iov_len = sizeof( csm_network_header_t );
  iov[1].iov_base = (void*)aMsg.GetDataPtr();
  iov[1].iov_len = aMsg.GetDataLen();

  char Remote[ sizeof( struct sockaddr_un ) ];
  bzero( Remote, sizeof( struct sockaddr_un ) );

  struct msghdr msg;
  bzero( &msg, sizeof( struct msghdr ) );
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;
  msg.msg_name = nullptr;
  msg.msg_namelen = 0;

  return SendMsgWrapper( &msg, MSG_WAITALL | MSG_NOSIGNAL );
}

// message based receive
ssize_t
csm::network::EndpointPTP_base::Recv( csm::network::Message &aMsg )
{
  ssize_t rlen = 0;
  int rc = 0;

  rlen = _RecvBufferState.Recv( aMsg );
  if( rlen > 0 )
  {
    LOG(csmnet, debug) << " RecvMsg from Buffer: "
        << " rlen=" << rlen
        << " CSMData=" << aMsg;
    rc = rlen;
  }
  else
  {
    struct iovec iov[1];
    iov[0].iov_base = _RecvBufferState.GetRecvBufferPtr();
    iov[0].iov_len = _RecvBufferState.GetRecvSpace();

    char Remote[ sizeof( struct sockaddr_un ) ];
    bzero( Remote, sizeof( struct sockaddr_un ) );

    struct msghdr msg;
    bzero( &msg, sizeof( struct msghdr ) );
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = Remote;
    msg.msg_namelen = sizeof( struct sockaddr_un );

  //  LOG(csmnet,info) << " Calling recvmsg...";
    rlen = recvmsg( _Socket, &msg, MSG_DONTWAIT );

    if( rlen <= 0 )
    {
      if(( errno == EAGAIN ) || ( errno == EWOULDBLOCK ))
        return 0;
      else
        throw csm::network::ExceptionEndpointDown( "Receive Error" );
    }

    if( msg.msg_flags & MSG_TRUNC )
    {
      // todo: LS: need to repeat the recv call to cover split messages
      LOG(csmnet, warning) << "data transmission truncated";
      throw csm::network::ExceptionRecv("Received Message Truncated. Incomplete protocol implementation...");
    }
    if( msg.msg_flags & MSG_ERRQUEUE )
    {
      LOG(csmnet, error) << "Received error/pending MSG_ERRQUEUE";
    }

    _RecvBufferState.Update( rlen );
    rc = _RecvBufferState.Recv( aMsg );
    if( rc == 0 )
      return 0;

    LOG(csmnet, debug) << " RecvMsg: "
        << " rcvd=" << rlen
        << " msgrc=" << rc << " errno=" << errno << " flags=" << msg.msg_flags << " buf_empty=" << _RecvBufferState.IsEmpty()
        << " CSMData=" << aMsg;
  }

  rc = RecvVerify( aMsg );
  return rc;
}

ssize_t
csm::network::EndpointPTP_base::RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
{
  int rc = 0;
  try
  {
    rc = Recv( aMsgAddr._Msg );
    if( rc > 0 )
      aMsgAddr.SetAddr( _RemoteAddr );
  }
  catch ( csm::network::Exception &e )
  {
    // set address on error to allow better error handling
    aMsgAddr.SetAddr( _RemoteAddr );
    throw;
  }
  return rc;
}
