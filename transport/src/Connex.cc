/*******************************************************************************
 |    Connex.cc
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
#include "Connex.h"

#include "txp_flightlog.h"

using namespace txp;


ConnexSocket::~ConnexSocket() {
    if (_memChunkReadPtr) {
        MemChunk* l_Temp = _memChunkReadPtr;
        _memChunkReadPtr = 0;
        delete l_Temp;
    }
    closefd();
}


void Connex::freeReadChunk(memItemPtr& mip) {
    delete mip;
        mip=NULL;
    return;
}

memItemPtr Connex::getReadChunk() {
    memItemPtr mip = new ::memItem(NULL,0,2*1024*1024);
    mip->address = new char[mip->chunkSize];
    return mip;
}

memItemPtr Connex::getSendChunk(int sizeRequest) {
   memItemPtr mip = new ::memItem(NULL,0,2*1024*1024);
   mip->address = new char[mip->chunkSize];
   return mip;
}

void Connex::freeSendChunk(memItemPtr& mip) {
    delete mip;
        mip=NULL;
    return;
}


/*  NOTE: Returns     0 - read lock successfully acquired
 *                    1 - acquiring lock was not necessary
 *               -errno - all other errors
 */
int Connex::readLock() {
    int l_RC = -1;

    if (!_lastReadHadNextMsg) {
        l_RC = pthread_mutex_lock(&_readLock);
    }

    if (l_RC <= 0) {
        if (!l_RC) {
            LOG(txp,debug) << "  TXP_RD:   LOCK, Read for message header...";
        } else {
            LOG(txp,debug) << "  TXP_RD:  READ+, Read for another message header...";
        }
    } else {
        LOG(txp,error) << "  TXP_RD:  ERROR, RC=" << l_RC << ", errno=" << errno;
    }

    return -l_RC;
}

/*  NOTE: Returns     0 - read lock successfully released
 *                    1 - unlock was not necessary
 *               -errno - all other errors
 */
int Connex::readUnlock(txp::Msg* pMsg) {
    int l_RC = -1;

    if ((pMsg == 0) || (!pMsg->getNextMsgNumber())) {
        _lastReadHadNextMsg = 0;
        l_RC = pthread_mutex_unlock(&_readLock);
    } else {
        _lastReadHadNextMsg = 1;
    }

    return -l_RC;
}



void Connex::sentSendChunk(memItemPtr& mip) {
    if (mip->address) delete[] mip->address;
    delete mip;
    mip=NULL;
    return;
}

/*  NOTE: Returns     0 - write lock successfully acquired
 *                    1 - acquiring lock was not necessary
 *               -errno - all other errors
 */
int Connex::writeLock(txp::Msg* pMsg) {
    int l_RC = -1;
    char l_MsgId[64] = {'\0'};

    if (pMsg) {
        pMsg->msgIdToChar(pMsg->getMsgId(), l_MsgId, sizeof(l_MsgId));
    }
    if ((pMsg == 0) || (!pMsg->getPrevMsgNumber())) {
        l_RC = pthread_mutex_lock(&_writeLock);
    }

    if (l_RC <= 0) {
        if (!l_RC) {
            // Lock was obtained...
            LOG(txp,debug) << " TXP_WRT:   LOCK, " << l_MsgId << ", Msg#=" << (pMsg ? pMsg->getMsgNumber() : -1) << ", RMsg#=" << (pMsg ? pMsg->getRequestMsgNumber() : -1) << ", #Attrs=" << (pMsg ? pMsg->getNumberOfAttributes() : -1) << ", RC=" << l_RC << ", errno=" << errno;
        } else {
            // Lock is already held...
            LOG(txp,debug) << " TXP_WRT: WRITE+, " << l_MsgId << ", Msg#=" << (pMsg ? pMsg->getMsgNumber() : -1) << ", RMsg#=" << (pMsg ? pMsg->getRequestMsgNumber() : -1) << ", #Attrs=" << (pMsg ? pMsg->getNumberOfAttributes() : -1) << ", RC=" << l_RC << ", errno=" << errno;
        }
    } else {
        // Error received during lock...  -errno is returned...
        LOG(txp,error) << " TXP_WRT: LCKERR, " << l_MsgId << ", Msg#=" << (pMsg ? pMsg->getMsgNumber() : -1) << ", RMsg#=" << (pMsg ? pMsg->getRequestMsgNumber() : -1) << ", #Attrs=" << (pMsg ? pMsg->getNumberOfAttributes() : -1) << ", RC=" << l_RC << ", errno=" << errno;
    }

    return -l_RC;
}

