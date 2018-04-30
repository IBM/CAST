/*******************************************************************************
 |    RDMAcoral.h
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


//! \file  RDMAcoral.h
//! \brief
//  http://linux.die.net/man/7/rdma_cm
#ifndef __RDMACORAL_H__
#define __RDMACORAL_H__
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <sys/mman.h>

#include "Connex.h"

static const int ALL_QP_ATT_MASK =
IBV_QP_STATE|IBV_QP_CUR_STATE|
IBV_QP_EN_SQD_ASYNC_NOTIFY|IBV_QP_ACCESS_FLAGS|IBV_QP_PKEY_INDEX|IBV_QP_PORT|IBV_QP_QKEY|IBV_QP_AV|
IBV_QP_PATH_MTU|IBV_QP_TIMEOUT|IBV_QP_RETRY_CNT|IBV_QP_RNR_RETRY|IBV_QP_RQ_PSN	|IBV_QP_MAX_QP_RD_ATOMIC|
IBV_QP_ALT_PATH	|IBV_QP_MIN_RNR_TIMER|IBV_QP_SQ_PSN|IBV_QP_MAX_DEST_RD_ATOMIC|IBV_QP_PATH_MIG_STATE|IBV_QP_CAP|(int)IBV_QP_DEST_QPN;

typedef struct lidStruct {
  uint16_t numLids;
  uint16_t lid[15];
} lidStruct;
  
int dumpQPAttr(struct ibv_qp_attr* pAttr);

//! Transport component uses txp namespace
namespace txp {

typedef struct cqe {
  ::memItem mi;
  struct ibv_wc wc;
} cqe;

typedef cqe * cqePtr;

#define _CORAL_RDMA_CHUNK 1048576
class RDMACMfship : public ConnexSocket {

private:
    RDMACMfship(const RDMACMfship& rRDMACMcoral)=delete;//no copy constructor permitted
    RDMACMfship& operator=(const RDMACMfship& rRDMACMcoral)=delete; //no assignment permitted

public:
    RDMACMfship(int sockFamily, int sockType, uint64_t& pRDMAchunkSize, uint32_t& pMaxRDMAchunks);
    ~RDMACMfship();

    // Virtual methods
    virtual int accept();
    virtual int accept(txp::Connex* &pNewSock){return 0;}
    virtual char * accessBuffer(txp::Msg* pMsg,  const size_t pDataBufferSize) {
        char * c = pMsg->getDataBufferPtr();
        if (!c) abort();
        uint32_t l_msgLengthWithData = pMsg->getMsgLengthWithDataValues();
        c+= l_msgLengthWithData;
        return c;
    }
    virtual int bindCnxSock();
    virtual int connect2Remote(void);
    virtual void freeReadChunk(memItemPtr& mip);
    virtual memItemPtr getReadChunk();
    virtual memItemPtr getSendChunk(int sizeRequest) { return _regSendMemChunksPtr->getChunk(); }
    virtual memItemPtr getSendChunk() { return _regSendMemChunksPtr->getChunk(); }
    virtual memItemPtr getRDMAChunk(){ return _regRDMAChunksPtr->getChunk(); }
    virtual uint64_t getRDMAchunkSize() {return _RDMAchunkSize;}

    virtual void freeSendChunk(memItemPtr& mip){ _regSendMemChunksPtr->putChunk(mip);}
    virtual void freeRDMAChunk(memItemPtr& mip){ _regRDMAChunksPtr->putChunk(mip);}
    virtual int dataRcvImmediate(memItemPtr mip, int msgLength) { return (mip->length > (unsigned)msgLength);}
    virtual int doSendImmediate(uint32_t dataLength) { 
             return (dataLength + 512 < _recvWorkRequestChunkSize); } //512 is slack for the message size
    virtual int rdmaReadFromTo(memItemPtr pRemoteMip,memItemPtr pRecvMsgMip);
    virtual int rdmaWriteToFrom(memItemPtr pRemoteMip,memItemPtr pRecvMsgMip);

    virtual int poll4DataIn();
    int pollEventIn(const int timeout=60000);///1 minute=60000 milliseconds (timout is ms) -1=forever
    virtual int read(txp::Msg* &pMsg, char* pDataBuffer=0, const size_t pDataBufferSize=0, const txp::DeserializeOption pDeserializeOption=txp::DEFAULT_DESERIALIZE_OPTION);
    /**
    * \brief Read in a message on a connection object
    * \return Message ptr and buffer pointer for handling a message
    * \note The caller must call delete on MsgwithBuffPair.first and delete[] on MsgwithBuffPair.second
    */
    virtual void releaseBuffer(char *& pCharBuff) { pCharBuff=NULL; }
    virtual void sentSendChunk(memItemPtr& mip){};
    virtual void setLocalAddr(in_addr_t addr, uint16_t port=0);
    virtual void setRemoteAddr(in_addr_t addr, uint16_t port);
    virtual ssize_t write(txp::Msg* pMsg);
    virtual ssize_t writeWithBuffer(txp::Msg* pMsg, char * pBuffer, size_t pBufflength, uint64_t pkey){abort(); return 0;}
    virtual ssize_t writeWithBuffer(txp::Msg* pMsg, memItemPtr mipRead) final;

    ssize_t write(txp::Msg* pMsg, memItemPtr mip);

    // Non-virtual methods
    int accept(struct rdma_cm_id * pCmID);
    int acceptRemote();
    int attachRemote(); // \TODO consider making virtual and also CnxSock
	int bindAndListenLocalPort();
    int createId(struct rdma_cm_id *& pCMID);
    int createEventChannel();
    int createQp(bool signalSendQueue);
    virtual int disconnect();
    bool isNonBlockMode(void);
    int prepareClient();
    int setAsyncPollFD(struct rdma_cm_id * pCmID);
    int handleAsyncEvent(struct rdma_cm_id * pCmID);
    int prepareServer();
    int RdmaConnection(const std::string localAddr, const std::string localPort, const std::string remoteAddr, const std::string remotePort);
    int reject(struct rdma_cm_id * pCmID);
    int resolveAddress(struct sockaddr_in *localAddr, struct sockaddr_in *remoteAddr);
    int resolveRoute(void);
    int setLocalAddr(const std::string localAddr, const std::string localPort);
    int setNonBlockMode(bool mode);
    int setRemoteAddr(const std::string remoteAddr, const std::string remotePort);
    void setSendFlags(int value) {_write_send_flags=value;}
    int stringToAddress(const std::string addrString, const std::string portString, struct sockaddr_in *address) const;
    int waitForConnectRequest();
    int waitForEstablished();
    int _write_send_flags;

    // Inline methods
    inline in_port_t getLocalPortCM(struct rdma_cm_id * pCmId) {
        struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_localAddress;
        if (sockaddr_in.sin_port==0)  if (pCmId) sockaddr_in.sin_port=rdma_get_src_port(pCmId);
        return ntohs(sockaddr_in.sin_port);
    }
    inline in_port_t getLocalListenerPort() {return getLocalPortCM(_cmIdListener);}

    inline in_port_t getRemotePort(struct rdma_cm_id * pCmId) {
	struct sockaddr_in& sockaddr_in = (struct sockaddr_in &)_remoteAddress;
        if (sockaddr_in.sin_port==0)  if (pCmId) sockaddr_in.sin_port=rdma_get_dst_port(pCmId);
	return ntohs(sockaddr_in.sin_port);
    }

    inline void getRemoteAddr(struct sockaddr &pRemoteAddr) {
    	memcpy(&pRemoteAddr, (void*)&(_remoteAddress), sizeof(struct sockaddr_in));
    	return;
    }
    inline void getLocalAddr(struct sockaddr &pLocalAddr) {
    	memcpy(&pLocalAddr, (void*)&(_localAddress), sizeof(struct sockaddr_in));
    	return;
    }

    inline std::string getRemoteAddr4withPort() {
                        struct sockaddr l_RemoteAddr;
                        getRemoteAddr(l_RemoteAddr);
			return getAddr4( l_RemoteAddr) +":"+ boost::lexical_cast<std::string>(getRemotePort(_cmId));
		}

    inline std::string getLocalAddr4withPort() {
                        struct sockaddr l_LocalAddr;
                        getLocalAddr(l_LocalAddr);
			return getAddr4( l_LocalAddr) +":"+ boost::lexical_cast<std::string>(rdma_get_src_port(_cmId));
		}


    std::string getInfoString(){
                   return "RDMACMfship=(L)"+getLocalAddr4withPort()+"/(R)"+ getRemoteAddr4withPort();
                }
    inline struct ibv_pd * getProtectionDomain() const {return _cmId->pd;}
    inline struct ibv_context *getContext(void) const { return _cmId->verbs; }
    inline struct ibv_context *getEventContext(void) const { return _event->id->verbs; }

    inline int getMigState(){
       doQPquery();
       return _QPattrPtr->path_mig_state;
    }

     
