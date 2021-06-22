/*******************************************************************************
 |    CnxSock.h
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

#ifndef CNX_CnxSock_H
#define CNX_CnxSock_H

// Includes

#include <sys/un.h>

#include "Connex.h"
#include "RDMACMfship.h"

//! Transport component uses txp namespace
namespace txp {


	//! \class CnxSock
	//! \brief Connection object socket class
	//! \note  Currently supports only IPv4
	class CnxSock: public ConnexSocket
	{
	private:
		struct sockaddr _sockaddrLocal;
		struct sockaddr _sockaddrRemote;
		socklen_t _sockaddrlen;

	public:
		/**
		 * \brief Constructor to setup base values for <b>ipv4</b>
		 * \param[in] sockFamily which should be AF_INET
		 * \param[in] sockType
		 * \param[in] protocol which should be 0
		 * \return object
		 * \post data squirreled away
		 */
		CnxSock(int sockFamily, int sockType, int protocol=0);
        CnxSock(int sockFamily, int sockType, const std::string& pConnectionName, int protocol=0);
		CnxSock(const CnxSock &pCnxSock);
		CnxSock(const RDMACMfship &pRDMACMcoral);
		~CnxSock(){}

		virtual int accept();
		virtual int bindCnxSock();
		virtual int connect2Remote();

		virtual inline in_port_t getLocalPort(void) const final {
			struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrLocal;
			return ntohs(sockaddr_in.sin_port);
		}

		virtual void setLocalAddr(in_addr_t addr, uint16_t port=0);

		virtual void setRemoteAddr(in_addr_t addr, uint16_t port);
#if 0
struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};
#endif
		inline std::string getRemoteAddr4withPort() {
			return getAddr4(_sockaddrRemote) +":"+ boost::lexical_cast<std::string>(getRemotePort());
		}

                inline std::string getLocalAddr4withPort() {
			return getAddr4(_sockaddrLocal) +":"+ boost::lexical_cast<std::string>(getLocalPort());
		}
                

		virtual inline in_port_t getRemotePort(void) const final{
			struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrRemote;
			return ntohs(sockaddr_in.sin_port);
		}
                virtual std::string getRemoteAddrIPv4() const final{
			return getAddr4(_sockaddrRemote);
		}
                virtual std::string getLocalAddrIPv4() const final{
			return getAddr4(_sockaddrLocal);
		}

                std::string getInfoString(){
                   return "CnxSock=(L)"+getLocalAddr4withPort()+"/(R)"+ getRemoteAddr4withPort();
                }

		inline void setNoDelay(int value) {
			int val = value;//1 for on, 0 for off
			if (setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) != 0) {
				printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
			}
			return;
		}



		inline void setRemoteAddr(struct sockaddr_in* pRemoteAddr) {
			memcpy(&_sockaddrRemote, (void*)&(pRemoteAddr), sizeof(struct sockaddr));
			return;
		}

		inline void setReuseAddr(int val) {
			if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
				printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
			}
			return;
		}

		inline void setReusePort(int val) {
			if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != 0) {
				printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
			}
			return;
		}

		int accept(txp::Connex* &pNewSock);
		int acceptRemote();
		int bindLocalPort();
		int bindAndListenLocalPort();

		//! \todo SO_LINGER setting in seconds, on/off http://stackoverflow.com/questions/3757289/tcp-option-so-linger-zero-when-its-required
	protected:

	};//class end

}   // namespace
#endif //CNX_CnxSock_H
