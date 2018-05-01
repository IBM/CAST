/*******************************************************************************
|    bbproxyConn2bbserver.cc
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
#include <iostream>
#include "bbproxyConn2bbserver.h"
#include <exception>

using namespace std;
using namespace txp;

static bool SSLready=false;
static void initSSLenvironment(){
    if (!SSLready){
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSLready=true;
    }
}


txp::Connex*  makeConnection2remoteSSL(const std::string& pName,std::string pIPinfo ){
    int rc = 0;
    in_addr_t l_ipremote;
    uint16_t l_port;
    std::string l_ipaddr;
    initSSLenvironment();
    txp::CnxSockSSL* sock=NULL;
    sock = new txp::CnxSockSSL(AF_INET,SOCK_STREAM,pName);
    sock->setConnectName(pName);
    try{
        rc = getIPPort(pIPinfo, l_ipaddr, l_port);
        if (rc) {
            LOG_RC_AND_BAIL(rc);
        }
        if (l_ipaddr=="0.0.0.0")l_ipaddr="127.0.0.1";
        rc = inet_pton(AF_INET, l_ipaddr.c_str(), &l_ipremote);
        if (rc!=1) {
            if (rc<0)LOG_RC_AND_BAIL(errno);
            LOG_RC_AND_BAIL(EINVAL);
        }
        sock->willDialRemote();
        sock->loadCertificateToTrustStore(config.get(pName + ".sslcertif", NO_CONFIG_VALUE));
        sock->setRemoteAddr(l_ipremote, l_port); //<<<<<<<<<<<<
        int retryConnectDelay = config.get(pName + ".retry_connect_delay", 0); //in seconds
        int retryConnectCount = config.get(pName + ".retry_connect_count", 0);
        if (sock->attachRemote(retryConnectDelay,retryConnectCount) <= 0)
        {
            LOG(bb,error) << "SSL failure, sock->attachRemote(), errno = " << errno << ", (" << strerror(errno) << ")";
            LOG_RC_AND_BAIL(errno);
        }
        else sock->verifyCertificates();
        
    }
    catch(ExceptionBailout& e)
    {
        //log error message--RAS??
        LOG(bb,always)<<"catch(ExceptionBailout& e)"<<" @="<<__FILE__<<":"<< __FUNCTION__<<":"<<__LINE__;
        if (sock) delete sock;
        sock=NULL;
        stringstream errorText;
        errorText << "SSL failure in attempting to connect to "<<pName;
        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.net.SSLconnectAttemptFailed);
    }
    return sock;
}

txp::Connex*  makeConnection2remoteNonSSL(const std::string& pName,std::string pIPinfo ){
    int rc = 0;
    in_addr_t l_ipremote;
    uint16_t l_port=0;
    std::string l_ipaddr;
    txp::CnxSock* sock=NULL;
    sock = new txp::CnxSock(AF_INET,SOCK_STREAM,pName);
    sock->setConnectName(pName);
    try{
        rc = getIPPort(pIPinfo, l_ipaddr, l_port);
        if (rc) {
            if (rc) LOG_RC_AND_BAIL(rc);
        }
        if (l_ipaddr=="0.0.0.0")l_ipaddr="127.0.0.1";
        rc = inet_pton(AF_INET, l_ipaddr.c_str(), &l_ipremote);
        if (rc!=1) {
            if (rc<0)LOG_RC_AND_BAIL(errno);
            LOG_RC_AND_BAIL(EINVAL);
        }
        sock->willDialRemote();
        sock->setRemoteAddr(l_ipremote, l_port);
        int retryConnectDelay = config.get(pName + ".retry_connect_delay", 0); //in seconds
        int retryConnectCount = config.get(pName + ".retry_connect_count", 0);
        if (sock->attachRemote(retryConnectDelay,retryConnectCount) <= 0)
        {
            LOG(bb,error) << "nonSSL failure, sock->attachRemote(), errno = " << errno << ", (" << strerror(errno) << ")";
            LOG_RC_AND_BAIL(errno);
        }
    }
    catch(ExceptionBailout& e)
    {
        //log error message--RAS??
        LOG(bb,always)<<"catch(ExceptionBailout& e)"<<" @="<<__FILE__<<":"<< __FUNCTION__<<":"<<__LINE__;
        if (sock) delete sock;
        sock=NULL;
        stringstream errorText;
        errorText << "nonSSL failure in attempting to connect to "<<pName;
        LOG_ERROR_TEXT_ERRNO_AND_RAS(errorText, errno, bb.net.nonSSLconnectAttemptFailed);
        
    }
    return sock;
}


txp::Connex* createConnection2bbserver(const std::string& pName){
    std::string sslIPinfo = config.get(pName + ".ssladdress", NO_CONFIG_VALUE);
    if(sslIPinfo != NO_CONFIG_VALUE)    //ssl connection provided
    {
        return  makeConnection2remoteSSL(pName,sslIPinfo );
    }
    std::string IPinfo = config.get(pName + ".address", NO_CONFIG_VALUE);
    if(IPinfo != NO_CONFIG_VALUE)    //ssl connection provided
    {
        return  makeConnection2remoteNonSSL(pName,IPinfo );
    }
    return NULL;
}
