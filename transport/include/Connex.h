/*******************************************************************************
 |    Connex.h
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


/**
 *
 *
 * \section Overview
 *
 * The Connection object base class is pure virtual requiring derived classes which implement
 * pure virtual methods.  Classes which send messages use methods to write and read on the
 * connection object.  The connection object is set up by main or other code in a daemon
 * and connects to the other daemon using a connection object--e.g., sockets connection object.
 *
 * The intent is for each connection object to represent a single connection.
 *
 * The intent is to also simplify connecting by providing a reasonable wrapper
 *
 * The intent is also to provide poll method and detection of connection failure.
 *
 * Derived methods <b>require</b>
 * -readMsg
 * -write
 * -writeWithBuffer
 *
 * \todo update required list
 *
 * \section thread_safety Thread Safety
 *
 * Multiple threads are not supported
 *
 * \section Warning
 */

//! \file  Connex.h
//! \brief Connection objects include for setting up and using communications

#ifndef CNX_Connex_H
#define CNX_Connex_H

// Includes

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <boost/property_tree/ptree.hpp>

#include "fshipMacros.h"
#include "Msg.h"
#include "MemChunk.h"

//! Transport component uses txp namespace
namespace txp {
inline std::string getAddr4(const struct sockaddr& pSockaddr){
                    char str[INET_ADDRSTRLEN];
                    str[0]=0;
                    struct sockaddr_in * sa = (struct sockaddr_in *)&pSockaddr;
                    inet_ntop(AF_INET,&sa->sin_addr,str,INET_ADDRSTRLEN);
                    std::string s = str;
                    return s;
                }

    //! \class Connex
    //! \brief Connection object base class--pure virtual class
    class Connex {
    private:

    protected:
        int _sockfd;
        pthread_mutex_t _readLock;  //!< provides serialization of reads
        pthread_mutex_t _writeLock; //!< provides serialization of writes
        int _useMonitor;
        int _lastReadHadNextMsg;
        std::string _connectionName;
        std::string _connectionNameAlias;
        int attachRemote(int retryTime, int retryCount);

    public:
        // Constructors
        Connex() : _sockfd(-1) {
            _useMonitor=0;
            _lastReadHadNextMsg=0;
            _readLock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
            _writeLock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
        }

        // Destructors
        virtual ~Connex() {
            pthread_mutex_destroy(&_readLock);
            pthread_mutex_destroy(&_writeLock);
        }; //need to provide for delete of derived-class object via a base-class pointer
        void setConnectName(const std::string& pConnectioName){
            _connectionName = pConnectioName;
        }
        std::string getConnectName( ) const {
            return _connectionName;
        }
        void setConnectNameAlias(const std::string& pConnectioName){
            _connectionNameAlias = pConnectioName;
        }
        std::string getConnectNameAlias( ) const {
            return _connectionNameAlias;
        }

        //  Pure virtual methods
        virtual char * accessBuffer(txp::Msg* pMsg,  const size_t pDataBufferSize)=0;
        virtual void releaseBuffer(char *& pCharBuff)=0;
        virtual ssize_t getDataBufferToRead(char * pBuffer, size_t pBufflength, uint64_t pkey)=0;

        //! \brief get a chunk of memory for send processing
        virtual memItemPtr getSendChunk(int sizeRequest);
        virtual memItemPtr getSendChunk(){return getSendChunk(64*1024);}

        virtual memItemPtr getRDMAChunk(){return getSendChunk(64*1024);}
        virtual uint64_t getRDMAchunkSize() {return 64*1024;}


        /**
         * \brief Read in a message on a connection object
         * \return Message ptr and buffer pointer for handling a message
         * \note The caller must call delete on MsgwithBuffPair.first and delete[] on MsgwithBuffPair.second
         */
        virtual int read(txp::Msg* &oMsg, char* pDataBuffer=0, const size_t pDataBufferSize=0, const txp::DeserializeOption pDeserializeOption=txp::DEFAULT_DESERIALIZE_OPTION )=0;   //!< pure virtual class method for read message on connection object
        virtual void sentSendChunk(memItemPtr& mip);
        /**
         * \brief Write a message on a connection object
         * \return Total bytes written
         */
        virtual ssize_t write(txp::Msg* pMsg)=0;  //!< pure virtual class method to write a message using connection object
        virtual ssize_t writeWithBuffer(txp::Msg* pMsg, char * pBuffer, size_t pBufflength, uint64_t pkey)=0;
        virtual ssize_t writeWithBuffer(txp::Msg* pMsg, memItemPtr mip )=0;
        /**
         * \brief pure virtual class method for poll of data on connection object
         * \return 1=data available for read; -1=error, check errno
         * \post data squirreled away
         */
        virtual int poll4DataIn()=0;
        virtual int disconnect()=0;
        virtual std::string getInfoString()=0;
        virtual int accept(txp::Connex* &pNewSock)=0;
        virtual int getFamily()=0;
        virtual inline int getSockfd() final {return _sockfd;};
        virtual int listen4remote()=0;
        virtual void setCred(){};
        virtual bool remoteAndLocalAddressNotSame(){return false;}
        virtual int keepAlive(){return 0;}

