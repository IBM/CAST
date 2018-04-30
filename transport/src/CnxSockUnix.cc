/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/*                                                                  */
/* (C) Copyright IBM Corp.  2015                                    */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* --License--                                                      */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */


// Includes
#include "CnxSockUnix.h"

#include "txp_flightlog.h"

#ifdef HAVE_GETPEERUCRED
#include <ucred.h>
#endif

using namespace txp;

CnxSockUnix::CnxSockUnix(int sockFamily, int sockType, int protocol) :
    txp::ConnexSocket(){
    _family=AF_UNIX;
    _type=sockType;
    _protocol=protocol;
    _sockfd=-1;
    _pathLength=0;
    _flagsOnOpen=O_CLOEXEC;
    memset(&_localAddr,0,sizeof(_localAddr));
    _localAddr.sun_family=AF_UNIX;
    _dialRemote=0; //assume listening
    _rcLast=0;
};


CnxSockUnix::CnxSockUnix(const CnxSockUnix &pCnxSock) :
    txp::ConnexSocket(){
    _family=pCnxSock._family;
    _type=pCnxSock._type;
    _protocol=pCnxSock._protocol;
    _sockfd=pCnxSock._sockfd;
    _flagsOnOpen=pCnxSock._flagsOnOpen;
    _dialRemote=1;
    _sockPath =  std::string(pCnxSock._sockPath);
    _pathLength=pCnxSock._pathLength;
    _rcLast=pCnxSock._rcLast ;
    memcpy(&_localAddr, (void*)&(pCnxSock._localAddr), sizeof(struct sockaddr_un));
};

CnxSockUnix::CnxSockUnix() :
    txp::ConnexSocket(){
    _family=PF_UNIX;
    _type=SOCK_STREAM;
    _protocol=0;
    _sockfd=-1;
    _pathLength=0;
    _flagsOnOpen=O_CLOEXEC;
    memset(&_localAddr,0,sizeof(_localAddr));
    _localAddr.sun_family=AF_UNIX;
    _dialRemote=0; //assume listening
    _rcLast=0;
};



int CnxSockUnix::accept(){
    int client_fd;
    struct sockaddr addr;
    socklen_t addrlen;

    addrlen = sizeof (addr);
    client_fd = accept4(_sockfd, &addr, &addrlen,SOCK_CLOEXEC);
    if (client_fd < 0)
        {
            perror("Could not accept connection\n");
            return -1;
        }
    LOG(txp,always)<<"CnxSockUnix::accept() "<<getInfoString()<<" sockfd="<<_sockfd;
    return client_fd;

}



int CnxSockUnix::accept(txp::Connex* &pNewSock) {
    int l_RC = 0;

    if (!_dialRemote) {
        CnxSockUnix* l_NewSock = new CnxSockUnix(*this);
        socklen_t l_Sockaddrlen = sizeof(struct sockaddr_un);
        l_NewSock->_rcLast = accept4(l_NewSock->_sockfd, (struct sockaddr*) &(l_NewSock->_localAddr), &l_Sockaddrlen, SOCK_CLOEXEC);
        l_RC = l_NewSock->_rcLast;
        if (l_RC < 0) {
            LOG(txp,warning)<< __PRETTY_FUNCTION__<<" l_RC="<< l_RC<< " errno="<<errno<<", "<<strerror(errno);
            delete l_NewSock;
            l_NewSock = 0;
        } else {
            l_NewSock->_sockfd = l_NewSock->_rcLast;
            pNewSock = l_NewSock;
            LOG(txp,always)<<"CnxSockUnix::accept(p) "<<l_NewSock->getInfoString()<<" sockfd="<<l_NewSock->_sockfd;
            errno = 0;
        }
    } else {
        l_RC = -2;
    }

    return l_RC;
}