/*  NOTE: Returns     0 - write lock successfully released
 *                    1 - unlock was not necessary
 *               -errno - all other errors
 */
int Connex::writeUnlock(txp::Msg* pMsg) {
    int l_RC = -1;

    if ((pMsg == 0) || (!pMsg->getNextMsgNumber())) {
        l_RC = pthread_mutex_unlock(&_writeLock);
    }

    return -l_RC;
}


void ConnexSocket::freeReadChunk(memItemPtr& mip) {
    _memChunkReadPtr->putChunk(mip);

    return;
}

ssize_t ConnexSocket::getDataBufferToRead(char * pBuffer,size_t pBufflength,uint64_t pkey) {
    ssize_t l_TotalRead = 0;
    ssize_t l_NumBytes =  0;
    while ((size_t)l_TotalRead  < pBufflength) {
        l_NumBytes = ::read(_sockfd,pBuffer+l_TotalRead ,pBufflength-l_TotalRead);
        if (l_NumBytes==-1) {
            if (errno==EINTR)continue;
            if (errno==EAGAIN)continue;//if O_NONBLOCK, spin if any data was initially sent
            return -1;
        }
        l_TotalRead += l_NumBytes;
    }

    // NOTE:  The following design relies on a single 'buffer' of data
    //        to be read between two linked messages.  If in the future
    //        this ceases to be true, the following readInMessage()
    //        logic will need to be altered.  @@DLH

    // We just read in the buffer of data between the two linked
    // messages.  Now read the next message, which is verified to be
    // a CORAL_NO_OP message, that will release the read lock.  We want
    // the read lock to be released now so that the next read can overlap
    // the processing of this message and associated buffer.  It also
    // makes it possible for the processing of this message to
    // write and receive another message.
    int l_RC = readInMessage();
//  printf("getDataBufferToRead(): l_TotalRead = %zu, l_RC = %d\n", l_TotalRead, l_RC);

    return (!l_RC ? l_TotalRead : -1);
}

memItemPtr ConnexSocket::getReadChunk() {
    if (!_memChunkReadPtr) _memChunkReadPtr = new MemChunk(_numChunks, _chunkSizeRead);
    return _memChunkReadPtr->getChunk();
}

int ConnexSocket::openCnxSock() {
    if (_sockfd>0) {errno=EALREADY; return -1;}
    //flags on open: SOCK_CLOEXEC, e.g. SOCK_STREAM | SOCK_CLOEXEC
    _sockfd = socket(_family,_type|_flagsOnOpen,_protocol);
    if ( ( _sockfd>0 )&&(_family == PF_UNIX) )
        {
            int val = 1;
            if (setsockopt(_sockfd, SOL_SOCKET, SO_PASSCRED, &val, sizeof (val)) < 0)
                {
                    LOG(txp,warning)<< __PRETTY_FUNCTION__<<"setting SO_PASSCRED failed, errno="<<errno<<", "<<strerror(errno);
                }
        }
    return _sockfd;
}

