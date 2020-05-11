/*================================================================================

    csmnet/src/CPP/endpoint_ptp_sec.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_ENDPOINT_COMPUTE_SEC_H_
#define CSMNET_SRC_CPP_ENDPOINT_COMPUTE_SEC_H_

#include <unistd.h>      // close()
#include <sys/socket.h>  // socket calls
#include <sys/un.h>      // struct sockaddr_un
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

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

#ifdef SECURE_COMM

std::string SSLPrintError( const int err );

/* Notes: TCP endpoint
 *  * only covers a single endpoint (i.e. creates a separate endpoint in accept() )
 *  * IsServer() is true only for the listening socket
 *  * Server/listening socket is set to non-blocking
 *  * other sockets provide non-blocking operations via flags
 */

class EndpointPTP_sec_base : public EndpointPTP_base {
protected:
  static SSL_CTX *_gSSLContext;
  SSL_CTX *_SSLContext;
  SSL *_SSLStruct;
  BIO *_BIO;
  char *_SendBuffer;

  EndpointPTP_sec_base()
  : EndpointPTP_base(),
    _SSLContext(nullptr),
    _SSLStruct(nullptr),
    _BIO(nullptr),
    _SendBuffer(nullptr)
  {}
public:
  EndpointPTP_sec_base( const Address_sptr aLocalAddr,
                        const EndpointOptions_sptr aOptions );
  EndpointPTP_sec_base( const int aSocket,
                        SSL *aSSL,
                        BIO *aBIO,
                        const Address_sptr aLocalAddr,
                        const EndpointOptions_sptr aOptions );
  EndpointPTP_sec_base( const Endpoint * aEndpoint );
  virtual ~EndpointPTP_sec_base( );


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

protected:
  virtual ssize_t SendMsgWrapper( const struct msghdr *aMsg, const int aFlags );
  void SetupSSLContext( const csm::network::SSLFilesCollection &i_SSLFiles );
  int SSLConnectPrep();

  inline std::string SSLExtractError( int aRc, std::string aPrefix )
  {
    if( _SSLStruct == nullptr )
      return aPrefix + " SSL not fully initialized.";

    int rc = aRc;
    std::string err_str = aPrefix;
    while( (rc = SSL_get_error( _SSLStruct, rc )) > 0 )
    {
      err_str.append( SSLPrintError( rc ) );
      err_str.append( " :: " );
    }
    SSL_clear( _SSLStruct );
    return err_str;
  }

