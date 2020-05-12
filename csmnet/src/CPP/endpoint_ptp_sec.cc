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
  _SSLContext( (dynamic_cast<const EndpointPTP_sec_base*>(aEP) == nullptr ) ? nullptr : dynamic_cast<const EndpointPTP_sec_base*>(aEP)->_SSLContext ),
  _SSLStruct( (dynamic_cast<const EndpointPTP_sec_base*>(aEP) == nullptr ) ? nullptr : dynamic_cast<const EndpointPTP_sec_base*>(aEP)->_SSLStruct ),
  _BIO( (dynamic_cast<const EndpointPTP_sec_base*>(aEP) == nullptr ) ? nullptr : dynamic_cast<const EndpointPTP_sec_base*>(aEP)->_BIO )
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

  // RH 8 - CSM 1.8 - Lars and Nick fix

  // Before RH8, we had a BIO write here. When we switched to RH8, SSL coms between daemons broke.
  // we beleived that the SSL cert wasn't 100% verified or handeled correctly.
  // By calling SSL_write() instead of BIO_write(), it seemed like the connection and verification was handled properly and cleanly.
  // leaving this comment and original code here in case a future version creates a similar problem. 
  // This may help guide a future support debug. 

  //int rc = BIO_write( _BIO, _SendBuffer, totalLen );

  int rlen = SSL_write( _SSLStruct, _SendBuffer, totalLen );
  // Send some debug info into the log
  LOG( csmnet, debug ) << "SSL_write: return length=" << rlen << " Anything > 0 is good. The read operation was successful.";

  // when rlen > 0, then The write operation was successful. 
  // The return value is the number of bytes actually written to the TLS/SSL connection.

  // for return code used later. 
  int rc = 0;

  if(rlen <= 0)
  {
    //The write operation was not successful, because either the connection was closed, an error occurred or action must be taken by the calling process. 
    //Call SSL_get_error() with the return value ret to find out the reason.

    //SSLv2 (deprecated) does not support a shutdown alert protocol, so it can only be detected, whether the underlying connection was closed. 
    //It cannot be checked, why the closure happened.

    //Old documentation indicated a difference between 0 and -1, and that -1 was retryable. 
    //You should instead call SSL_get_error() to find out if it's retryable.

    rc = SSL_get_error(_SSLStruct, rlen);

    // Send some debug info into the log
    LOG( csmnet, error ) << "SSL_get_error: rc=" << rc;

    // since we already pulled the first error, we need to add the error to the prefix for the full error string
    std::string err_str = " SSL_Write: " + SSLPrintError( rlen );

    // Process the error
    switch( rc )
    {
      // There are a whole lot of return values.
      // the full docs can be seen at: https://www.openssl.org/docs/man1.1.1/man3/SSL_get_error.html

      // seems unlikely, but we better cover this case
      case SSL_ERROR_NONE: 
      {
        // The TLS/SSL I/O operation completed. 
        // This result code is returned if and only if return value of the SSL_read was > 0.
        // but because we are in a failure case of rlen <= 0, then we should NEVER get into this case...
        LOG( csmnet, error ) << "SSL_ERROR_NONE";
        LOG( csmnet, error ) << "Unknown logic. SSL_read() reports a failure, but SSL_get_error() reports no error.";
        throw csm::network::ExceptionEndpointDown( "Receive Error" );
        break;
      }
      // There was a case for a "ENOTCONN" in the BIO write code, but that error doesnt exist in the "SSL_get_error" context
      default:
      {
        // original default exception CSM throws
        // Prints the error code returned from SSL_get_error to the CSM logs and throws an exception. 
        throw csm::network::ExceptionSend("sendmsg errno=" + std::to_string(rc) );
      }
    }
  }

  LOG(csmnet, debug) << "PTP_sec::SendMsg: "
      << " total_len=" << aMsg->msg_iov[0].iov_len + aMsg->msg_iov[1].iov_len
      << " rlen=" << rlen << " errno=" << errno;

  return rlen;
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
    LOG(csmnet, debug) << "PTP_sec:: RecvMsg from Buffer: "
        << " rlen=" << rlen
        << " CSMData=" << aMsg;
    rc = rlen;
  }
  else
  {
    void *buffer = _RecvBufferState.GetRecvBufferPtr();
    int space = _RecvBufferState.GetRecvSpace();


    // RH 8 - CSM 1.8 - Lars and Nick fix

    // Before RH8, we had a BIO read here. When we switched to RH8, SSL coms between daemons broke.
    // We beleived that the SSL cert wasn't 100% verified or handeled correctly.
    // By calling SSL_read() instead of BIO_read(), it seemed like the connection and verification was handled properly and cleanly.
    // leaving this comment and original code here in case a future version creates a similar problem. 
    // This may help guide a future support debug. 

    //rlen = BIO_read( _BIO, buffer, space );

    rlen = SSL_read( _SSLStruct, buffer, space );
    // Send some debug info into the log
    LOG( csmnet, debug ) << "SSL_read: return length=" << rlen << " Anything >= 0 is good. The read operation was successful.";

    // when rlen >= 0, then The read operation was successful. 
    // The return value is the number of bytes actually read from the TLS/SSL connection.

    if( rlen <= 0 )
    {
      // The read operation was not successful, 
      // because either the connection was closed, an error occurred, or action must be taken by the calling process. 
      // Call SSL_get_error(3) with the return value ret to find out the reason.

      // Old documentation indicated a difference between 0 and -1, and that -1 was retryable. 
      // You should instead call SSL_get_error() to find out if it's retryable.
      rc = SSL_get_error(_SSLStruct, rlen);

      // Send some debug info into the log
      LOG( csmnet, error ) << "SSL_get_error: rc=" << rc;

      // since we already pulled the first error, we need to add the error to the prefix for the full error string
      std::string err_str = " SSL_Read: " + SSLPrintError( rlen );

      // Process the error
      switch( rc )
      {
          // There are a whole lot of return values.
          // the full docs can be seen at: https://www.openssl.org/docs/man1.1.1/man3/SSL_get_error.html

          // seems unlikely, but we better cover this case
          case SSL_ERROR_NONE: 
          {
            // The TLS/SSL I/O operation completed. 
            // This result code is returned if and only if return value of the SSL_read was > 0.
            // but because we are in a failure case of rlen <= 0, then we should NEVER get into this case...
            LOG( csmnet, error ) << "SSL_ERROR_NONE";
            LOG( csmnet, error ) << "Unknown logic. SSL_read() reports a failure, but SSL_get_error() reports no error.";
            throw csm::network::ExceptionEndpointDown( "Receive Error" );
            break;
          }
          case SSL_ERROR_ZERO_RETURN:
          {
            // The TLS/SSL peer has closed the connection for writing by sending the close_notify alert. 
            // No more data can be read. 
            // Note that SSL_ERROR_ZERO_RETURN does not necessarily indicate that the underlying transport has been closed.
            LOG( csmnet, error ) << "SSL_ERROR_ZERO_RETURN";
            LOG( csmnet, error ) << "The TLS/SSL peer has closed the connection for writing by sending the close_notify alert. No more data can be read.";
            throw csm::network::ExceptionEndpointDown( "Receive Error" );
            break;
          }
          case SSL_ERROR_WANT_READ:
          {
            // The operation did not complete and can be retried later.

            // logically this would be the same path as:
            /*
            if (BIO_should_retry( _BIO ) )
              return 0;
            */

            // So I'll also return 0.
            
            // Lars: return 0 is the correct behavior for the endpoint::recv call because 
            // it means that currently there's not more data to receive so we shouldn't block on another recv call 
            // and instead see if any other endpoint has inbound data or any outbound work is to be done.

            return 0;
          }
          default:
          {
            // Use info above to call a CSM ccustom error report. 
            throw csm::network::ExceptionEndpointDown( SSLExtractError( rc, err_str ) );
            // Before when we had the BIO read write... the error message was a simple text "receive error" as seen below.
            // throw csm::network::ExceptionEndpointDown( "Receive Error" );
            break;
          }
      }
    }

    _RecvBufferState.Update( rlen );
    rc = _RecvBufferState.Recv( aMsg );
    if( rc == 0 )
      return 0;

    LOG(csmnet, debug) << "PTP_sec:: RecvMsg: "
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
    // We had "SSLv23_method" pre RH8. 
    // Lars suggest that we change to TLS. I'm not sure why, but he knows more about it than me. 
    // Leaving here incase it breaks again in the future and we have to debug around this. 
    // This might be a note of different methods to investigate. 

    //_gSSLContext = SSL_CTX_new( SSLv23_method() );
    _gSSLContext = SSL_CTX_new( TLS_method() );
  }
  else
  {
    if( IsServerEndpoint() )
      //_gSSLContext = SSL_CTX_new( SSLv23_server_method() );
      _gSSLContext = SSL_CTX_new( TLS_server_method() );
    else
      //_gSSLContext = SSL_CTX_new( SSLv23_client_method() );
      _gSSLContext = SSL_CTX_new( TLS_client_method() );
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

  // Check to see how to set up peer verification
  // servers and clients need to verify differently. 
  int verifyer_flags = SSL_VERIFY_PEER;
  if( IsServerEndpoint() )
  {
    verifyer_flags |= SSL_VERIFY_CLIENT_ONCE | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  }  
   SSL_CTX_set_verify( _gSSLContext, verifyer_flags, nullptr );
   SSL_CTX_set_verify_depth( _gSSLContext, 1 );
   SSL_CTX_set_mode( _gSSLContext, SSL_MODE_AUTO_RETRY );
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

  current_setting &= (~O_NONBLOCK);
  rc = fcntl( bsock, F_SETFL, current_setting );
  if( rc )
  {
    throw csm::network::ExceptionEndpointDown("fcntl: NONBLOCK");
  }

  // SSL get peer cert
  // non blocking?

  // This part of the code is the "client"
  // Handels when we try to go out and get verified, and dealing with responses.

  // The following checks are in case a SSL cert fails in some way.
  // By doing these checks, we can fail in a more graceful and informative way. 
  
  X509 *peer_cert = SSL_get_peer_certificate(_SSLStruct);
  if(peer_cert == nullptr)
  {
    throw csm::network::ExceptionEndpointDown("SSL Peer Certificate unavailable but required.");
  }

  long SGVR_rc = SSL_get_verify_result(_SSLStruct);

  LOG( csmnet, debug ) << "SSL_get_verify_result return code: " << SGVR_rc;

  if( SGVR_rc != X509_V_OK )
  {
    throw csm::network::ExceptionEndpointDown( "SSL verification failed." );
  }
  else
  {
    rc = 0;
  }

  // The reference count of the X509 object is incremented by one, 
  //so that it will not be destroyed when the session containing the peer certificate is freed. 
  //The X509 object must be explicitly freed using X509_free().
  X509_free(peer_cert);
  

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
  if( addr == nullptr )
    throw csm::network::ExceptionProtocol("Remote address has wrong type", ENOTCONN );

  if( *addr != *(csm::network::AddressPTP*)_RemoteAddr.get() )
    LOG( csmnet, warning ) << "EndpointPTP_sec::SendTo(): Request to send to different address than connected. Given address IGNORED!";

  return csm::network::EndpointPTP_sec_base::Send( aMsg );
}

#endif  // SECURE_COMM