int ConnexSocket::poll4DataIn() {
    if (-1==_sockfd){
        errno=ENOTCONN;
        return -1;
    }
    const int numFds=1;
    const int timeout = -1;//negative for infinite timeout
    struct pollfd pollInfo;
    pollInfo.fd=_sockfd;
    pollInfo.events=POLLIN;
    int rc=0;

    while (rc==0){
        pollInfo.revents=0;
        rc = ::poll(&pollInfo, numFds, timeout);
        if (rc==0) continue;
        if (rc==(-1)){//errno came in
            if(errno==EINTR)continue; //ignore an interrupt
            LOGERRNO(txp,error,errno);//syslog that something bad happened
            abort();
        }
        if (pollInfo.revents & POLLIN) {
            return 1; //indicates data to be read;
        }
        if (pollInfo.revents & (POLLERR|POLLHUP|POLLNVAL) ) {//error!
            closefd();
            errno=ECONNRESET;
            LOGERRNO(txp,error,errno);//syslog that something bad happened
            return -1;
        }
    }
    return rc;
}

    int ConnexSocket::keepAlive( )
    {  
        int val=0; //start by turning it off
        if (setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) != 0) {
             LOGERRNO(txp,error,errno);//syslog that something bad happened;     
        }

        if (_turnOnKeepAlive){//turn it on

            //override net.ipv4.tcp_keepalive_time
            if (setsockopt(_sockfd, SOL_TCP, TCP_KEEPIDLE, &_keepAliveIdle, sizeof(_keepAliveIdle) ) != 0){
                LOGERRNO(txp,error,errno);//syslog that something bad happened;     
            }
            //override net.ipv4.tcp_keepalive_intvl
            if (setsockopt(_sockfd, SOL_TCP, TCP_KEEPINTVL, &_keepAliveIntvl, sizeof(_keepAliveIntvl)) != 0){
                LOGERRNO(txp,error,errno);//syslog that something bad happened      
            }
            //override net.ipv4.tcp_keepalive_probes
            if (setsockopt(_sockfd, SOL_TCP, TCP_KEEPCNT, &_keepAliveCount, sizeof(_keepAliveCount)) != 0){
                LOGERRNO(txp,error,errno);//syslog that something bad happened
            }

            val=1; //on=1
            if (setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) != 0) {
               LOGERRNO(txp,error,errno);//syslog that something bad happened;     
            }
        }
    
      return 0;
    }