  template<typename AddressClass, typename EndpointClass>
  csm::network::Endpoint* GenericAcceptSSL()
  {
    if(( _Socket ) && ( !IsServerEndpoint() ))
      throw csm::network::Exception( "Accept on non-server socket" );

    sockaddr_in CltAddr;
    unsigned int addrsize = sizeof( sockaddr_in );
    bzero( &CltAddr, addrsize );
    int newsock = accept( _Socket, (sockaddr*)&CltAddr, &addrsize );
    if( newsock < 0 )
    {
      if(( errno == EAGAIN ) || ( errno == EWOULDBLOCK ))
        return nullptr;
      throw csm::network::ExceptionEndpointDown("Failed accept");
    }

    BIO *newbio = BIO_new_socket(newsock, BIO_NOCLOSE);
    if( newbio == nullptr )
    {
      close( newsock );
      throw csm::network::ExceptionEndpointDown("Failed to create SSL BIO");
    }

    SSL *newssl = SSL_new( _gSSLContext );
    if( ! newssl )
    {
      // todo: cleanup BIO
      close( newsock );
      throw csm::network::ExceptionEndpointDown("Failed to create new SSL endpoint.");
    }

    SSL_set_bio(newssl, newbio, newbio);

    /* Do the SSL Handshake */
    int rc = SSL_accept( newssl );
    LOG( csmnet, debug ) << "SSL_accept rc: " << rc;
    if( rc != 1 )
    {
      std::string err_str = SSLExtractError( rc, " SSL ACCEPT: " );
      // todo: cleanup BIO
      close( newsock );
      throw csm::network::ExceptionEndpointDown(err_str, rc);
    }

    // SSL get peer cert
    // non blocking?

    // This part of the code is the "server"
    // Handels incoming connections

    // The following checks are in case a SSL cert fails in some way.
    // By doing these checks, we can fail in a more graceful and informative way. 
    
    X509 *peer_cert = SSL_get_peer_certificate(newssl);
    if(peer_cert == nullptr)
    {
      close( newsock );
      // todo: cleanup newssl
      throw csm::network::ExceptionEndpointDown("SSL Peer Certificate unavailable but required.");
    }

    long SGVR_rc = SSL_get_verify_result(newssl);

    LOG( csmnet, debug ) << "SSL_get_verify_result return code: " << SGVR_rc;

    if( SGVR_rc != X509_V_OK )
    {
      close( newsock );
      // todo: cleanup newssl
      throw csm::network::ExceptionEndpointDown("SSL verification failed." );
    }
    else
    {
      rc = 0;
    }

    // Debug prints to help with SSL issues.
    // Print out connection details
    LOG( csmnet, debug ) << "SSL_get_version: " << SSL_get_version(newssl);
    LOG( csmnet, debug ) << "SSL_get_cipher: " << SSL_get_cipher(newssl);

    // The reference count of the X509 object is incremented by one, 
    // so that it will not be destroyed when the session containing the peer certificate is freed. 
    // The X509 object must be explicitly freed using X509_free().
    X509_free(peer_cert);
    
    EndpointClass *ret = nullptr;
    if( newsock >= 0 )
    {
      // need to create new set of options based on listening endpoint and address class
      std::shared_ptr<AddressClass> raddr = std::make_shared<AddressClass>( htonl( CltAddr.sin_addr.s_addr ),
                                                                            htons( CltAddr.sin_port ) );
      std::shared_ptr<EndpointOptionsPTP<AddressClass>> options =
          std::dynamic_pointer_cast<csm::network::EndpointOptionsPTP<AddressClass>>(GetOptions());
      uint32_t interval = options->getHeartbeatInterval();
      LOG(csmnet, debug) << "EndpointPTP_sec::Accept(): new client: " << (raddr->Dump());
      std::shared_ptr<csm::network::EndpointOptionsPTP<AddressClass>> new_opts =
          std::make_shared<csm::network::EndpointOptionsPTP<AddressClass>>( false, options->_SSLFiles, false, interval );

      ret = new EndpointClass( newsock, newssl, newbio, _LocalAddr, new_opts );
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
};

class EndpointCompute_sec : public EndpointPTP_sec_base {
protected:
  EndpointCompute_sec()
  : EndpointPTP_sec_base()
  {}

public:
  EndpointCompute_sec( const Address_sptr aLocalAddr,
                   const EndpointOptions_sptr aOptions );
  EndpointCompute_sec( const int aSocket,
                   SSL *aSSL,
                   BIO *aBIO,
                   const Address_sptr aLocalAddr,
                   const EndpointOptions_sptr aOptions )
  : EndpointPTP_sec_base( aSocket, aSSL, aBIO, aLocalAddr, aOptions )
  {}
  EndpointCompute_sec( const Endpoint * aEndpoint )
  : EndpointPTP_sec_base( aEndpoint )
  {}
  virtual ~EndpointCompute_sec( ) {}

  virtual int Connect( const csm::network::Address_sptr aSrvAddr );
  virtual Endpoint* Accept( );

  virtual ssize_t SendTo( const csm::network::Message &aMsg,
                          const Address_sptr aRemoteAddr );

};



#else // SECURE_COMM

// if secure comm is not built in but user requested SSL via configuration,
// we can only fall back to regular sockets
typedef EndpointCompute_plain EndpointCompute_sec;

#endif // SECURE_COMM

}  // namespace network
} // namespace csm

#endif /* CSMNET_SRC_CPP_ENDPOINT_PTP_H_ */