        // Virtual methods
        virtual memItemPtr getReadChunk();
        virtual void freeReadChunk(memItemPtr& mip);
        virtual void freeSendChunk(memItemPtr& mip);
        virtual void freeRDMAChunk(memItemPtr& mip){ freeSendChunk(mip);};
        inline virtual in_port_t getLocalPort() const {abort(); return 0;};
        virtual int dataRcvImmediate(memItemPtr mip, int msgLength) { return 1;}
        virtual int doSendImmediate(uint32_t dataLength) { return 1;}
        virtual int rdmaReadFromTo(memItemPtr pRemoteMip,memItemPtr pRecvMsgMip){ abort(); return -1;};
        virtual int rdmaWriteToFrom(memItemPtr pRemoteMip,memItemPtr pRecvMsgMip){ abort(); return -1;};
        virtual void closefd()=0;
        // Inline methods
        inline int useMonitor() {
            return _useMonitor;
        }
        inline int willUseMonitor(int val=1) {_useMonitor=val; return _useMonitor;}

        virtual uid_t getUser_ID() { abort(); return 0;}
        virtual gid_t getGrp_ID()  { abort(); return 0;}

        virtual std::string getRemoteAddrString() const {
            std::string temp;
			return temp;
		}


        // Member methods
        int readLock();
        int readUnlock(txp::Msg* pMsg=0);

        int writeLock(txp::Msg* pMsg=0);
        int writeUnlock(txp::Msg* pMsg=0);

    };  //end class Connex

    typedef Connex * ConnexPtr;


    /**********************************/
    /* abstracting socket connection  */

    class ConnexSocket : public Connex //abstract class for all socket based connections
    {
    private:

    protected:
        ConnexSocket() :
            _family(0),
            _type(0),
            _protocol(0),
            _flagsOnOpen(SOCK_CLOEXEC),
            _dialRemote(0),
            _rcLast(0),
            _memChunkReadPtr(0),
            _numChunks(64),
            _chunkSizeRead(2*64*1024) {
            _keepAliveIntvl=5;
            _keepAliveIdle=60;
            _keepAliveCount=12;
            _tcpUserTimeout=120*1000;
            _turnOnKeepAlive=true;
        }

        int _family;    //!< socket family
        int _type;      //!< socket type
        int _protocol;
        int _flagsOnOpen;
        int _dialRemote;  //!< nonzero indicates whether to dial remote; else listen
        int _rcLast;      //!< last return code
        MemChunk * _memChunkReadPtr; //!Point to memchunks for read processing
        int _numChunks;  //! number of memory chunks for read processing
        uint64_t _chunkSizeRead; //! size of memory chunks for read processing
        int _keepAliveIntvl;   //!  the interval seconds between subsequential keepalive probes
        int _keepAliveIdle;    //!  the interval seconds between the last data packet sent and the first keepalive probe
        int _keepAliveCount;   //!  the maximum number of unacknowledged probes to send
        int _tcpUserTimeout;   //!  the number of milliseconds for a TCP response
        bool _turnOnKeepAlive; //!  turn keepAlive on if true; turn off if false

    public:
        virtual int getFamily() final {return _family;}
        virtual ~ConnexSocket();

        // Pure virtual functions
        virtual int accept() =0;
        virtual int bindCnxSock() =0;
        virtual int connect2Remote() =0;
        virtual std::string getInfoString()=0;
        virtual int accept(txp::Connex* &pNewSock)=0;
        virtual int SockRead(void* pDataBuffer, const size_t pDataBufferSize);
        virtual int SockWrite(const void* pDataBuffer, const size_t pDataBufferSize);

        // Virtual functions
        virtual char * accessBuffer(txp::Msg* pMsg,  const size_t pDataBufferSize) {
            char * c = new char[pDataBufferSize];
            // Need to read data from file/socket descriptor into buffer
            //printf("sizeOfDataBuffer=%d\n",sizeOfDataBuffer);
            ssize_t getdataRC = getDataBufferToRead(c,pDataBufferSize,0);
            if (getdataRC != (ssize_t)pDataBufferSize) abort();
            return c;
        }
        virtual void freeReadChunk(memItemPtr& mip);
        virtual ssize_t getDataBufferToRead(char * pBuffer,size_t pBufflength,uint64_t pkey);
        virtual memItemPtr getReadChunk();
        virtual int listen4remote(){ return listen(_sockfd,0);}
        virtual int openCnxSock();
        virtual int poll4DataIn();