int ConnexSocket::read(txp::Msg* &oMsg, char* pDataBuffer, const size_t pDataBufferSize, const txp::DeserializeOption pDeserializeOption ) {
    int l_RC = 0;
    errno = 0;

    ssize_t l_TotalRead = 0, l_TotalMsgSize = 0;
    txp::Msg * l_MsgPtr = 0;
    const int HEADERLENGTH = LENGTH_OF_MSG_HEADER;

    l_RC = readLock();
    if (l_RC >= 0) {
        l_RC = 0;
        char l_Header[HEADERLENGTH]={'\0'};
        ssize_t l_NumBytes = ::read(_sockfd, l_Header, HEADERLENGTH);
        //        if (NULL == _cSSL)
        //   l_NumBytes = ::read(_sockfd, l_Header, HEADERLENGTH);
        //else
        //   l_NumBytes = SSL_read(_cSSL, l_Header, HEADERLENGTH);

        if (l_NumBytes >= 0) {
            l_TotalRead = l_NumBytes;

            while (l_TotalRead < HEADERLENGTH) {    //weird but possible
                // if (NULL == _cSSL)
                    //   l_NumBytes = ::read(_sockfd, l_Header+l_TotalRead, HEADERLENGTH-l_TotalRead );
                //else
                    //                    l_NumBytes = SSL_read(_cSSL, l_Header+l_TotalRead, HEADERLENGTH-l_TotalRead );
                l_NumBytes = ::read(_sockfd, l_Header+l_TotalRead, HEADERLENGTH-l_TotalRead );
                if (l_NumBytes == -1) {
                    if (errno == EINTR) continue;
                    if (errno == EAGAIN) continue;      //  if O_NONBLOCK, spin if any data was initially sent
                    LOG(txp,warning)<< __PRETTY_FUNCTION__<<"_rcLast"<< _rcLast<< " errno="<<errno<<", "<<strerror(errno);
                    l_RC = -errno;
                    break;
                }
                if (l_NumBytes == 0) {
                    int error=0;
                    socklen_t len = sizeof(error);
                    int retval = getsockopt(_sockfd, SOL_SOCKET, SO_ERROR, &error, &len );
                    if (retval) {
                        LOGERRNO(txp,error,errno);    //  syslog that something bad happened
                    }
                    if (error) {
                        LOGERRNO(txp,error,error);    //  syslog that something happened
                        closefd();
                        _rcLast = error;
                        errno = error;
                    } else {
                        errno=ENOTCONN;
                        LOG(txp,debug) << "File="<<__FILE__<<":"<<__LINE__<<" Connection to descriptor " << _sockfd << " closed (" << strerror(errno) << ")"<< " _connectionName="<<_connectionName;
                        closefd();
                    }
                    l_RC = -errno;
                    break;
                }
                l_TotalRead += l_NumBytes;
            }

            if (!l_RC) {
                l_RC = txp::Msg::deserializeHeaderToMsg(l_MsgPtr, l_Header, l_TotalMsgSize, pDataBuffer, pDataBufferSize);
                if (!l_RC) {
                    char l_MsgId[64] = {'\0'};
                    l_MsgPtr->msgIdToChar(l_MsgPtr->getMsgId(), l_MsgId, sizeof(l_MsgId));
                    LOG(txp,debug) << "  TXP_RD:  ATTRS, " << l_MsgId << ", Msg#=" << l_MsgPtr->getMsgNumber() << ", RMsg#=" << l_MsgPtr->getRequestMsgNumber() << ", l_TotalMsgSize=" << l_TotalMsgSize << ", padLength=" << l_MsgPtr->getLengthOfPad() << ", RC=" << l_RC << ", errno=" << errno;
                    while (l_TotalRead < l_TotalMsgSize) {
                        l_NumBytes = ::read(_sockfd, l_MsgPtr->getDataBufferPtr()+l_TotalRead, l_TotalMsgSize-l_TotalRead );
                        //if (NULL == _cSSL)
                        //                        l_NumBytes = ::read(_sockfd, l_MsgPtr->getDataBufferPtr()+l_TotalRead, l_TotalMsgSize-l_TotalRead );
                        //else
                        //   l_NumBytes = SSL_read(_cSSL, l_MsgPtr->getDataBufferPtr()+l_TotalRead, l_TotalMsgSize-l_TotalRead );
                        if (l_NumBytes == -1) {
                            if (errno == EINTR) continue;
                            if (errno == EAGAIN) continue;      //  if O_NONBLOCK, spin if any data was initially sent
                            l_RC = -errno;
                            break;
                        }
                        l_TotalRead += l_NumBytes;
                    }
                    if (!l_RC) {
                        l_RC = l_MsgPtr->deserializeAttrsToMsg(l_MsgPtr->getBufferPtr(), pDeserializeOption, l_TotalMsgSize);
                        if (l_RC){
                            LOG(txp,critical)<< __PRETTY_FUNCTION__<<":"<< __LINE__<< " l_RC="<<l_RC;
                            sleep(-1);
                        }
                    }
                } else {
                    LOG(txp,critical)<< __PRETTY_FUNCTION__<<":"<< __LINE__<< " l_RC="<<l_RC;
                    abort();
                }
            }
        } else {
            l_RC = -1;  // errno is not set
        }

        int l_RC2 = 0;
        if (!l_RC) {
            l_RC2 = readUnlock(l_MsgPtr);
            if (l_MsgPtr) {
                char l_MsgId[64] = {'\0'};
                l_MsgPtr->msgIdToChar(l_MsgPtr->getMsgId(), l_MsgId, sizeof(l_MsgId));
                if (l_RC2 >= 0) {
                    if (!l_RC2) {
                        //  Successful read, no next message...
                        LOG(txp,debug) << "  TXP_RD: UNLOCK, " << l_MsgId << ", Msg#=" << l_MsgPtr->getMsgNumber() << ", RMsg#=" << l_MsgPtr->getRequestMsgNumber() << ", #Attrs=" << l_MsgPtr->getNumberOfAttributes() << ", RC=" << l_RC << ", errno=" << errno;
                    } else {
                        //  Successful read, next message exists...
                        LOG(txp,debug) << "  TXP_RD:   DONE, " << l_MsgId << ", Msg#=" << l_MsgPtr->getMsgNumber() << ", RMsg#=" << l_MsgPtr->getRequestMsgNumber() << ", #Attrs=" << l_MsgPtr->getNumberOfAttributes() << ", RC=" << l_RC << ", errno=" << errno;
                    }
                } else {
                    //  Successful read, error on the unlock...
                    LOG(txp,error) << "  TXP_RD:  ERROR, " << l_MsgId << ", Msg#=" << l_MsgPtr->getMsgNumber() << ", RMsg#=" << l_MsgPtr->getRequestMsgNumber() << ", #Attrs=" << l_MsgPtr->getNumberOfAttributes() << ", RC=" << l_RC << ", RC2=" << l_RC2 << ", errno=" << errno;
                    l_RC = -3;
                }
            } else {
                //  Don't know if we can get into this leg of code...
                //  If a zero l_RC, then l_MsgPtr *should* be set...  @@DLH
                if (l_RC2 >= 0) {
                    if (!l_RC2) {
                        //  Unsuccessful read, successful unlock...
                        LOG(txp,debug) << "  TXP_RD: UNLOCK, RC=" << l_RC << ", errno=" << errno;
                    } else {
                        //  Unsuccessful read, remains locked...
                        // \todo - Not sure what to do here...  @@DLH
                        LOG(txp,error) << "  TXP_RD:  ERROR, RC=" << l_RC << ", errno=" << errno;
                        l_RC = -3;
                    }
                } else {
                    //  Unsuccessful read, unsuccessful unlock...
                    LOG(txp,error) << "  TXP_RD:  ERROR, RC=" << l_RC << ", RC2=" << l_RC2 << ", errno=" << errno;
                    l_RC = -3;
                }
            }
        } else {
            l_RC2 = readUnlock(0);  // Unconditional unlock
            if (!l_RC2) {
                //  Successful unlock...
                LOG(txp,debug) << "  TXP_RD:U_UNLCK, RC=" << l_RC << ", errno=" << errno;
            } else {
                //  Remains locked...
                // \todo - Not sure what to do here...  @@DLH
                LOG(txp,error) << "  TXP_RD:U_ERROR, RC=" << l_RC << ", RC2=" << l_RC2 << ", errno=" << errno;
            }
        }
    } else {
        errno = -l_RC;
    }

    FL_Write6(FLTxp, TXP_RD, "Msg#=0x%08X, RMsg#=0x%08X, MsgId=0x%08X, #Attrs=%d, MsgSize=%d, RC=%d", (l_MsgPtr ? l_MsgPtr->getMsgNumber() : 0), (l_MsgPtr ? l_MsgPtr->getRequestMsgNumber() : 0), (l_MsgPtr ? l_MsgPtr->getMsgId() : 0), (l_MsgPtr ? l_MsgPtr->getNumberOfAttributes() : 0), (l_MsgPtr ? l_TotalMsgSize : 0), l_RC);
    if (!l_RC) {
        oMsg = l_MsgPtr;
    } else {
        if (l_MsgPtr) {
            delete l_MsgPtr;
            l_MsgPtr = 0;
        }
    }

    return l_RC;
}