private:
    
    // Non-static methods
    int ackEvent(void);
    std::string addressToString(const struct sockaddr_in *address) const;
    //! \brief  Enable listening for connections from clients.
    //! \param  backlog Number incoming connections to put in backlog.
    //! \return 0 when successful, errno when unsuccessful.
    int listen(int backlog);
    int putBackRcvWorkRequest(char* pAddr) const;
    struct ibv_cq * reArmCQ();
    int setUpRcvWorkRequests();
    int waitForEvent(void);
    int checkBadRecvAddr(void * pAddr)const;

/*
struct ibv_mr {
    struct ibv_context     *context;
    struct ibv_pd          *pd;
    void               *addr;
    size_t          length;
    uint32_t        handle;
    uint32_t        lkey;
    uint32_t        rkey;
};
*/

   // Member data
   struct ibv_mr * _regRcvMemPtr;
   MemChunkRDMAptr _regSendMemChunksPtr;
   MemChunkRDMAptr _regRDMAChunksPtr;


   int _family;    //!< socket family
#if 0
   int _type;      //!< socket type
   int _protocol;
#endif
   int _sockfd;
   int _timeout_ms4resolveAddr; // \TODO need a setter
   struct ibv_pd * _pd;
   struct ibv_comp_channel * _completionChannel;
   struct ibv_cq * _sendReceiveCQ;
   int _cqEntries;
   int _numEvents;

    //! RDMA connection management id.
   struct rdma_cm_id *_cmId;
   struct rdma_cm_id *_cmIdListener;

   //! Event channel for notification of RDMA connection management events.
   struct rdma_event_channel *_eventChannel;

   //! Current RDMA connection management event.
   struct rdma_cm_event *_event;


   //! Address of this (local) side of the connection.
   struct sockaddr_in _localAddress;

   //! Address of other (remote) side of the connection.
   struct sockaddr_in _remoteAddress;

