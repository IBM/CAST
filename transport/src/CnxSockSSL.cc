/*******************************************************************************
 |    CnxSockSSL.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

// Includes
#include <exception>
#include "CnxSockSSL.h"
#include "txp_flightlog.h"
#include "txperror.h"

using namespace std;
using namespace txp;

thread_local TXP_Handler txp::txperror;

CnxSockSSL::CnxSockSSL(int sockFamily, int sockType, std::string pConnectionName, int protocol) :
txp::CnxSockSSL(sockFamily, sockType, protocol)
{
    _connectionName=pConnectionName;
}

CnxSockSSL::CnxSockSSL(int sockFamily, int sockType, int protocol) :
    txp::CnxSockSSL()
{
    _family = sockFamily;
    _type= sockType;
    _protocol=protocol;
    _sockfd=-1;
    _flagsOnOpen=O_CLOEXEC;
    memset(&_sockaddrLocal,0,sizeof(_sockaddrLocal));
    memset(&_sockaddrRemote,0,sizeof(_sockaddrRemote));
    _sockaddrLocal.sa_family=sockFamily;
    _sockaddrRemote.sa_family=sockFamily;
    _sockaddrlen = sizeof(struct sockaddr_in);
    _dialRemote=0; //assume listening
    _rcLast=0;
    const SSL_METHOD * method = SSLv23_method();
    _sslctx = SSL_CTX_new(method);
    if( NULL == _sslctx  )
        {
            LOG(txp,error) << "Cannot create SSL context structure: error=" << ERR_get_error();
            ERR_print_errors_fp(stderr);
            throw runtime_error(string("Cannot create SSL context structure: error=") + to_string(ERR_get_error()));
        }
};

CnxSockSSL::CnxSockSSL(const CnxSockSSL &pCnxSock) :
    txp::CnxSockSSL()
{
    _family=pCnxSock._family;
    _type=pCnxSock._type;
    _protocol=pCnxSock._protocol;
    _sockfd=pCnxSock._sockfd;
    _sockaddrlen=pCnxSock._sockaddrlen;
    _flagsOnOpen=pCnxSock._flagsOnOpen;
    _dialRemote=pCnxSock._dialRemote;
    _rcLast=pCnxSock._rcLast;
    memcpy(&_sockaddrLocal, (void*)&(pCnxSock._sockaddrLocal), sizeof(struct sockaddr));
    memcpy(&_sockaddrRemote, (void*)&(pCnxSock._sockaddrRemote), sizeof(struct sockaddr));
    _sslctx = pCnxSock._sslctx;
    _keepAliveIntvl=pCnxSock._keepAliveIntvl;
    _keepAliveIdle=pCnxSock._keepAliveIdle;
    _keepAliveCount=pCnxSock._keepAliveCount;
    _turnOnKeepAlive=pCnxSock._turnOnKeepAlive;

};


void CnxSockSSL::loadCertificates(string certificate, string key)
{
    int rc;
    
    rc = SSL_CTX_use_certificate_file(_sslctx, certificate.c_str() , SSL_FILETYPE_PEM);
    if(rc != 1)
    {
        throw(runtime_error(string("Unable to load certificate file: \"") + certificate + string("\"")));
    }
    
    rc = SSL_CTX_use_PrivateKey_file(_sslctx, key.c_str(), SSL_FILETYPE_PEM);
    if(rc != 1)
    {
        throw(runtime_error(string("Unable to load private key: \"") + key + string("\"")));
    }
    
    rc = SSL_CTX_check_private_key(_sslctx);
    if(rc != 1)
    {
        throw(runtime_error(string("Private key (") + key + string(") does not match the public certificate (") + certificate + string(")")));
    }
    return;
}

void CnxSockSSL::loadCertificateToTrustStore(string certfile)
{
    int rc;
    FILE* fp = fopen(certfile.c_str(), "rb");
    if(fp == NULL)
	txperror << err("certfile", certfile) << errloc(0) << RAS("bb.net.certLoadFailed") << bailout;

    X509 *cert = PEM_read_X509(fp, NULL, NULL, NULL);
    fclose(fp);
    if(cert == NULL)
	txperror << err("PEM_read_X509_failed", NULL) << RAS("bb.net.certLoadFailed") << bailout;

    if(trustStore == NULL)
	trustStore = X509_STORE_new();

    rc = X509_STORE_add_cert(trustStore, cert);
    X509_free(cert);
    
    if(rc != 1)
	txperror << errloc(rc) << RAS("bb.net.certLoadFailed") << bailout;
}

void CnxSockSSL::verifyCertificates()
{
    X509 *cert;
    char *line;
    int rc;

    cert = SSL_get_peer_certificate(_cSSL);/* get the server's certificate */
    if ( cert != NULL )
    {
	line = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
	LOG(bb,info) << "x509 Subject: " << line;
	free(line); /* free the malloc'ed string */

	line = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);
	LOG(bb,info) << "x509 Issuer : " << line;
	free(line); /* free the malloc'ed string */

	X509_STORE_CTX* ctx = X509_STORE_CTX_new();
	if(ctx == NULL)
	    txperror << err("X509_STORE_CTX_new_rc", 0) << RAS("bb.net.certVerifyFailed") << bailout;
	if(trustStore == NULL)
	    txperror << err("X509_trustStore", NULL) << RAS("bb.net.certVerifyFailed") << bailout;

	rc = X509_STORE_CTX_init(ctx, trustStore, cert, NULL);
	if(rc != 1)
	    txperror << err("X509_verify_rc", rc) << errloc(rc) << RAS("bb.net.certVerifyFailed") << bailout;

	rc = X509_verify_cert(ctx);
	if(rc != 1)
	    txperror << err("X509_verify_rc", rc) << errloc(rc) << RAS("bb.net.certVerifyFailed") << bailout;

	LOG(bb,info) << "x509 certificate verified success";

	X509_STORE_CTX_cleanup(ctx);
	X509_STORE_CTX_free(ctx);
	X509_free(cert);
    }
    else
    {
	txperror << err("SSL_get_peer_certificate", NULL) << RAS("bb.net.certVerifyFailed") << bailout;
    }

    return;
}