int ConnexSocket::readInMessage() {
    int l_RC = 0;

    txp::Msg * l_MsgPtr = 0;
    l_RC = read(l_MsgPtr);

    if (!l_RC) {
        if (l_MsgPtr->getMsgId() != CORAL_NO_OP) {
            l_RC = -1;
        }
    }

    if (l_MsgPtr) {
        delete l_MsgPtr;
        l_MsgPtr = 0;
    }

    return l_RC;
}

ssize_t ConnexSocket::write(txp::Msg* pMsg) {
    int l_RC = 0;
    ssize_t l_TotalWritten = 0;
    errno = 0;

    char l_MsgId[64] = {'\0'};
    pMsg->msgIdToChar(pMsg->getMsgId(), l_MsgId, sizeof(l_MsgId));

    pMsg->allocateHeapBuffer(pMsg->getMsgLengthWithDataValues() + 1024);
    int32_t l_DataLen = pMsg->serializeWithValuesToHeapBuffer();

    if (l_DataLen > 0) {
        l_RC = writeLock(pMsg);
        if (l_RC >= 0) {
            l_RC = 0;
            char * l_Data = pMsg->getDataBufferPtr();
            ssize_t l_NumBytes = ::write(_sockfd, l_Data, l_DataLen);
            //            if (NULL == _cSSL)
            //   l_NumBytes = ::write(_sockfd, l_Data, l_DataLen);
            //else
            //   l_NumBytes = SSL_write(_cSSL, l_Data, l_DataLen);

            if (l_NumBytes >= 0) {
                l_TotalWritten = l_NumBytes;
                if (l_NumBytes < l_DataLen) {
                    while ((!l_RC) && (l_TotalWritten < (unsigned)l_DataLen)) {
                        //                        if (NULL == _cSSL)
                        //   l_NumBytes = ::write(_sockfd, l_Data+l_TotalWritten, l_DataLen-l_TotalWritten);
                        // else
                        //   l_NumBytes = SSL_write(_cSSL, l_Data+l_TotalWritten, l_DataLen-l_TotalWritten);
                        l_NumBytes = ::write(_sockfd, l_Data+l_TotalWritten, l_DataLen-l_TotalWritten);
                        if (l_NumBytes == -1) {
                            if (EINTR==errno) continue;
                            if (EAGAIN==errno) continue;    //  if O_NONBLOCK, spin if any data was initially sent
                            else l_RC = -1;
                        }
                        l_TotalWritten += l_NumBytes;
                    }
                }
                if (!l_RC) {
                    pMsg->sent();
                }
            } else {
                l_RC = -errno;
                LOG(txp,debug) << " TXP_WRT:Error, " << l_MsgId << ", Msg#=" << pMsg->getMsgNumber() << ", RMsg#=" << pMsg->getRequestMsgNumber() << ", #Attrs=" << pMsg->getNumberOfAttributes() << ", RC=" << l_RC << ", errno=" << errno<<"("<<strerror(errno)<<")";
                  
            }

            int l_RC2 = 0;
            if (!l_RC) {
                l_RC2 = writeUnlock(pMsg);
                if (l_RC2 >= 0) {
                    if (!l_RC2) {
                        //  Successful write, no next message...
                        LOG(txp,debug) << " TXP_WRT: UNLOCK, " << l_MsgId << ", Msg#=" << pMsg->getMsgNumber() << ", RMsg#=" << pMsg->getRequestMsgNumber() << ", #Attrs=" << pMsg->getNumberOfAttributes() << ", RC=" << l_RC << ", errno=" << errno;
                    } else {
                        //  Successful write, next message exists...
                        LOG(txp,debug) << " TXP_WRT:   DONE, " << l_MsgId << ", Msg#=" << pMsg->getMsgNumber() << ", RMsg#=" << pMsg->getRequestMsgNumber() << ", #Attrs=" << pMsg->getNumberOfAttributes() << ", RC=" << l_RC << ", errno=" << errno;
                    }
                } else {
                    //  Successful write, error on the unlock...
                    LOG(txp,error) << " TXP_WRT:  ERROR, " << l_MsgId << ", Msg#=" << pMsg->getMsgNumber() << ", RMsg#=" << pMsg->getRequestMsgNumber() << ", #Attrs=" << pMsg->getNumberOfAttributes() << ", RC=" << l_RC << ", RC2=" << l_RC2 << ", errno=" << errno;
                    l_RC = -3;
                }
            } else {
                l_RC2 = writeUnlock(0); // Unconditional unlock
                if (!l_RC2) {
                    //  Successful unlock...
                    LOG(txp,debug) << " TXP_WRT:U_UNLCK, RC=" << l_RC << ", errno=" << errno;
                } else {
                    //  Remains locked...
                    // \todo - Not sure what to do here...  @@DLH
                    LOG(txp,error) << " TXP_WRT:U_ERROR, RC=" << l_RC << ", RC2=" << l_RC2 << ", errno=" << errno;
                }
            }
        } else {
            errno = -l_RC;
        }
    } else {
        errno=ENODATA;
        l_RC = -1;
    }

    FL_Write6(FLTxp, TXP_WRT, "Msg#=0x%08X, RMsg#=0x%08X, MsgId=0x%08X, #Attrs=%d, RC=%zu, errno=%d", pMsg->getMsgNumber(), pMsg->getRequestMsgNumber(), pMsg->getMsgId(), pMsg->getNumberOfAttributes(), l_RC, errno);

    return ((!l_RC) ? l_TotalWritten : (ssize_t)l_RC);
}

