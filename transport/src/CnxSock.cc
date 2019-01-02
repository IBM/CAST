/*******************************************************************************
 |    CnxSock.cc
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
#include "CnxSock.h"

#include "txp_flightlog.h"

#ifdef HAVE_GETPEERUCRED
#include <ucred.h>
#endif

using namespace txp;

CnxSock::CnxSock(int sockFamily, int sockType, const std::string& pConnectionName, int protocol):
txp::CnxSock(sockFamily,sockType,protocol)
{
    _connectionName=pConnectionName;
}

CnxSock::CnxSock(int sockFamily, int sockType, int protocol) :
	txp::ConnexSocket() {
	_family = sockFamily;
	_type= sockType;
	_protocol=protocol;
	memset(&_sockaddrLocal,0,sizeof(_sockaddrLocal));
	memset(&_sockaddrRemote,0,sizeof(_sockaddrRemote));
	_sockaddrLocal.sa_family=sockFamily;
	_sockaddrRemote.sa_family=sockFamily;
	_sockaddrlen = sizeof(struct sockaddr_in);
};

CnxSock::CnxSock(const CnxSock &pCnxSock) :
	txp::ConnexSocket() {
	_family=pCnxSock._family;
	_type=pCnxSock._type;
	_protocol=pCnxSock._protocol;
	_sockfd=pCnxSock._sockfd;
	_flagsOnOpen=pCnxSock._flagsOnOpen;
	_dialRemote=pCnxSock._dialRemote;
	_useMonitor=pCnxSock._useMonitor;
	_rcLast=pCnxSock._rcLast;
    _memChunkReadPtr=NULL;
    _numChunks = 64;
    _chunkSizeRead = 2* 64 * 1024;
	memcpy(&_sockaddrLocal, (void*)&(pCnxSock._sockaddrLocal), sizeof(struct sockaddr));
	memcpy(&_sockaddrRemote, (void*)&(pCnxSock._sockaddrRemote), sizeof(struct sockaddr));
	_sockaddrlen=pCnxSock._sockaddrlen;
	_keepAliveIntvl=pCnxSock._keepAliveIntvl;
    _keepAliveIdle=pCnxSock._keepAliveIdle;
    _keepAliveCount=pCnxSock._keepAliveCount;
    _turnOnKeepAlive=pCnxSock._turnOnKeepAlive;
};

int CnxSock::accept() {
	socklen_t lSockaddrlen;
	while (1) {
		_rcLast = accept4(_sockfd,&_sockaddrRemote,&lSockaddrlen, SOCK_CLOEXEC);
		if (_rcLast<0) {
			if (errno == EINTR) continue;
			LOG(txp,warning)<< __PRETTY_FUNCTION__<<"_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
		}
		break;
	}
	if (_rcLast>0) {
		closefd();  //close listening CnxSock
		_sockfd=_rcLast;
		int RCgetpeername=getpeername(_sockfd, &_sockaddrRemote, &lSockaddrlen);
        if (RCgetpeername){
            LOG(txp,always)<<"CnxSock::accept() getpeername errno="<<errno<<", "<<strerror(errno);
        }
		LOG(txp,always)<<"CnxSock::accept() "<<getInfoString()<<" sockfd="<<_sockfd;
		keepAlive();
		errno=0;
	}

	return _rcLast;
}

int CnxSock::accept(txp::Connex* &pNewSock) {
	int l_RC = 0;
    pNewSock = NULL;
	if (!_dialRemote) {
		CnxSock* l_NewSock = new CnxSock(*this);
		socklen_t l_Sockaddrlen = sizeof(l_NewSock->_sockaddrRemote);
		while (1) {
			l_NewSock->_rcLast = accept4(l_NewSock->_sockfd, &(l_NewSock->_sockaddrRemote), &l_Sockaddrlen, SOCK_CLOEXEC);
			if (l_NewSock->_rcLast<0) {
				if (errno == EINTR) continue;
				LOG(txp,warning)<< __PRETTY_FUNCTION__<<"l_NewSock->_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
			}
			break;
		}
		l_RC = l_NewSock->_rcLast;
		if (l_RC > 0) {
			l_NewSock->_sockfd = l_NewSock->_rcLast;
			l_NewSock->keepAlive();
			int RCgetpeername = getpeername(l_NewSock->_sockfd, &(l_NewSock->_sockaddrRemote), &l_Sockaddrlen);
            if (RCgetpeername){//error to log
                LOG(txp,always)<<"CnxSock::accept(p) "<<"getpeername errno="<<errno<<", "<<strerror(errno);
            }
            int RCgetsockname = getsockname(l_NewSock->_sockfd, &(l_NewSock->_sockaddrLocal), &l_Sockaddrlen);
            if (RCgetsockname) {
                LOG(txp,always)<<"CnxSock::accept(p) "<<"getsockname errno="<<errno<<", "<<strerror(errno);
            }
			pNewSock = l_NewSock;
            LOG(txp,always)<<"CnxSock::accept(p) "<<l_NewSock->getInfoString()<<" sockfd="<<l_NewSock->_sockfd;
			errno = 0;
		}
        else {
            delete l_NewSock;
        }
        
	} else {
		l_RC = -2;
	}

	return l_RC;
}

int CnxSock::acceptRemote() {
	if (accept()==-1) return -errno;
        setNoDelay(true);
        
	return 0;
}

int CnxSock::bindCnxSock() {
	_rcLast = bind(_sockfd,&_sockaddrLocal,_sockaddrlen);
        LOG(txp,always)<< "bind fd="<<_sockfd;
	if (_rcLast<0) {
		LOG(txp,warning)<< __PRETTY_FUNCTION__<<"_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
	}

	return _rcLast;
}

int CnxSock::bindLocalPort() {
	if (_sockfd==-1) {
		if (openCnxSock()< 0) return -errno;
	}
	if (bindCnxSock()==-1) return -errno;
    if (_sockfd>0)
    {
        _sockaddrlen=sizeof(_sockaddrLocal);
        int RCgetsockname = getsockname(_sockfd, &_sockaddrLocal, &_sockaddrlen);
        if (RCgetsockname) {
            LOG(txp,warning)<<__PRETTY_FUNCTION__<< " getsockname errno="<<errno<<", "<<strerror(errno);
        }
    }

	return _sockfd;
}

int CnxSock::bindAndListenLocalPort() {
	int l_RC = 0;

	l_RC = bindLocalPort();
	if (l_RC >= 0) {
		if (listen4remote()) {
			l_RC = -errno;
		}
	}

	return l_RC;
}

int CnxSock::connect2Remote(){
	_sockaddrlen=sizeof(_sockaddrRemote);
        LOG(txp,info)<<"doing connect local="<<getLocalAddrIPv4()<<":"<<getLocalPort()<<" remote="<<getRemoteAddrIPv4()<<":"<<getRemotePort();
	_rcLast = connect(_sockfd, &_sockaddrRemote,_sockaddrlen);
	if (_rcLast<0) {
		LOG(txp,warning)<< __PRETTY_FUNCTION__<<"_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
	} else {
		_sockaddrlen=sizeof(_sockaddrLocal);
		int RCgetsockname = getsockname(_sockfd, &_sockaddrLocal, &_sockaddrlen);
        if (RCgetsockname) {
            LOG(txp,warning)<<__PRETTY_FUNCTION__<< " getsockname errno="<<errno<<", "<<strerror(errno);
        }
        int RCgetpeername = getpeername(_sockfd, &_sockaddrRemote, &_sockaddrlen);
        if (RCgetpeername){
            LOG(txp,always)<<"CnxSock::connect2Remote getpeername errno="<<errno<<", "<<strerror(errno);
        }
		LOG(txp,always)<<"CnxSock::connect2Remote "<< getInfoString()<<" sockfd="<<_sockfd;
	}

	return _rcLast;
}

void CnxSock::setLocalAddr(in_addr_t addr, uint16_t port) {
	struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrLocal;
	sockaddr_in.sin_family = _family;
	memcpy(&sockaddr_in.sin_addr.s_addr, &addr, sizeof(in_addr_t));
	sockaddr_in.sin_port = htons(port);
}

void CnxSock::setRemoteAddr(in_addr_t addr, uint16_t port) {
	struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_sockaddrRemote;
	sockaddr_in.sin_family = _family;
	memcpy(&sockaddr_in.sin_addr.s_addr, &addr, sizeof(in_addr_t));
	sockaddr_in.sin_port = htons(port);
}
