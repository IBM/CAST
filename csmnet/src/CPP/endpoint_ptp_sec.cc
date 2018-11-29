/*================================================================================

    csmnet/src/CPP/endpoint_ptp_sec.cc

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

#include <errno.h>

#include <logging.h>
#include "csm_network_config.h"
#include "csm_network_exception.h"
#include "csm_network_msg_cpp.h"
#include "address.h"
#include "endpoint_ptp.h"

#ifdef SECURE_COMM

SSL_CTX *csm::network::EndpointPTP_sec_base::_gSSLContext = nullptr;

std::string csm::network::SSLPrintError( const int err )
{
  switch( err )
  {
    case SSL_ERROR_NONE:
      return "SSL_ERROR_NONE";
    case SSL_ERROR_ZERO_RETURN:
      return "SSL_ERROR_ZERO_RETURN";
    case SSL_ERROR_WANT_READ:
      return "SSL_ERROR_WANT_READ";
    case SSL_ERROR_WANT_WRITE:
      return "SSL_ERROR_WANT_WRITE";
    case SSL_ERROR_WANT_CONNECT:
      return "SSL_ERROR_WANT_CONNECT";
    case SSL_ERROR_WANT_ACCEPT:
      return "SSL_ERROR_WANT_ACCEPT";
    case SSL_ERROR_WANT_X509_LOOKUP:
      return "SSL_ERROR_WANT_X509_LOOKUP";
    case SSL_ERROR_SYSCALL:
      return "SSL_ERROR_SYSCALL";
    case SSL_ERROR_SSL:
      return "SSL_ERROR_SSL";
    default:
      return "UNKNOWN SSL ERROR";
  }
}


/**
 *  constructor allows to pass in a local address to bind.
 *  endpoint creates a copy to use, so caller can destroy instance of address anytime
 */
csm::network::EndpointPTP_sec_base::EndpointPTP_sec_base( const csm::network::Address_sptr aLocalAddr,
                                                          const csm::network::EndpointOptions_sptr aOptions )
: csm::network::EndpointPTP_base( aLocalAddr, aOptions )
{
  _SSLStruct = nullptr;
  _BIO = nullptr;

  _SendBuffer = new char[ DGRAM_PAYLOAD_MAX ];

  csm::network::EndpointOptionsPTP_base_sptr ptpOptions =
      std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP_base>( _Options );

  SetupSSLContext( ptpOptions->_SSLFiles );

  _SSLContext = _gSSLContext;
}

// Constructing/Initializing a new endpoint for an existing socket
csm::network::EndpointPTP_sec_base::EndpointPTP_sec_base( const int aSocket,
                                                          SSL *aSSL,
                                                          BIO *aBIO,
                                                          const Address_sptr aLocalAddr,
                                                          const EndpointOptions_sptr aOptions )
: csm::network::EndpointPTP_base( aLocalAddr, aOptions )
{
  // requires the glocal ssl setup to be finished
  if(( aSocket < 0 ) || ( aSSL == nullptr ) || ( aBIO == nullptr ))
    throw csm::network::ExceptionEndpointDown("Endpoint creation from known SSL failed.");

  if( _gSSLContext == nullptr )
    throw csm::network::ExceptionEndpointDown("Endpoint creation from known SSL failed because of nullptr global SSL context." );

  _SSLContext = _gSSLContext;
  _Socket = aSocket;
  _SSLStruct = aSSL;
  _BIO = aBIO;
  _SendBuffer = new char[ DGRAM_PAYLOAD_MAX ];
}

csm::network::EndpointPTP_sec_base::EndpointPTP_sec_base( const Endpoint *aEP )
: csm::network::EndpointPTP_base( aEP ),
  _SSLContext( dynamic_cast<const EndpointPTP_sec_base*>(aEP)->_SSLContext ),
  _SSLStruct( dynamic_cast<const EndpointPTP_sec_base*>(aEP)->_SSLStruct ),
  _BIO( dynamic_cast<const EndpointPTP_sec_base*>(aEP)->_BIO )
{
  _SendBuffer = new char[ DGRAM_PAYLOAD_MAX ];
}

