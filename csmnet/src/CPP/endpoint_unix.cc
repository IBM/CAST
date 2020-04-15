/*================================================================================

    csmnet/src/CPP/endpoint_unix.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

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
#include <sys/stat.h>    // stat()
#include <grp.h>

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un

#include <errno.h>

#include "logging.h"
#include "csmutil/include/csm_version.h"

#include "csm_network_config.h"
#include "csm_network_exception.h"
#include "csm_network_msg_cpp.h"
#include "endpoint_unix.h"

/**
 *  constructor allows to pass in a local address to bind.
 *  endpoint creates a copy to use, so caller can destroy instance of address anytime
 */
csm::network::EndpointUnix::EndpointUnix( const Address_sptr aLocalAddr,
                                          const EndpointOptions_sptr aOptions )
: csm::network::Endpoint( aLocalAddr, aOptions ),
  _RecvBufferState(),
  _UnixOptions( std::dynamic_pointer_cast<csm::network::EndpointOptionsUnix>( aOptions ) )
{
  if( _UnixOptions == nullptr )
    throw csm::network::Exception("UnixOptions are NULL. Can't continue.", EINVAL );

  LOG(csmnet,debug) << "Creating endpoint isServer=" << IsServer()
      << " Perm=" << std::oct << _UnixOptions->_Permissions << std::resetiosflags(std::ios::oct)
      << " Grp=" << _UnixOptions->_Group;

  if(( aLocalAddr ) && ( aLocalAddr->GetAddrType() != CSM_NETWORK_TYPE_LOCAL ))
    throw csm::network::Exception("Attempt to initialize Unix endpoint with wrong address type.");

  if( aLocalAddr == nullptr )
  {
    std::string addrPath;
    if( IsServer() )
      addrPath = std::string( CSM_NETWORK_LOCAL_SSOCKET );
    else
      addrPath = std::string( CSM_NETWORK_LOCAL_CSOCKET ) + std::to_string( getpid() );

    SetLocalAddr( std::make_shared<csm::network::AddressUnix>( addrPath.c_str() ) );
  }

  const csm::network::AddressUnix *addr = dynamic_cast<const csm::network::AddressUnix*>( _LocalAddr.get() );
  if( addr == nullptr )
    throw csm::network::ExceptionEndpointDown("Wrong address type",EBADF );

  // check if socket file exists
  int rc = unlink( addr->_SockAddr.sun_path );
  bool critical = false;
  int stored_errno = errno;
  if( rc == 0 )
  {
    LOG( csmnet, warning ) << "Detected an existing local socket: " << addr->_SockAddr.sun_path
        << " either the previous run didn't clean up or there's another CSM daemon configured to use the same socket.";
    LOG( csmnet, warning ) << "REPLACING " << addr->_SockAddr.sun_path
        << ". IF IT BELONGED TO ANOTHER RUNNING CSMD, THAT OTHER CSMD IS NOW BROKEN!!!";
  }
  if( rc != 0 )
    switch( stored_errno )
    {
      case EPERM:
      case EACCES:
        LOG( csmnet, critical ) << "No permission to replace existing " << addr->_SockAddr.sun_path;
        critical=true;
        break;
      case EIO:
        LOG( csmnet, critical ) << "I/O error when trying to access " << addr->_SockAddr.sun_path;
        critical=true;
        break;
      case EBUSY:
        LOG( csmnet, critical ) << "Another process is using " << addr->_SockAddr.sun_path;
        critical=true;
        break;
      case EROFS:
        LOG( csmnet, critical ) << "Socket configured to be on a RO file system: " << addr->_SockAddr.sun_path;
        critical=true;
        break;
      case EFAULT:
      case ENAMETOOLONG:
      case ENOMEM:
      case ENOTDIR:
      case ELOOP:
      case EISDIR:
        critical=true;
        // no break on purpose
      case ENOENT:
      default:
        break; // no warning or anything needed, the socket might not have been there which is fine
    }

  if( critical )
    throw csm::network::ExceptionFatal("Replacing/creating local socket", stored_errno );


  _CtrlBuf = new char[ 2 * CSM_UNIX_CREDENTIAL_LENGTH ];
  memset( _CtrlBuf, 0, 2 * CSM_UNIX_CREDENTIAL_LENGTH );

  // create socket
  _Socket = socket(AF_UNIX, SOCK_DGRAM, 0);
  if( _Socket < 0 )
    throw csm::network::Exception("Socket creation");

  int value = 1;
  if (setsockopt(_Socket, SOL_SOCKET, SO_PASSCRED, &value, sizeof (value)) < 0)
  {
    LOG(csmnet, warning)<< "setting SO_PASSCRED failed, errno=" << errno << ", "<< strerror(errno);
    throw csm::network::Exception("Socket options");
  }
  unsigned int valueLen = sizeof( value );
  if(( getsockopt( _Socket, SOL_SOCKET, SO_PASSCRED, &value, &valueLen ) < 0) && (value != 1))
  {
    LOG(csmnet, warning) << "SO_PASSCRED not set even after call to setsockopt errno=" << errno << ", " << strerror(errno);
    throw csm::network::Exception("Socket options", EINVAL);
  }

  value = 0;
  socklen_t vallen = sizeof( value );
  if (getsockopt(_Socket, SOL_SOCKET, SO_SNDBUF, &value, &vallen) < 0)
  {
    LOG(csmnet, warning)<< "Getting SO_SNDBUF failed, errno=" << errno << ", "<< strerror(errno);
    throw csm::network::Exception("Socket options");
  }

  _MaxPayloadLen = value - CSM_UNIX_CREDENTIAL_LENGTH - sizeof( csm_network_header_t );
  LOG( csmnet, debug ) << "Largest single-msg transfer: " << _MaxPayloadLen;

  struct timeval tval;
  tval.tv_sec = CSM_RECV_TIMEOUT_GRANULARITY;
  tval.tv_usec = 0;
  if (setsockopt(_Socket, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof (struct timeval)) < 0)
  {
    LOG(csmnet, warning)<< "setting SO_RCVTIMEO failed, errno=" << errno << ", "<< strerror(errno);
    throw csm::network::Exception("Socket options");
  }
  if (setsockopt(_Socket, SOL_SOCKET, SO_SNDTIMEO, &tval, sizeof (struct timeval)) < 0)
  {
    LOG(csmnet, warning)<< "setting SO_SNDTIMEO failed, errno=" << errno << ", "<< strerror(errno);
    throw csm::network::Exception("Socket options");
  }



  // it's safe to unlink for later cleanup: according to man-page:
  // "socket can be unlinked at any time and will be finally removed from the file system when the last reference to it is closed."
  csm::network::AddressUnix *uep = dynamic_cast<csm::network::AddressUnix*>( GetLocalAddr().get() );
  if( uep == nullptr )
    throw csm::network::Exception( "Local address is null", EBADFD );
  unlink( uep->_SockAddr.sun_path );
  // the result doesn't matter here

  // bind
  LOG(csmnet,debug) << "binding " << _Socket << " to " << addr->_SockAddr.sun_path;
  LOG(csmnet,debug) << "Created endpoint server=" << IsServer() << " binding...";
  rc = bind( _Socket,
             (sockaddr*)&( addr->_SockAddr ),
             sizeof( addr->_SockAddr ) );
  if( rc )
    throw csm::network::Exception("Binding");

  // check if socket file exists
  struct stat statbuf;
  rc = stat( addr->_SockAddr.sun_path, &statbuf );
  if( rc )
    throw csm::network::Exception("Access");

  // make sure server socket is accessible as configured
  if( chmod(addr->_SockAddr.sun_path, _UnixOptions->_Permissions ) != 0 )
    LOG( csmnet, warning ) << "Failed to change client socket permissions. Unintended consequences for client access might occur. chmod returned: " << strerror( errno );
    // todo: should this cause the creation to fail because it might have security implications?

  struct group *grp = getgrnam( _UnixOptions->_Group.c_str() );
  if( grp == nullptr )
  {
    LOG( csmnet, error ) << "Failed to get groupID information for socket. errno=" << errno;
    throw csm::network::Exception("Permissions");
  }

  if( chown(addr->_SockAddr.sun_path, geteuid(), grp->gr_gid ) )
  {
    LOG( csmnet, warning ) << "Unable to set groupID to " << _UnixOptions->_Group
        << " for " << addr->_SockAddr.sun_path
        << ". errno=" << errno << ":" << strerror(errno);;
  }

  if( _UnixOptions->_AuthList == nullptr )
  {
    LOG(csmnet, warning) << "UnixEndpoint creation without AuthList.";
    _UnixOptions->_AuthList = std::make_shared<csm::daemon::CSMIAuthList>("nofile");
    if( _UnixOptions->_AuthList == nullptr )
      throw csm::network::Exception("Failed to initialize API permissions." );
  }
  LOG(csmnet,debug) << "Created endpoint " << _UnixOptions->_IsServer << " complete";

  LOG(csmnet,debug) << "Timeoutserialization: " << _UnixOptions->_APIConfig->GetSerializedTimeouts();

  if(( ! IsServer()  ) && ( ! _UnixOptions->_ServerPath.empty() ))
  {
    csm::network::AddressUnix_sptr srvAddr = std::make_shared<csm::network::AddressUnix>( _UnixOptions->_ServerPath.c_str() );
    Connect( srvAddr );
  }
  else
    _Connected = false;
}