#if 0
   //! Total number of receive operations posted to queue pair.
   uint64_t _totalRecvPosted;

   //! Total number of send operations posted to queue pair.
   uint64_t _totalSendPosted;

   //! Total number of rdma read operations posted to queue pair.
   uint64_t _totalReadPosted;

   //! Total number of rdma write operations posted to queue pair.
   uint64_t _totalWritePosted;
#endif

   socklen_t _sockaddrlen;
   struct ibv_qp_init_attr _qp_init_attr;
   uint32_t _maxRecvWorkRequests;
   uint64_t _recvWorkRequestChunkSize;
   uint32_t _maxSendWorkRequests;
   uint64_t _sendWorkRequestChunkSize;

   uint32_t _maxRDMAchunks;
   uint64_t _RDMAchunkSize;

   enum pollList {eventChannel=0, completionChannel=1, asyncEvent=2, numInPollList};
   pollfd _pollInfo[numInPollList];

   struct ibv_device_attr * _ibvDeviceAttrPtr;
   struct ibv_port_attr * _portAttrArray;
   struct ibv_qp_attr * _QPattrPtr;
   struct ibv_qp_init_attr *_QPintattrPtr;
   
   int _lastMigStateChange;
   uint16_t * _dlidList;//list of destination lid per port on remote device
   uint16_t * _slidList;//local list of source lid per port on this local device
   uint16_t _dlidNum;
   uint16_t _slidNum;
   
