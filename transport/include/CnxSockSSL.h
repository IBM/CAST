/*******************************************************************************
 |    CnxSockSSL.h
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


//! \file  CnxSock.h
//! \brief Connection objects include for setting up and using communications
/**********************************/
#ifndef CNX_CnxSockSSL_H
#define CNX_CnxSockSSL_H

#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <exception>
#include "Connex.h"


namespace txp{
//! \class CnxSockSSL
//! \brief Connection object sockets class
//! \note  Currently supports only IPv4 with secured connection.

    class CnxSockSSL: public ConnexSocket
    {
    private:
        struct sockaddr _sockaddrLocal;
        struct sockaddr _sockaddrRemote;
        socklen_t _sockaddrlen;

        SSL_CTX *_sslctx;
        SSL *_cSSL;
	X509_STORE* trustStore;
	
    public:
        CnxSockSSL() : ConnexSocket() {
            _sslctx = NULL;
            _cSSL = NULL;
	    trustStore = NULL;
        _sockaddrLocal.sa_family=AF_INET;
        _sockaddrRemote.sa_family=AF_INET;
        _sockaddrlen = sizeof(struct sockaddr_in);
        }
        /**
         * \brief Constructor to setup base values for <b>ipv4</b>
         * \param[in] sockFamily which should be AF_INET
         * \param[in] sockType
         * \param[in] protocol which should be 0
         * \return object
         * \post data squirreled away
         */
        CnxSockSSL(int sockFamily, int sockType, int protocol=0);
        CnxSockSSL(int sockFamily, int sockType, std::string pConnectionName, int protocol=0);
        CnxSockSSL(const CnxSockSSL &pCnxSock);
        ~CnxSockSSL(){
            if(NULL != _cSSL)
	    {
                SSL_free(_cSSL);
                _cSSL=NULL;
            }
	    if(NULL != trustStore)
	    {
		X509_STORE_free(trustStore);
		trustStore = NULL;
	    }
            //dialers use separate _sslctx; accepters share same _sslctx as listener
            if (_dialRemote) if (_sslctx) SSL_CTX_free(_sslctx);
        }

        virtual int connect2Remote();
        virtual int bindCnxSock();
        virtual int accept();

        inline int willDialRemote(int val=1){_dialRemote=val; return _dialRemote;}
        inline in_port_t getLocalPort(void) const
        {
            struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrLocal;
            return ntohs(sockaddr_in.sin_port);
        }

        inline in_port_t getRemotePort(void) const
        {
        struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrRemote;
        return ntohs(sockaddr_in.sin_port);
        }

    		inline std::string getRemoteAddr4withPort() {
			return getAddr4(_sockaddrRemote) +":"+ boost::lexical_cast<std::string>(getRemotePort());
		}

                inline std::string getLocalAddr4withPort() {
			return getAddr4(_sockaddrLocal) +":"+ boost::lexical_cast<std::string>(getLocalPort());
		}
                

                virtual std::string getRemoteAddrIPv4() const final{
			return getAddr4(_sockaddrRemote);
		}
                virtual std::string getLocalAddrIPv4() const final{
			return getAddr4(_sockaddrLocal);
		}

                std::string getInfoString(){
                   return "CnxSockSSL=(L)"+getLocalAddr4withPort()+"/(R)"+ getRemoteAddr4withPort();
                }
        bool remoteAndLocalAddressNotSame(){
            return (getLocalAddrIPv4() != getRemoteAddrIPv4() );
        }
        
    inline void setNoDelay(bool value)
    {
        int val = value;//1 for on, 0 for off
        if (setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) != 0)
            {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,
                       errno,strerror(errno));
            }
        return;
    }

    int accept(txp::Connex* &pNewSock);

    void setLocalAddr(in_addr_t addr, uint16_t port=0);
    void setRemoteAddr(in_addr_t addr, uint16_t port);
    void loadCertificates(std::string certificate, std::string key);
    void verifyCertificates();
    void loadCertificateToTrustStore(std::string certfile);
    };//class end

} //namespace

#endif //CNX_CnxSockSSL_H