csm::network::EndpointPTP_sec_base::~EndpointPTP_sec_base( )
{
  if( _SSLStruct != nullptr )
    SSL_free( _SSLStruct );

  // BIO is apparently cleaned up by SSL_free
//  if( _BIO != nullptr )
//    BIO_free_all( _BIO );

  // if documentation is right, SSL_free() calls SSL_CTX_free
  // which in turn will delete the CTX only if the internal refcount is 0
  // so only free CTX if it's the server endpoint:
//    SSL_CTX_free( _SSLContext );
    _SSLContext = nullptr;

  delete [] _SendBuffer;
}

int csm::network::EndpointPTP_sec_base::Connect( const csm::network::Address_sptr aSrvAddr )
{
  int rc;
  rc = SSLConnectPrep();

  if( rc == 0 )
    _Connected = true;

  if( _Connected )
    rc = ConnectPost();

  return rc;
}

csm::network::Endpoint* csm::network::EndpointPTP_sec_base::Accept()
{
  return nullptr;
}


// message based send
ssize_t
csm::network::EndpointPTP_sec_base::SendTo( const csm::network::Message &aMsg,
                                   const csm::network::Address_sptr aRemoteAddr )
{
  return Send( aMsg );
}

ssize_t
csm::network::EndpointPTP_sec_base::SendMsgWrapper( const struct msghdr *aMsg, const int aFlags )
{
  // assemble the message since it consists of 2 iov (header + data)
  char *spos = _SendBuffer;
  size_t totalLen = 0;
  for( unsigned n=0; (n < aMsg->msg_iovlen) && (totalLen < DGRAM_PAYLOAD_MAX); ++n )
  {
    totalLen += aMsg->msg_iov[n].iov_len;
    if( totalLen > DGRAM_PAYLOAD_MAX )
      throw csm::network::ExceptionSend("Message exceeds buffer size.", E2BIG );
    memcpy( spos, aMsg->msg_iov[n].iov_base, aMsg->msg_iov[n].iov_len );
    spos += aMsg->msg_iov[n].iov_len;
  }

  int rc = BIO_write( _BIO, _SendBuffer, totalLen );

  if( rc < 0 )
  {
    rc = errno;
    switch( rc )
    {
      case ENOTCONN:
        throw csm::network::ExceptionEndpointDown("Send: Socket not connected.");
        break;
      default:
        throw csm::network::ExceptionSend("sendmsg errno=" + std::to_string(rc) );
    }
  }

  LOG(csmnet, debug) << "PTP::SendMsg: "
      << " total_len=" << aMsg->msg_iov[0].iov_len + aMsg->msg_iov[1].iov_len
      << " rc=" << rc << " errno=" << errno;

  return rc;
}

ssize_t csm::network::EndpointPTP_sec_base::Send( const csm::network::Message &aMsg )
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

  return SendMsgWrapper( &msg, MSG_WAITALL );
}

// message based receive
ssize_t
csm::network::EndpointPTP_sec_base::Recv( csm::network::Message &aMsg )
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
    void *buffer = _RecvBufferState.GetRecvBufferPtr();
    int space = _RecvBufferState.GetRecvSpace();
    rlen = BIO_read( _BIO, buffer, space );

    if( rlen <= 0 )
    {
      if (BIO_should_retry( _BIO ) )
        return 0;
      else
        throw csm::network::ExceptionEndpointDown( "Receive Error" );
    }

    _RecvBufferState.Update( rlen );
    rc = _RecvBufferState.Recv( aMsg );
    if( rc == 0 )
      return 0;

    LOG(csmnet, debug) << " RecvMsg: "
        << " rcvd=" << rlen
        << " msgrc=" << rc << " errno=" << errno << " buf_empty=" << _RecvBufferState.IsEmpty()
        << " CSMData=" << aMsg;
  }

  rc = RecvVerify( aMsg );
  return rc;
}

ssize_t
csm::network::EndpointPTP_sec_base::RecvFrom( csm::network::MessageAndAddress &aMsgAddr )
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