csm::network::EndpointUnix::EndpointUnix( const std::string &aPath,
                                          const csm::network::EndpointOptions_sptr aOptions )
: csm::network::EndpointUnix( std::make_shared<csm::network::AddressUnix>( aPath.c_str() ), aOptions )
{}

csm::network::EndpointUnix::EndpointUnix( const Endpoint *aEP )
: csm::network::Endpoint( aEP ),
  _Socket( ( dynamic_cast<const EndpointUnix*>(aEP) == nullptr ) ? -1 : dynamic_cast<const EndpointUnix*>(aEP)->_Socket ),
  _CtrlBuf( nullptr ),
  _RecvBufferState( ( dynamic_cast<const EndpointUnix*>(aEP) == nullptr ) ? csm::network::EndpointStateUnix() : dynamic_cast<const EndpointUnix*>(aEP)->_RecvBufferState ),
  _UnixOptions( std::dynamic_pointer_cast<csm::network::EndpointOptionsUnix>( (aEP == nullptr ) ? nullptr : aEP->GetOptions() )),
  _MaxPayloadLen( ( dynamic_cast<const EndpointUnix*>(aEP) == nullptr ) ? 0 : dynamic_cast<const EndpointUnix*>(aEP)->_MaxPayloadLen )
{
  if( dynamic_cast<const EndpointUnix*>(aEP) == nullptr )
    throw csm::network::Exception("Wrong endpoint type", EBADF );

  _CtrlBuf = new char[ 2 * CSM_UNIX_CREDENTIAL_LENGTH ];
  const csm::network::EndpointUnix *uep = dynamic_cast<const csm::network::EndpointUnix*>( aEP );
  if( uep == nullptr )
    throw csm::network::ExceptionProtocol("Attempted copy from incompatible type", EBADFD );

  memcpy( _CtrlBuf,
          uep->_CtrlBuf,
          2 * CSM_UNIX_CREDENTIAL_LENGTH );
}