int CnxSockUnix::bindCnxSock(){
    if (_pathLength == 0){
        LOG(txp,warning)<< __PRETTY_FUNCTION__<<"Binding failed: no unix path specified for AF_UNIX socket";
    }

    int val = 1;
    if (setsockopt(_sockfd, SOL_SOCKET, SO_PASSCRED, &val, sizeof (val)) < 0)
        {
            LOG(txp,warning)<< __PRETTY_FUNCTION__<<"Setting SO_PASSCRED failed, errno="<<errno<<", "<<strerror(errno);
        }

    _rcLast = bind(_sockfd, (struct sockaddr*) &_localAddr, offsetof(struct sockaddr_un, sun_path) + _pathLength);
    if (_rcLast<0) {
        LOG(txp,warning)<< __PRETTY_FUNCTION__<<"_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
    }
    else{
        LOG(txp,always)<<"CnxSockUnix::accept() "<<getInfoString()<<" sockfd="<<_sockfd;
    }

    return _rcLast;
}

int CnxSockUnix::connect2Remote()
{
    if (_pathLength == 0)
        {
            LOG(txp,warning)<< __PRETTY_FUNCTION__<<"Cannot connect if remote address unknown";
            return -1;
        }
    _rcLast = connect(_sockfd, (struct sockaddr *)&_localAddr, offsetof(struct sockaddr_un, sun_path) + _pathLength);
    if (_rcLast<0) {
        LOG(txp,warning)<< __PRETTY_FUNCTION__<<"_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
    } else {
        LOG(txp,info)<<"Connected through unix socket using path "<< _sockPath;
    }

    return _rcLast;

}

void CnxSockUnix::getAddr(char * path, int * pathsize)
{
    strncpy(path, _sockPath.c_str(), _pathLength );
    *pathsize = _pathLength;
}

void CnxSockUnix::setCred()
{
    //credentials_t user_cred;

#ifdef SO_PEERCRED
#ifdef __OpenBSD__
    struct sockpeercred cr;
#else
    struct ucred cr;
#endif
    int cr_len = sizeof (cr);
    if (getsockopt (_sockfd, SOL_SOCKET, SO_PEERCRED, &cr, (socklen_t *)&cr_len) == 0 &&
        cr_len == sizeof (cr))
        {
            _user_cred.pid_read = cr.pid;
            _user_cred.uid_read = cr.uid;
            _user_cred.gid_read = cr.gid;
        }
    else
        {
            LOG(txp,error)<< __PRETTY_FUNCTION__<<"Could not retrieve user credentials, errno="<<errno<<", "<<strerror(errno);
        }
#elif defined(HAVE_GETPEEREID)
    uid_t euid;
    gid_t egid;
    //    printf("Strange to be in HAVE_GETPEEREID\n");
    if (getpeereid (_sockfd, &euid, &egid) == 0)
        {
            _user_cred.uid_read = euid;
            _user_cred.gid_read = egid;
        }
    else
        {
            LOG(txp,error)<< __PRETTY_FUNCTION__<<"Could not retrieve user credentials, errno="<<errno<<", "<<strerror(errno);
        }
#elif defined(HAVE_GETPEERUCRED)
    ucred_t * ucred = NULL;
    //    printf("using ucred as a backup\n");
    if (getpeerucred (_sockfd, &ucred) == 0)
        {
            _user_cred.pid_read = ucred_getpid(ucred);
            _user_cred.uid_read = ucred_geteuid(ucred);
            _user_cred.gid_read = ucred_getegid(ucred);
        }
    else
        {
            LOG(txp,error)<< __PRETTY_FUNCTION__<<"Could not retrieve user credentials, errno="<<errno<<", "<<strerror(errno);
        }
    if (ucred != NULL)
        ucred_free (ucred);
#else /* !SO_PEERCRED && !HAVE_CMSGCRED && !HAVE_GETPEEREID && !HAVE_GETPEERUCRED */
    LOG(txp,error)<< __PRETTY_FUNCTION__<<"Could not retrieve user credentials, NOT SUPPORTED ON THIS OS";
    printf ("Socket credentials not supported on this OS\n");
#endif

    LOG(txp,info) << "Credentials: pid= " <<_user_cred.pid_read << ", uid= " <<  _user_cred.uid_read << ", gid= "<< _user_cred.gid_read;

    return;
}



void CnxSockUnix::setAddr(std::string path)
{
    _sockPath=path;
    _pathLength = _sockPath.length();
    strncpy(_localAddr.sun_path, _sockPath.c_str(), _pathLength);
    return;
}