ssize_t ConnexSocket::writeWithBuffer(txp::Msg* pMsg, char * pBuffer, size_t pBufflength, uint64_t pkey) {
    int l_RC = 0;
    ssize_t l_NumBytes = 0;
    ssize_t l_TotalWritten = 0;
    errno = 0;

    if (!pMsg->getNextMsgNumber()) {
        //  Build the no-op message that will be sent after the raw data in pBuffer...
        txp::Msg* l_Msg = 0;
        txp::Msg::buildMsg(txp::CORAL_NO_OP, l_Msg);
        l_RC = pMsg->nextMsg(*l_Msg);
        if (!l_RC) {
            //  Write the message...
            l_NumBytes = write(pMsg);
            if (l_NumBytes >= 0) {
                l_TotalWritten = l_NumBytes;

                //  Now, write the buffer...
                ssize_t l_TotalBufferWritten = 0;
                while ((!l_RC) && (l_TotalBufferWritten < (unsigned)pBufflength)) {
                    //                    if (NULL == _cSSL)
                    //   l_NumBytes = ::write(_sockfd, pBuffer+l_TotalBufferWritten, pBufflength-l_TotalBufferWritten);
                    //else
                    //  l_NumBytes = SSL_write(_cSSL, pBuffer+l_TotalBufferWritten, pBufflength-l_TotalBufferWritten);
                    l_NumBytes = ::write(_sockfd, pBuffer+l_TotalBufferWritten, pBufflength-l_TotalBufferWritten);
                    if (l_NumBytes == -1) {
                        if (EINTR == errno) continue;
                        if (EAGAIN == errno) continue;  //  if O_NONBLOCK, spin if any data was initially sent
                        else l_RC = -1;
                    }
                    l_TotalBufferWritten += l_NumBytes;
                }

                if (!l_RC) {
                    //  Now, write the trailing message that will unlock the write lock...
                    l_TotalWritten += l_TotalBufferWritten;
                    l_NumBytes = write(l_Msg);
                    if (l_NumBytes >= 0) {
                        pMsg->sent();
                    } else {
                        //  Error when writing the trailing message...
                        l_RC = -1;  //  errno is already set
                    }
                } else {
                    // Error when writing the buffer.
                    // Unconditionally, unlock the write lock...
                    writeUnlock(0); //  l_RC and errno already set
                }
            } else {
                // Error when writing message...
                l_RC = -1;  //  errno already set
            }
        } else {
            // Could not link messages...
            // \todo What should errno be set to?  @@DLH
            l_RC = -1;
        }
        if (l_Msg) {
            delete l_Msg;
            l_Msg = 0;
        }
    } else {
        // \todo Need additional support if we are to send already linked messages with a buffer...  @@DLH
        l_RC = -1;
    }

    return ((!l_RC) ? l_TotalWritten : (ssize_t)l_RC);
}

ssize_t ConnexSocket::writeWithBuffer(txp::Msg* pMsg, memItemPtr mip ) {
   return writeWithBuffer(pMsg, mip->address,mip->length,0);
}

int ConnexSocket::attachRemote(){
    if (_sockfd==-1) {
        if (openCnxSock()< 0) return -errno;
    }
    if (_dialRemote) {
        if (connect2Remote()) return -errno;
    } else {    //  Listen for an incoming connection
        if (bindCnxSock()==-1) return -errno;
        if (listen4remote() ) return -errno;
        if (accept()==-1) return -errno;
    }
    
    return _sockfd;
}

int ConnexSocket::attachRemote(int retryTime, int retryCount){
    int rc = attachRemote();
    for (int i=0; i<retryCount; i++)
    {
        if (rc>0) break;
        sleep(retryTime);
        rc = attachRemote();
    }
    return rc;
}