/* destruction of network interface
 * e.g. close socket and remove socket file names
 */
csm::network::EndpointUnix::~EndpointUnix()
{
  if( _Socket >= 0 )
    close( _Socket );

  if( _CtrlBuf )
    delete [] _CtrlBuf;

  csm::network::AddressUnix_sptr addr = std::dynamic_pointer_cast<csm::network::AddressUnix>( GetLocalAddr() );
  if( addr == nullptr )
  {
    LOG(csmnet, critical ) << "EndpointUnix with wrong address type in destructor";
  }
  else
  {
    if( addr->_SockAddr.sun_path[0] != 0 )
      unlink( addr->_SockAddr.sun_path );
  }
}

int csm::network::EndpointUnix::Connect( const csm::network::Address_sptr aSrvAddr )
{
  int rc = 0;
  if( aSrvAddr == nullptr )
    throw csm::network::Exception("EndpointUnix::Connect(): destination address == nullptr.");

  if( _RemoteAddr != nullptr )
  {
    const csm::network::AddressUnix *remote = dynamic_cast<const csm::network::AddressUnix*>( _RemoteAddr.get() );
    if( remote == nullptr )
      throw csm::network::Exception("Unrecognized or empty remote address", EBADF );

    const csm::network::AddressUnix *server = dynamic_cast<const csm::network::AddressUnix*>( aSrvAddr.get() );
    if( server == nullptr )
      throw csm::network::Exception("Unrecognized or empty server/destination address", EBADF );

    if( *remote == *server )
    {
      LOG(csmnet, debug) << "Endpoint already connected to destination: " << aSrvAddr->Dump()
          << ". skipping without error."<< std::endl;
      return 0;
    }
  }

  const csm::network::AddressUnix* AddrUnix = dynamic_cast<const csm::network::AddressUnix*>( aSrvAddr.get() );
  if( AddrUnix == nullptr )
    throw csm::network::Exception("Invalid server address provided to Connect", EBADFD );

  sockaddr_un* SrvAddr = (sockaddr_un*) ((&(AddrUnix->_SockAddr)));
  if( SrvAddr == nullptr )
    throw csm::network::Exception("Invalid server address provided to Connect", EBADFD );

  LOG(csmnet, debug) << "connecting " << _Socket << " to " << SrvAddr->sun_path;
  if( _Socket && !IsServer() )
  {
    rc = 1;
    for( int retries = 1; ((rc > 0) && (retries < 10)); ++retries )
    {
      rc = connect( _Socket, (struct sockaddr *) SrvAddr, sizeof(sockaddr_un) );
      if( rc )
      {
        LOG(csmnet, warning ) << "Connection attempt " << retries << " failed: rc/errno=" << rc << "/" << errno
        << " Retrying after "
        << retries * 500 << "ms.";
        usleep( retries * 500000 );
        rc = 1;
      }
      else
        _Connected = true;
    }
  }
  if( rc )
    throw csm::network::Exception( "Connection error (non-client socket?)" );

  SetRemoteAddr( aSrvAddr );

  // skip any connection message if this we're "connecting" to ourselves...
  csm::network::AddressUnix_sptr local_addr = std::dynamic_pointer_cast<csm::network::AddressUnix>( GetLocalAddr() );
  const csm::network::AddressUnix *server = dynamic_cast<const csm::network::AddressUnix*>( aSrvAddr.get() );
  if(( local_addr == nullptr ) || ( server == nullptr ))
    throw csm::network::Exception("Wrong address type", EBADF );
  if( *server == *local_addr )
    return rc;

  csm::network::Message ConnectMsg;
  ConnectMsg.Init( CSM_CMD_STATUS,
                   CSM_HEADER_INT_BIT,
                   CSM_PRIORITY_NO_ACK,
                   1, 0x1234, 0x1234,
                   geteuid(), getegid(),
                   std::string( CSM_VERSION ));
  Send( ConnectMsg );
  try
  {
    csm::network::MessageAndAddress AcceptMsgAddr;
    ssize_t rlen = 0;
    while( ( rlen = RecvFrom( AcceptMsgAddr )) == 0 )
    {
      // repeat until we got a ACK or HEARTBEAT msg with address
      if( AcceptMsgAddr.GetAddr() == nullptr )
        continue;

      if(( std::dynamic_pointer_cast<csm::network::AddressUnix>(AcceptMsgAddr.GetAddr()) == nullptr ) ||
          (std::dynamic_pointer_cast<csm::network::AddressUnix>(GetRemoteAddr()) == nullptr ))
      {
        AcceptMsgAddr.SetAddr( nullptr );
        throw csm::network::ExceptionEndpointDown("Wrong address types while waiting for ACCEPT message from peer", EBADF );
      }
      if( *std::dynamic_pointer_cast<csm::network::AddressUnix>(AcceptMsgAddr.GetAddr()) !=
             *std::dynamic_pointer_cast<csm::network::AddressUnix>(GetRemoteAddr()) )
      {
        AcceptMsgAddr.SetAddr( nullptr );
        throw csm::network::ExceptionEndpointDown("Expected ACCEPT Message from peer.", EPROTO );
      }
      else
      {
        AcceptMsgAddr.SetAddr( nullptr );
        break;
      }
    }
    if( AcceptMsgAddr.GetAddr() == nullptr )
      throw csm::network::ExceptionEndpointDown("Accept message with empty address", ENOENT );
    LOG( csmnet, debug ) << "Received Heartbeat Acceptance Msg from: " << AcceptMsgAddr.GetAddr()->Dump();
  }
  catch ( csm::network::ExceptionRecv &e )
  {
    LOG( csmnet, error ) << "Connection failure: " << e.what();
    rc = -1;
  }

  return rc;
}