// setting up the global ssl context
void csm::network::EndpointPTP_sec_base::SetupSSLContext( const csm::network::SSLFilesCollection &i_SSLFiles )
{
  if( csm::network::EndpointCompute_sec::_gSSLContext != nullptr)
    return;

  unsigned long rc = 1;
  SSL_library_init();
  SSL_load_error_strings();
  ERR_load_BIO_strings();

  if( _LocalAddr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_AGGREGATOR )
  {
    _gSSLContext = SSL_CTX_new( SSLv23_method() );
  }
  else
  {
    if( IsServerEndpoint() )
      _gSSLContext = SSL_CTX_new( SSLv23_server_method() );
    else
      _gSSLContext = SSL_CTX_new( SSLv23_client_method() );
  }

  if( _gSSLContext == nullptr )
    throw csm::network::Exception( "Error creating SSL context" );

  char error_buffer[ 1024 ];

  ERR_clear_error();
  rc = SSL_CTX_load_verify_locations( _gSSLContext, i_SSLFiles._CAFile.c_str(), nullptr );
  if( rc != 1 )
  {
    std::string err_str = "";
    while( (rc = ERR_get_error()) > 0 )
    {
      ERR_error_string_n( rc, error_buffer, 1024 );
      err_str.append( error_buffer );
      err_str.append( " :: " );
    }
    throw csm::network::Exception( "Error loading verification files: " + i_SSLFiles._CAFile + "::" + err_str );
  }

  rc = SSL_CTX_use_certificate_file( _gSSLContext, i_SSLFiles._CredPem.c_str(), SSL_FILETYPE_PEM );
  if( rc != 1 )
  {
    std::string err_str = "";
    while( (rc = ERR_get_error()) > 0 )
    {
      ERR_error_string_n( rc, error_buffer, 1024 );
      err_str.append( error_buffer );
      err_str.append( " :: " );
    }
    throw csm::network::Exception( "Error loading cert from PEM file: " + i_SSLFiles._CredPem + "::" + err_str );
  }

  rc = SSL_CTX_use_PrivateKey_file( _gSSLContext, i_SSLFiles._CredPem.c_str(), SSL_FILETYPE_PEM );
  if( rc != 1 )
  {
    std::string err_str = "";
    while( (rc = ERR_get_error()) > 0 )
    {
      ERR_error_string_n( rc, error_buffer, 1024 );
      err_str.append( error_buffer );
      err_str.append( " :: " );
    }
    throw csm::network::Exception( "Error loading key from PEM file: " + i_SSLFiles._CredPem + "::" + err_str );
  }

  if( SSL_CTX_check_private_key( _gSSLContext ) != 1 )
    throw csm::network::Exception( "Error while checking SSL-keys." );

  SSL_CTX_set_verify( _gSSLContext, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr );
  SSL_CTX_set_verify_depth( _gSSLContext, 1 );
}