        virtual int read(txp::Msg* &pMsg, char* pDataBuffer=0, const size_t pDataBufferSize=0, const txp::DeserializeOption pDeserializeOption=txp::DEFAULT_DESERIALIZE_OPTION);
        virtual void releaseBuffer(char *& pCharBuff) {
            delete[] pCharBuff;
            pCharBuff=NULL;
        }
        virtual void setLocalAddr(in_addr_t addr, uint16_t port=0) {}
        virtual void setRemoteAddr(in_addr_t addr, uint16_t port) {}
        virtual ssize_t write(txp::Msg* pMsg);
        virtual ssize_t writeWithBuffer(txp::Msg* pMsg,char * pBuffer,size_t pBufflength,uint64_t pkey);
        virtual ssize_t writeWithBuffer(txp::Msg* pMsg, memItemPtr mip );

        int attachRemote();
        int attachRemote(int retryTime, int retryCount);
        virtual int keepAlive();
        void setKeepAliveBool(bool pTurnOnKeepAlive){_turnOnKeepAlive=pTurnOnKeepAlive;}
        void setKeepAliveParms(int pKeepAliveIntvl, int pKeepAliveIdle, int pKeepAliveCount, int pUserTimeout )
        {
            _keepAliveIntvl = pKeepAliveIntvl;
            _keepAliveIdle = pKeepAliveIdle;   
            _keepAliveCount = pKeepAliveCount;   
            _tcpUserTimeout = pUserTimeout;
        }


        virtual void closefd() {
            if (_sockfd != -1) {
                close(_sockfd);
                _sockfd = -1;
            }
        }
        virtual int disconnect(){closefd(); return 0;}

        inline int dialRemote() {
            return _dialRemote;
        }


        inline void setFlagsOnOpen(int flags) {_flagsOnOpen=flags;}

        inline void setNonBlockMode(bool mode) {
            int flags = fcntl(_sockfd,F_GETFL);
            if (mode) flags |= O_NONBLOCK;
            else flags &= ~(O_NONBLOCK);
            _rcLast = fcntl(_sockfd,F_SETFL,flags);
            if (_rcLast !=0){
                printf("%s _rcLast=%d errno=%d(%s)\n",__PRETTY_FUNCTION__,_rcLast,errno,strerror(errno) );
            }
            return;
        }

        inline timeval getSendTimeout(void) const {
            timeval timeout;
            socklen_t optlen = sizeof(timeout);
            if (getsockopt(_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, &optlen) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return timeout;
        }

        inline void setSendTimeout(timeval& timeout) {
            if (setsockopt(_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeval)) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return;
        }

        inline int getRecvBufferSize(void) const {
            int size;
            socklen_t optlen = sizeof(size);
            if (getsockopt(_sockfd, SOL_SOCKET, SO_RCVBUF, &size, &optlen) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return size;
        }

        inline void setRecvBufferSize(int size) {
            if (setsockopt(_sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return;
        }

        inline int getSendBufferSize(void) const {
            int size;
            socklen_t optlen = sizeof(size);
            if (getsockopt(_sockfd, SOL_SOCKET, SO_SNDBUF, &size, &optlen) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return size;
        }

        inline void setSendBufferSize(int size) {
            if (setsockopt(_sockfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return;
        }

        inline timeval getRecvTimeout(void) const {
            timeval timeout;
            socklen_t optlen = sizeof(timeout);
            if (getsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, &optlen) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return timeout;
        }

        inline void setNoDelay(bool value) {
			int val = value;//1 for on, 0 for off
			if (setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) != 0) {
				printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
			}
			return;
		}

        inline void setRecvTimeout(timeval& timeout) {
            if (setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval)) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return;
        }

        inline void setKeepAlive(bool value) {
            int val = value;//1 for on, 0 for off
            if (setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return;
        }

        inline void setReuseAddress(bool value) {
            int val = value;//1 for on, 0 for off
            if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
                printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
            }
            return;
        }
        inline void setlinger(int on=1, int seconds=5) {
          struct linger l_linger = { .l_onoff=0, .l_linger=seconds};
          if (on) l_linger.l_onoff=1;
          if (setsockopt(_sockfd, SOL_SOCKET, SO_LINGER, &l_linger, sizeof(l_linger)) != 0) {
		printf("%s setsockopt failed errno=%d(%s)\n",__PRETTY_FUNCTION__,errno,strerror(errno));
	  }
          return;
        }

        inline int willDialRemote(int val=1) {_dialRemote=val; return _dialRemote;}

        // Member functions
        int readInMessage();
    };

    typedef ConnexSocket* ConnexSocketPtr;

}   // namespace
#endif //CNX_Connex_H