ssize_t
csm::network::EndpointUnix::SendMsgWrapper( struct msghdr *aMsg, const int aFlags )
{
  if( aMsg == nullptr )
    throw csm::network::ExceptionSend("Nullptr Message", EINVAL );
  aMsg->msg_control = _CtrlBuf;
  aMsg->msg_controllen = CSM_UNIX_CREDENTIAL_LENGTH;

  struct cmsghdr *cmsg = CMSG_FIRSTHDR( aMsg );
  if( !cmsg )
    throw csm::network::ExceptionSend("FATAL: Network message setup problem. FIRSTHDR = nullptr.");
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_CREDENTIALS;
  cmsg->cmsg_len = CMSG_LEN( sizeof( struct ucred ) );

  struct ucred credentials;
  credentials.gid = getegid();
  credentials.uid = geteuid();
  credentials.pid = getpid();

  struct ucred *cptr = (struct ucred*)CMSG_DATA( cmsg );
  memcpy( cptr, &credentials, sizeof( struct ucred ) );
  aMsg->msg_controllen = CSM_UNIX_CREDENTIAL_LENGTH;

  LOG( csmnet, debug ) << "Sending Credentials: ( uid:" << credentials.uid
      << " gid:" << credentials.gid
      << " pid:" << credentials.pid << " )"
      << " cmsg: " << cmsg->cmsg_level << ":" << cmsg->cmsg_type << ":" << cmsg->cmsg_len;
  errno=0;

  size_t totallen = aMsg->msg_iov[1].iov_len;
  char *next_ptr = (char*)aMsg->msg_iov[1].iov_base;
  if( aMsg->msg_iov[1].iov_len > _MaxPayloadLen )
  {
    aMsg->msg_iov[1].iov_len = _MaxPayloadLen;
    next_ptr += aMsg->msg_iov[1].iov_len;
  }

  ssize_t rc = sendmsg( _Socket, aMsg, aFlags );
  if( rc < 0 )
  {
    rc = errno;
    switch( rc )
    {
      case ECONNRESET:
      case ENOTCONN:
      case EDESTADDRREQ:
      case EBADF:
      case EACCES: // todo: LS: this will likely need an extra exception path later
        throw csm::network::ExceptionEndpointDown("Unix::Endpoint error.");
        break;
      default:
        throw csm::network::ExceptionSend("Unix::sendmsg rc=" + std::to_string(rc) );
    }
  }

  ssize_t remaining = totallen - aMsg->msg_iov[1].iov_len;
  while(( remaining > 0 ) && ( rc > 0 ))
  {
    struct msghdr part;
    struct iovec iov;
    part.msg_control = _CtrlBuf;
    part.msg_controllen = CSM_UNIX_CREDENTIAL_LENGTH;
    part.msg_flags = aMsg->msg_flags;
    part.msg_name = aMsg->msg_name;
    part.msg_namelen = aMsg->msg_namelen;
    part.msg_iovlen = 1;
    iov.iov_base = next_ptr;
    iov.iov_len = std::min( (size_t)remaining, _MaxPayloadLen );
    part.msg_iov = &iov;

    LOG(csmnet, debug) << "Unix::SendMsg: Partial "
        << " total_data=" << totallen
        << " this part=" << part.msg_iov[0].iov_len
        << " remaining=" << remaining - part.msg_iov[0].iov_len;

    ssize_t tmp_rc = sendmsg( _Socket, &part, aFlags );
    if( tmp_rc < 0 )
    {
      rc = errno;
      switch( rc )
      {
        case ECONNRESET:
        case ENOTCONN:
        case EDESTADDRREQ:
        case EBADF:
        case EACCES: // todo: LS: this will likely need an extra exception path later
          throw csm::network::ExceptionEndpointDown("Unix::Endpoint error.", rc);
          break;
        default:
          throw csm::network::ExceptionSend("Unix::sendmsg part rc=" + std::to_string(rc), rc );
      }
    }
    remaining -= tmp_rc;
    next_ptr += tmp_rc;
  }

  LOG(csmnet, debug) << "Unix::SendMsg: "
      << " total_len=" << aMsg->msg_iov[0].iov_len + totallen
      << " rc=" << rc << " errno=" << errno
      << " msg: " << *(csm::network::Message*)(aMsg->msg_iov[0].iov_base);

  return rc;
}

