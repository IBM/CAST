/*******************************************************************************
 |    CnxSockUnix.h
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


//! \file  CnxSockUnix.h
//! \brief Connection objects include for setting up and using communications

#ifndef CNX_CnxSockUnix_H
#define CNX_CnxSockUnix_H

// Includes

#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "Connex.h"

//! Transport component uses txp namespace
namespace txp {

	typedef struct credentials {
		uid_t uid_read;
		pid_t pid_read;
		gid_t gid_read;
	} credentials_t;


	/**********************************/

	//! \class CnxSockUnix
	//! \brief Connection object unix socket class

	class CnxSockUnix: public ConnexSocket
	{
	private:
		struct sockaddr_un _localAddr;
                std::string _sockPath;
		unsigned int _pathLength;
                credentials_t _user_cred;
	protected:

	public:
		CnxSockUnix();
		/**
		 * \brief Constructor to setup base values for <b>Unix Socket</b>
		 * \param[in] sockFamily which should be PF_UNIX
		 * \param[in] sockType
		 * \param[in] protocol which should be 0
		 * \return object
		 * \post data squirreled away
		 */
		CnxSockUnix(int sockFamily, int sockType, int protocol=0);
		CnxSockUnix(const CnxSockUnix &pCnxSockUnix);
		~CnxSockUnix(){}

		virtual int accept();
		virtual int bindCnxSock();
		virtual int connect2Remote();

		virtual inline in_port_t getLocalPort(void) const {
			struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_localAddr;
			return ntohs(sockaddr_in.sin_port);
		}

                std::string getInfoString(){
                   return "CnxSockUnix="+_sockPath;
                }

		int accept(txp::Connex* &pNewSock);
		void setAddr(std::string path);
		void getAddr(char * path, int * pathsize);
		void setCred();
                virtual uid_t getUser_ID(){return _user_cred.uid_read;}
                virtual gid_t getGrp_ID(){return _user_cred.gid_read;}

	};//class end

}   // namespace
#endif //CNX_CnxSockUnix_H