int CnxSockSSL::accept() {
    socklen_t lSockaddrlen;
    _rcLast = accept4(_sockfd,&_sockaddrRemote,&lSockaddrlen, SOCK_CLOEXEC);
    if (_rcLast<0) {
        LOG(txp,warning)<< __PRETTY_FUNCTION__<<"_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
    }
    if (_rcLast>0)
        {
            close(_sockfd); //close listening CnxSockSSL
            _sockfd=_rcLast;
            int RCgetpeername=getpeername(_sockfd, &_sockaddrRemote, &lSockaddrlen);
            if (RCgetpeername){
                LOG(txp,always)<<"CnxSockSSL::accept() getpeername errno="<<errno<<", "<<strerror(errno);
            }
            int RCgetsockname = getsockname(_sockfd, &_sockaddrLocal, &_sockaddrlen);
            if (RCgetsockname) {
                LOG(txp,warning)<<__PRETTY_FUNCTION__<< " getsockname errno="<<errno<<", "<<strerror(errno);
            }
            LOG(txp,always)<<"CnxSockSSL::accept() "<<getInfoString()<<" sockfd="<<_sockfd;
            _cSSL= SSL_new(_sslctx);
            SSL_set_fd(_cSSL, _sockfd);
            if (SSL_accept(_cSSL)<0)
                {
                    LOG(txp,error) << __PRETTY_FUNCTION__<< "SSL handcheck failed";
                    return -1;
                }

            errno=0;
        }
    return _rcLast;
}