ssize_t
csm::network::EndpointUnix::SendTo( const csm::network::Message &aMsg,
                                    const csm::network::Address_sptr aRemoteAddr )
{
  if( aRemoteAddr == nullptr )
    throw csm::network::ExceptionSend( "SendTo destination is nullptr." );

  struct iovec iov[2];
  iov[0].iov_base = aMsg.GetHeaderBuffer();
  iov[0].iov_len = sizeof( csm_network_header_t );
  iov[1].iov_base = (void*)aMsg.GetDataPtr();
  iov[1].iov_len = aMsg.GetDataLen();

  if(( iov[0].iov_base == nullptr ) || ( iov[1].iov_base == nullptr ))
    throw csm::network::ExceptionSend("Nullptr data buffers detected", EINVAL );

  char Remote[ sizeof( struct sockaddr_un ) ];
  bzero( Remote, sizeof( struct sockaddr_un ) );

  struct msghdr msg;
  bzero( &msg, sizeof( struct msghdr ) );
  msg.msg_iov = iov;
  msg.msg_iovlen = aMsg.GetDataLen() > 0 ? 2 : 1;
  msg.msg_name = (void*)&( std::dynamic_pointer_cast<const csm::network::AddressUnix>( aRemoteAddr )->_SockAddr );
  msg.msg_namelen = sizeof( sockaddr_un );

  return SendMsgWrapper( &msg, MSG_WAITALL );
}