int
csm::network::EndpointPTP_sec_base::SSLConnectPrep()
{
  int rc = 0;

  LOG( csmnet, debug ) << "Beginning SSL Connection Setup";
  _SSLStruct = SSL_new( _gSSLContext );
  if( _SSLStruct == nullptr )
    throw csm::network::ExceptionEndpointDown( "SSL Creation failure during connect." );

  _BIO = BIO_new_socket(_Socket, BIO_NOCLOSE);
  if( _BIO == nullptr )
    throw csm::network::ExceptionEndpointDown( "BIO Creation failure during connect." );

  // retrieve the socket descriptor for this BIO ...
  int bsock;
  BIO_get_fd( _BIO, &bsock );
  if( bsock <= 0 )
    throw csm::network::ExceptionEndpointDown( "BIO Creation failure to retrieve socket number" );

  if( bsock != _Socket )
    LOG( csmnet, debug ) << "Connected socket: " << _Socket << " is different from BIO fd: " << bsock;

  // ... to make the socket non-blocking
  long current_setting = fcntl( bsock, F_GETFL, NULL );
  if( (current_setting & O_NONBLOCK) == 0 )
  {
    current_setting |= O_NONBLOCK;
    rc = fcntl( bsock, F_SETFL, current_setting );
    if( rc )
      throw csm::network::ExceptionEndpointDown("fcntl: NONBLOCK");
  }

  SSL_set_bio(_SSLStruct, _BIO, _BIO);

  LOG( csmnet, debug ) << "SSL Connecting...";
  ERR_clear_error();
  bool keep_retrying = true;
  std::chrono::time_point< std::chrono::steady_clock > end = std::chrono::steady_clock::now() + std::chrono::milliseconds( 1000 );

  while( keep_retrying )
  {
    rc = SSL_connect( _SSLStruct );
    switch( rc )
    {
      case 0: // unable to continue error
        LOG( csmnet, error ) << "SSL Connection error.";
        throw csm::network::ExceptionEndpointDown( SSLExtractError( rc, " SSL_Connect: " ) );

      case 1: // successful connection
        LOG( csmnet, debug ) << "SSL Connection complete.";
        keep_retrying = false;
        break;

      default: // potentially incomplete connect or other serious error
        LOG( csmnet, trace ) << "SSL Connection incomplete.";
        rc = SSL_get_error( _SSLStruct, rc );
        // since we already pulled the first error, we need to add the error to the prefix for the full error string
        std::string err_str = " SSL_Connect: " + SSLPrintError( rc );
        LOG( csmnet, trace ) << "SSL Connection status: rc=" << rc;
        switch( rc )
        {
          case SSL_ERROR_NONE: // seems unlikely, but we better cover that case
            keep_retrying = false; // we're connected
            break;

          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            rc = csm::network::EndpointPTP_base::CheckConnectActivity( bsock, true ); // check read and write activity
            if( rc != 0 )
              throw csm::network::ExceptionEndpointDown( "SSL Connection failed.", rc );
            if( std::chrono::steady_clock::now() > end )
              throw csm::network::ExceptionEndpointDown( "SSL Connection timed out", ETIMEDOUT );
            break;

          default:
            throw csm::network::ExceptionEndpointDown( SSLExtractError( rc, err_str ) );
            break;
        }
    }
  }

  if( SSL_get_verify_result( _SSLStruct ) != X509_V_OK )
    throw csm::network::ExceptionEndpointDown( "SSL verification failed." );
  else
    rc = 0;

  current_setting &= (~O_NONBLOCK);
  rc = fcntl( bsock, F_SETFL, current_setting );
  if( rc )
    throw csm::network::ExceptionEndpointDown("fcntl: NONBLOCK");

  return rc;
}








/**
 *  constructor allows to pass in a local address to bind.
 *  endpoint creates a copy to use, so caller can destroy instance of address anytime
 */
csm::network::EndpointCompute_sec::EndpointCompute_sec( const csm::network::Address_sptr aLocalAddr,
                                        const csm::network::EndpointOptions_sptr aOptions )
: csm::network::EndpointPTP_sec_base( aLocalAddr, aOptions )
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

csm::network::Endpoint* csm::network::EndpointCompute_sec::Accept()
{
  return GenericAcceptSSL<csm::network::AddressPTP, csm::network::EndpointCompute_sec>();
}


int csm::network::EndpointCompute_sec::Connect( const csm::network::Address_sptr aSrvAddr )
{
  int rc = ConnectPrep<csm::network::AddressPTP>( aSrvAddr );

  // stop any ssl setup if not connected
  if( rc != 0 )
    return rc;

  return EndpointPTP_sec_base::Connect( aSrvAddr );
}

ssize_t csm::network::EndpointCompute_sec::SendTo( const csm::network::Message &aMsg,
                                                   const csm::network::Address_sptr aRemoteAddr )
{
  const csm::network::AddressPTP *addr = dynamic_cast<const csm::network::AddressPTP*>( aRemoteAddr.get() );
  if( *addr != *(csm::network::AddressPTP*)_RemoteAddr.get() )
    LOG( csmnet, warning ) << "EndpointPTP_sec::SendTo(): Request to send to different address than connected. Given address IGNORED!";

  return csm::network::EndpointPTP_sec_base::Send( aMsg );
}

#endif  // SECURE_COMM