public:
int canDoAPM();
int armAPM();
int forceMigrate();
private:
// return 0 if success, errno otherwise
int setDeviceAttr();
int doQPquery();
int doPortQuery();
int findAltPort(int currentPort);
int findAltdlid(int currentdlid);

/*
struct rdma_cm_id {
    struct ibv_context  *verbs;
    struct rdma_event_channel *channel;
    void            *context;
    struct ibv_qp       *qp;
    struct rdma_route    route;
    enum rdma_port_space     ps;
    uint8_t          port_num;
    struct rdma_cm_event    *event;
    struct ibv_comp_channel *send_cq_channel;
    struct ibv_cq       *send_cq;
    struct ibv_comp_channel *recv_cq_channel;
    struct ibv_cq       *recv_cq;
    struct ibv_srq      *srq;
    struct ibv_pd       *pd;
    enum ibv_qp_type    qp_type;
};
*/

/*
Client Operation
This section provides a general overview of the basic operation for the active, or client, side of communication. This flow assume asynchronous operation with low level call details shown. For synchronous operation, calls to rdma_create_event_channel, rdma_get_cm_event, rdma_ack_cm_event, and rdma_destroy_event_channel would be eliminated. Abstracted calls, such as rdma_create_ep encapsulate serveral of these calls under a single API. Users may also refer to the example applications for code samples. A general connection flow would be:

rdma_getaddrinfo
    retrieve address information of the destination
rdma_create_event_channel
    create channel to receive events
rdma_create_id
    allocate an rdma_cm_id, this is conceptually similar to a socket
rdma_resolve_addr
    obtain a local RDMA device to reach the remote address

rdma_get_cm_event
    wait for RDMA_CM_EVENT_ADDR_RESOLVED event
rdma_ack_cm_event
    ack event
-------------> Need to create completion queue(s) for send/receive  (maybe srq if shared receive)
rdma_create_qp
    allocate a QP for the communication
rdma_resolve_route
    determine the route to the remote address
rdma_get_cm_event
    wait for RDMA_CM_EVENT_ROUTE_RESOLVED event
rdma_ack_cm_event
    ack event
rdma_connect
    connect to the remote server
rdma_get_cm_event
    wait for RDMA_CM_EVENT_ESTABLISHED event
rdma_ack_cm_event
    ack event

Perform data transfers over connection

rdma_disconnect
    tear-down connection
rdma_get_cm_event
    wait for RDMA_CM_EVENT_DISCONNECTED event
rdma_ack_cm_event
    ack event
rdma_destroy_qp
    destroy the QP
rdma_destroy_id
    release the rdma_cm_id
rdma_destroy_event_channel
    release the event channel

SERVER

This section provides a general overview of the basic operation for the passive, or server, side of communication. A general connection flow would be:

rdma_create_event_channel
    create channel to receive events
rdma_create_id
    allocate an rdma_cm_id, this is conceptually similar to a socket
rdma_bind_addr
    set the local port number to listen on
rdma_listen
    begin listening for connection requests
rdma_get_cm_event
    wait for RDMA_CM_EVENT_CONNECT_REQUEST event with a new rdma_cm_id
rdma_create_qp
    allocate a QP for the communication on the new rdma_cm_id
rdma_accept
    accept the connection request
rdma_ack_cm_event
    ack event
rdma_get_cm_event
    wait for RDMA_CM_EVENT_ESTABLISHED event
rdma_ack_cm_event
    ack event

Perform data transfers over connection
*/

};//class end

}   // namespace

#endif //__RDMACORAL_H__