ssize_t
csm::network::EndpointUnix::Send( const csm::network::Message &aMsg )
{
  if( IsServer() )
  {
    // server in connection-less mode cannot just do send without remote address
    errno = ENOTCONN;
    throw csm::network::ExceptionSend("Send call via unconnected server socket without destination address");
  }

  struct iovec iov[2];
  iov[0].iov_base = aMsg.GetHeaderBuffer();
  iov[0].iov_len = sizeof( csm_network_header_t );
  iov[1].iov_base = (void*)aMsg.GetDataPtr();
  iov[1].iov_len = aMsg.GetDataLen();

  if(( iov[0].iov_base == nullptr ) || ( iov[1].iov_base == nullptr ))
    throw csm::network::ExceptionSend("Nullptr data buffers detected", EINVAL );

  char Remote[ sizeof( struct sockaddr_un ) ];
  bzero( Remote, sizeof( struct sockaddr_un ) );

  struct msghdr msg;
  bzero( &msg, sizeof( struct msghdr ) );
  msg.msg_iov = iov;
  msg.msg_iovlen = aMsg.GetDataLen() > 0 ? 2 : 1;
  msg.msg_name = nullptr;
  msg.msg_namelen = 0;

  return SendMsgWrapper( &msg, MSG_WAITALL );
}