int CnxSockSSL::accept(txp::Connex* &pNewSock) {
    int l_RC = 0;

    if (!_dialRemote) {
        CnxSockSSL* l_NewSock = new CnxSockSSL(*this);
        socklen_t l_Sockaddrlen = sizeof(l_NewSock->_sockaddrRemote);
        l_NewSock->_rcLast = accept4(l_NewSock->_sockfd, &(l_NewSock->_sockaddrRemote), &l_Sockaddrlen, SOCK_CLOEXEC);
        l_RC = l_NewSock->_rcLast;
        if (l_RC < 0) {
            LOG(txp,warning)<< __PRETTY_FUNCTION__<<" l_RC="<< l_RC<< " errno="<<errno<<", "<<strerror(errno);
            delete l_NewSock;
        } else {
            l_NewSock->_sockfd = l_NewSock->_rcLast;
            int RCgetpeername=getpeername(l_NewSock->_sockfd, &(l_NewSock->_sockaddrRemote), &l_Sockaddrlen);
            if (RCgetpeername){
                LOG(txp,always)<<"CnxSockSSL::accept(p) getpeername errno="<<errno<<", "<<strerror(errno);
            }
            int RCgetsockname = getsockname(l_NewSock->_sockfd, &(l_NewSock->_sockaddrLocal), &l_Sockaddrlen);
            if (RCgetsockname) {
                LOG(txp,warning)<<__PRETTY_FUNCTION__<< " getsockname errno="<<errno<<", "<<strerror(errno);
            }
            LOG(txp,always)<<"CnxSockSSL::accept(p) "<<l_NewSock->getInfoString()<<" sockfd="<<l_NewSock->_sockfd;

            l_NewSock->_cSSL= SSL_new(l_NewSock->_sslctx);
            SSL_set_fd(l_NewSock->_cSSL, l_NewSock->_sockfd);
            if (SSL_accept(l_NewSock->_cSSL)<0)
                {
                    LOG(txp,error) << __PRETTY_FUNCTION__<< "SSL handcheck failed";
                    delete l_NewSock;
                    return -2;
                }
            pNewSock = l_NewSock;
            errno = 0;
        }
    } else {
        l_RC = -2;
    }

    return l_RC;
}


int CnxSockSSL::bindCnxSock() {
    setReuseAddress(1);
    _rcLast = bind(_sockfd,&_sockaddrLocal,_sockaddrlen);
    if (_rcLast<0) {
        LOG(txp,warning)<< __PRETTY_FUNCTION__<<":"<<__LINE__<<" _rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
    }

    return _rcLast;
}

int CnxSockSSL::connect2Remote(){
    _sockaddrlen=sizeof(_sockaddrRemote);
    _rcLast = connect(_sockfd, &_sockaddrRemote,_sockaddrlen);
    if (_rcLast<0) {
        LOG(txp,warning)<< __PRETTY_FUNCTION__<< "_rcLast="<< _rcLast << " errno="<<errno<<", "<<strerror(errno);
    } else {
        _sockaddrlen=sizeof(_sockaddrRemote);
        int RCgetsockname = getsockname(_sockfd, &_sockaddrLocal, &_sockaddrlen);
        if (RCgetsockname) {
            LOG(txp,warning)<<__PRETTY_FUNCTION__<< " getsockname errno="<<errno<<", "<<strerror(errno);
        }
        int RCgetpeername = getpeername(_sockfd, &_sockaddrRemote, &_sockaddrlen);
        if (RCgetpeername){
            LOG(txp,always)<<"CnxSock::connect2Remote getpeername errno="<<errno<<", "<<strerror(errno);
        }
        LOG(txp,always)<< "CnxSockSSL::connect2Remote() "<< getInfoString()<<" sockfd="<<_sockfd;
        _cSSL= SSL_new(_sslctx);
        SSL_set_fd(_cSSL, _sockfd);
        if ( SSL_connect(_cSSL) <0 )   /* perform the connection handshake*/
            {
                LOG(txp,error) << __PRETTY_FUNCTION__<<" ssl connect failed" << strerror(errno);
                return -1;
            }
    }

    return _rcLast;
}

void CnxSockSSL::setLocalAddr(in_addr_t addr, uint16_t port) {
    struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrLocal;
    sockaddr_in.sin_family = _family;
    memcpy(&sockaddr_in.sin_addr.s_addr, &addr, sizeof(in_addr_t));
    sockaddr_in.sin_port = htons(port);
}

void CnxSockSSL::setRemoteAddr(in_addr_t addr, uint16_t port) {
    struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrRemote;
    sockaddr_in.sin_family = _family;
    memcpy(&sockaddr_in.sin_addr.s_addr, &addr, sizeof(in_addr_t));
    sockaddr_in.sin_port = htons(port);
}