ssize_t
csm::network::EndpointUnix::RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
{
  ssize_t rc = 0;
  ssize_t rlen = 0;
  struct ucred credentials;

  // if there's no buffered data, then go and check for new data
  if(( _RecvBufferState.IsEmpty() ) || ( _RecvBufferState.HasPartialMsg() ))
  {
    struct iovec iov[2];
    iov[0].iov_base = _RecvBufferState.GetRecvBufferPtr();
    iov[0].iov_len = _RecvBufferState.GetRecvSpace();

    if( iov[0].iov_base == nullptr )
      throw csm::network::ExceptionRecv("Nullptr data buffers detected", EINVAL );

    char Remote[ sizeof( struct sockaddr_un ) ];
    bzero( Remote, sizeof( struct sockaddr_un ) );

    struct msghdr msg;
    bzero( &msg, sizeof( struct msghdr ) );
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = Remote;
    msg.msg_namelen = sizeof( struct sockaddr_un );
    msg.msg_control = _CtrlBuf;
    msg.msg_controllen = CSM_UNIX_CREDENTIAL_LENGTH;
    bzero( _CtrlBuf, 2 * CSM_UNIX_CREDENTIAL_LENGTH );

    rlen = recvmsg( _Socket, &msg, MSG_DONTWAIT );

    if( rlen <= 0 )
    {
      if(( errno == EAGAIN ) || ( errno == EWOULDBLOCK ))
          return 0;
      throw csm::network::ExceptionEndpointDown( "Receive Error" );
    }

    rc = rlen;
    switch( msg.msg_flags )
    {
      case MSG_TRUNC:
        // todo: LS: need to repeat the recv call to cover split messages
        LOG(csmnet, warning) << "data transmission truncated";
        throw csm::network::ExceptionEndpointDown("Received Message Truncated. Incomplete protocol implementation...");
        break;
      case MSG_EOR:
        // msg complete
      default:
        break;
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR( &msg );
    if( cmsg == nullptr )
    {
      throw csm::network::ExceptionRecv("Credentials reception failed.", EBADMSG );
    }
    bzero( &credentials, sizeof( struct ucred ) );
    if(( cmsg->cmsg_level == SOL_SOCKET ) && ( cmsg->cmsg_type == SCM_CREDENTIALS ) )
    {
      memcpy( (void*)&credentials,
              CMSG_DATA( cmsg ),
              sizeof( struct ucred ) );
    }
    else
      throw csm::network::ExceptionRecv("Credentials reception failed.", EBADMSG );

    LOG( csmnet, debug ) << "received " << rlen << " bytes from socket with credentials: ( uid:" << credentials.uid
        << " gid:" << credentials.gid
        << " pid:" << credentials.pid << " )";

    struct sockaddr_un *addr = (struct sockaddr_un*)msg.msg_name;

    _RecvBufferState.Update( rlen, (struct sockaddr*)addr );
  }
  else
  {
    // create a tmp msg struct to extract credentials from existing control buffer
    struct msghdr msg;
    bzero( &msg, sizeof( struct msghdr ) );
    msg.msg_control = _CtrlBuf;
    msg.msg_controllen = CSM_UNIX_CREDENTIAL_LENGTH;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR( &msg );
    if( cmsg == nullptr )
    {
      throw csm::network::ExceptionRecv("Credentials reception failed.", EBADMSG );
    }
    bzero( &credentials, sizeof( struct ucred ) );
    if(( cmsg->cmsg_level == SOL_SOCKET ) && ( cmsg->cmsg_type == SCM_CREDENTIALS ) )
    {
      memcpy( (void*)&credentials,
              CMSG_DATA( cmsg ),
              sizeof( struct ucred ) );
    }
    else
      throw csm::network::ExceptionRecv("Credentials reception from buffer failed.", EBADMSG );

    LOG( csmnet, debug ) << "received " << rlen << " bytes from buffer with credentials: ( uid:" << credentials.uid
        << " gid:" << credentials.gid
        << " pid:" << credentials.pid << " )";
  }

  // consume the buffer until we have some actual message for upper layers to process
  do
  {
    rc = _RecvBufferState.Recv( aMsgAddr );
    if( rc == 0 )
      return 0;
    if( aMsgAddr.GetAddr() == nullptr )
      throw csm::network::ExceptionProtocol("BUG: empty address for received data.");

    // make sure we properly handle any incoming status msgs
    if( aMsgAddr._Msg.Validate() )
    {
      if( aMsgAddr._Msg.GetCommandType() == CSM_CMD_STATUS )
      {
        // was it a disconnect msg?
        if( ( aMsgAddr._Msg.GetData().compare( CSM_DISCONNECT_MSG ) == 0 ) )
        {
          rc = aMsgAddr._Msg.GetDataLen();
          LOG( csmnet, info ) << "Received disconnect msg from client " << aMsgAddr.GetAddr()->Dump();
        }
        else
        {
          // compare the version IDs
          if( ( aMsgAddr._Msg.GetData().compare( CSM_VERSION ) != 0 ) )
          {
            LOG( csmnet, warning ) << "Client library version mismatch! Received:" << aMsgAddr._Msg.GetData()
                << "  Expected: " CSM_VERSION;
            // todo: This causes 2 messages to get send (error response + accept msg)
            //   - which is fine if client exits after failed connection attempt
            //   - however, it needs to be fixed
            throw csm::network::ExceptionRecv("Client Protocol Version Mismatch.", EPROTO);
          }
          else
          {
            LOG( csmnet, debug ) << "Received Heartbeat/Connect with CSM_VERSION=" << aMsgAddr._Msg.GetData();
          }
        }
        return rc;
      }
    }
  } while ( rc == 0 );

  LOG(csmnet, debug) << " RecvMsg: "
      << " rcvd=" << rlen
      << " msgrc=" << rc << " errno=" << errno << " buf_empty=" << _RecvBufferState.IsEmpty()
      << " from=" << aMsgAddr.GetAddr()->Dump() << std::endl
      << " MSG=" << aMsgAddr._Msg;

  // input validation:
  if ( ! aMsgAddr._Msg.Validate() )
    throw csm::network::ExceptionRecv( "Invalid Msg/Header", EBADMSG );
  if(( aMsgAddr._Msg.GetInt() ) && ( aMsgAddr._Msg.GetCommandType() != CSM_CMD_HEARTBEAT ))
    throw csm::network::ExceptionRecv( "Internal msg from client detected.", EBADMSG );
  if( aMsgAddr._Msg.GetMulticast() )
    throw csm::network::ExceptionRecv( "Multicast msg from client detected.", EBADMSG );

  // length check validation
  if( (int)aMsgAddr._Msg.GetDataLen() + (int)sizeof( csm_network_header_t ) != rc )
    throw csm::network::ExceptionRecv( "Data Length Error", EBADMSG );

  // FIXME Should this always print? 
  if ( aMsgAddr._Msg.GetCommandType() < CSM_CMD_MAX_REGULAR  )
  {
      LOG (csmapi,info)              << cmd_to_string( aMsgAddr._Msg.GetCommandType())
          << "["                     << aMsgAddr._Msg.GetReservedID() 
          << "]; Client "
          << (aMsgAddr._Msg.GetFlags() & CSM_HEADER_RESP_BIT ? "Recv" : "Sent")
          << "; PID: "               << credentials.pid 
          << "; UID:"                << credentials.uid 
          << "; GID:"                << credentials.gid;
  }

  // skip the security check for ACKs
  if( aMsgAddr._Msg.GetAck() )
  {
    LOG( csmnet, debug ) << "Received ACK on " << GetLocalAddr()->Dump() << " for " << cmd_to_string( aMsgAddr._Msg.GetCommandType() )
        << " from: " << aMsgAddr.GetAddr()->Dump();
    return rc;
  }

  // API privilege check:
  csm::daemon::API_SEC_LEVEL SecLevel = _UnixOptions->_AuthList->GetSecurityLevel( (csmi_cmd_t)aMsgAddr._Msg.GetCommandType() );
  if( (  SecLevel != csm::daemon::PRIVILEGED ) ||
      (   ( SecLevel == csm::daemon::PRIVILEGED ) &&
          ( _UnixOptions->_AuthList->HasPrivilegedAccess( credentials.uid, credentials.gid ) ))  )
  {
    // determine whether private API user check is required for the back-end
    if( _UnixOptions->_AuthList->PrivateRequiresUserCompare( aMsgAddr._Msg.GetCommandType(),
                                                             credentials.uid, credentials.gid ) )
      aMsgAddr._Msg.SetPrivateCheck();
    else
      aMsgAddr._Msg.ClrPrivateCheck();

    LOG( csmnet, debug ) << "Permission to " << cmd_to_string( aMsgAddr._Msg.GetCommandType() )
      << " granted to user:grp " << credentials.uid << ":" << credentials.gid
      << " private API check required: " << aMsgAddr._Msg.GetPrivateCheck();

    // if permission granted: make sure the header fields match the actual credentials
    bool cred_Update = ( aMsgAddr._Msg.GetUserID() != credentials.uid ) || ( aMsgAddr._Msg.GetGroupID() != credentials.gid );
    if( cred_Update )
    {
      LOG( csmnet, warning ) << "Credentials mismatch detected!"
          << " Found: " << aMsgAddr._Msg.GetUserID() << ":" << aMsgAddr._Msg.GetGroupID()
          << " Expected: " << credentials.uid << ":" << credentials.gid;
      aMsgAddr._Msg.SetUserID( credentials.uid );
      aMsgAddr._Msg.SetGroupID( credentials.gid );
    }
    // make sure the message is valid after any changes caused by setting flags or credentials
    aMsgAddr._Msg.CheckSumUpdate();
  }
  else
  {
    LOG (csmapi,warning)           << cmd_to_string( aMsgAddr._Msg.GetCommandType())
        << "["                     << aMsgAddr._Msg.GetReservedID()
        << "]; Client "
        << "  PID: "               << credentials.pid
        << "; UID:"                << credentials.uid
        << "; GID:"                << credentials.gid
        << ": Permission Denied.";
    csm::network::ExceptionRecv ex = csm::network::ExceptionRecv( "Permission Denied", EPERM );
    throw ex;
  }

  return rc;
}

ssize_t
csm::network::EndpointUnix::Recv( csm::network::Message &aMsg )
{
  ssize_t rc = 0;
  if( IsServer() || ! IsConnected() )
    throw csm::network::ExceptionEndpointDown("Recv without remote address on unconnected endpoint.");

  csm::network::MessageAndAddress MsgAddr( aMsg, nullptr );
  rc = RecvFrom( MsgAddr );
  aMsg = MsgAddr._Msg;
  return rc;
}
csm::network::NetworkCtrlInfo *
csm::network::EndpointUnix::Sync( const csm::network::SyncAction aSync )
{
  // nothing to do right now
  return nullptr;
}
