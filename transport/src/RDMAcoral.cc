/*******************************************************************************
 |    RDMAcoral.cc
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


//! \file  RDMAcoral.cc
//! \brief class RDMACMcoral methods

#include <sys/socket.h>
#include <netdb.h>
#include "fshipMacros.h"
#include <string.h>

#include "../include/RDMACMfship.h"


void printRdmaDeviceList();
void printPath(struct rdma_cm_id& pRDMAcmid);
void dumpMigState(int pMigState){
LOG(txp,info)<<"mig state="<<pMigState<<":"<< (pMigState?(pMigState==1?"IBV_MIG_REARM":"IBV_MIG_ARMED"):"IBV_MIG_MIGRATED");
}

/**
 * rdma_get_devices - Get list of RDMA devices currently available.
 * @num_devices: If non-NULL, set to the number of devices returned.
 * Description:
 *   Return a NULL-terminated array of opened RDMA devices.  Callers can use
 *   this routine to allocate resources on specific RDMA devices that will be
 *   shared across multiple rdma_cm_id's.
 * Notes:
 *   The returned array must be released by calling rdma_free_devices.  Devices
 *   remain opened while the librdmacm is loaded.
 * See also:
 *   rdma_free_devices
struct ibv_context {
	struct ibv_device      *device;
	struct ibv_context_ops	ops;
	int			cmd_fd;
	int			async_fd;
	int			num_comp_vectors;
	pthread_mutex_t		mutex;
	void		       *abi_compat;
};
 */


void adjustRDMAchunks(uint64_t& pRDMAchunkSize, uint32_t& pMaxRDMAchunks){
  //first adjust chunk size to be a multiple of a page
  const unsigned int pageSize=getpagesize();
  if (pRDMAchunkSize < pageSize) pRDMAchunkSize=pageSize;
  if (pMaxRDMAchunks == 0) pMaxRDMAchunks = 1;
  const int pageMasklow = pageSize -1; //assume pageSize is a power of 2
  if (pRDMAchunkSize & pageMasklow) {
     pRDMAchunkSize &= ~pageMasklow;
     pRDMAchunkSize += pageSize;
  }
}



using namespace  txp;



RDMACMfship::RDMACMfship(int sockFamily, int sockType, uint64_t& pRDMAchunkSize, uint32_t& pMaxRDMAchunks) :
    txp::ConnexSocket(),
    _family(sockFamily),

    _sockfd(-1),
    _cmId(NULL),
    _cmIdListener(NULL),
    _eventChannel(NULL)
{
    memset(&_localAddress,0,sizeof(_localAddress));
    memset(&_remoteAddress,0,sizeof(_remoteAddress));
    _sockaddrlen = sizeof(struct sockaddr_in);
    _dialRemote=0; //assume listening
    _rcLast=0;
    _timeout_ms4resolveAddr = 5000;
    _pd=NULL;
    _completionChannel = NULL;
    _sendReceiveCQ = NULL;
    _cqEntries=256; // \TODO: make configurable

    _regRcvMemPtr=NULL;
    _maxRecvWorkRequests=64;
    _recvWorkRequestChunkSize = 4*64*1024;
    _regSendMemChunksPtr = NULL;
    _maxSendWorkRequests=64;
    _sendWorkRequestChunkSize = _recvWorkRequestChunkSize;

   _regRDMAChunksPtr = NULL; //pointer to object for BIG RDMA blocks
   adjustRDMAchunks(pRDMAchunkSize,pMaxRDMAchunks);
   _maxRDMAchunks = pMaxRDMAchunks;
   _RDMAchunkSize = pRDMAchunkSize;

    _numEvents = 0;
        //IBV_SEND_SIGNALED fires the completion notification indicater for the work request (WR) for the local QP.  Ignored if QP was created with sq_siq_all.
        //IBV_SEND_SOLICITED creates a solicted event to the remote QP and if user is waiting for a solicted event the user is woken up
        // \TODO for fshipcld no need for SIGNALED but for fshipd need signaled
    _write_send_flags=IBV_SEND_SOLICITED | IBV_SEND_SIGNALED; //default
    for (int i=0;i<numInPollList;i++){
        _pollInfo[i].events=POLLIN;
        _pollInfo[i].revents=0;
        _pollInfo[i].fd=-1;
    }
   _ibvDeviceAttrPtr=NULL;
   _portAttrArray=NULL;
   _QPattrPtr=NULL;
   _QPintattrPtr=NULL;
   _lastMigStateChange = -1; //not yet defined
   _dlidList=NULL;
   _dlidNum=0;
   _slidNum=0;
   _slidList=NULL;
    printRdmaDeviceList();
};



RDMACMfship::~RDMACMfship(){
  if (_cmId) {
      /*    * Notes:
            *   Users must free any associated QP with the rdma_cm_id before destroying the cm ID
            *   and ack an related events.<<<<<<<<<<<<<<<<<<
      */
      disconnect();
      rdma_destroy_qp(_cmId);
      if (_sendReceiveCQ){
         if (_numEvents) ibv_ack_cq_events(_sendReceiveCQ,_numEvents); //required before destroying cq
         int errnoCQ =  ibv_destroy_cq(_sendReceiveCQ);
         if (errnoCQ) LOG(txp,warning)<< "ibv_destroy_cq"<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoCQ<<", "<<strerror(errnoCQ);
      }
      if (_completionChannel) {
         int errnoCC = ibv_destroy_comp_channel(_completionChannel);
         if (errnoCC) LOG(txp,warning)<< "ibv_destroy_comp_channel"<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoCC<<", "<<strerror(errnoCC);
      }
      if (_regRcvMemPtr) {
        int errnoDeregRcvMem = ibv_dereg_mr(_regRcvMemPtr);
        if (errnoDeregRcvMem) LOG(txp,warning)<< "ibv_dereg_mr regRcvMemPtr failed"<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoDeregRcvMem<<", "<<strerror(errnoDeregRcvMem);
      }
      if (_pd) { //need to free resources associated with pd prior to freeing pd
        int errnoRet = ibv_dealloc_pd(_pd);
        if (errnoRet) LOG(txp,warning)<< "ibv_dealloc_pd failed "<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoRet<<", "<<strerror(errnoRet);
      }
      if (_regSendMemChunksPtr ) {delete _regSendMemChunksPtr; _regSendMemChunksPtr=NULL;}
      if (_regRDMAChunksPtr)     {delete _regRDMAChunksPtr;    _regRDMAChunksPtr=NULL;   }
      if (_cmId){
         int err = rdma_destroy_id (_cmId);
         if (err) LOG(txp,warning)<< "rdma_destroy_id failed"<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
         _cmId=NULL;
      }
  }
  if (_eventChannel) rdma_destroy_event_channel(_eventChannel);
}

int RDMACMfship::stringToAddress(const std::string addrString, const std::string portString, struct sockaddr_in *address) const
{
   struct addrinfo *res;
   int err = getaddrinfo(addrString.c_str(), NULL, NULL, &res);
   if (err != 0) {
      LOG(txp,warning)<< "getaddrinfo() failed"<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<gai_strerror(errno);
      return err;
   }

   *address = *(struct sockaddr_in *) res->ai_addr;
   address->sin_port = (in_port_t)atoi(portString.c_str());

   freeaddrinfo(res);
   return err;
}

int RDMACMfship::bindAndListenLocalPort()
{
  int err = createEventChannel();
  if (err) return err;
  err = createId(_cmIdListener);
  if (err) return err;
  err = bindCnxSock(); //uses _cmIdListener
  if (err) return err;
  printPath(*_cmIdListener);
  return listen(1);//uses _cmIdListener
}

int RDMACMfship::createEventChannel()
{
    if (_eventChannel){
        errno=EEXIST;
        LOG(txp,warning)<< "_eventChannel exists "<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
        return errno;
    }

    // Create the event channel.
    _eventChannel = rdma_create_event_channel();
    if (_eventChannel == NULL) {
        LOG(txp,warning)<< "rdma_create_event_channel() failed"<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
        errno=EINVAL;
        return errno;
    }

    int flags = fcntl(_eventChannel->fd, F_GETFL);
    flags |= O_NONBLOCK;
    int rc = fcntl(_eventChannel->fd, F_SETFL, flags);
    if (rc != 0) {
      // e(errno, "fcntl() failed");
      LOG(txp,always)<< "error changing event channel with fd " << _eventChannel->fd << "to O_NONBLOCK" << std::hex << flags<<std::dec<<" errno="<<errno<<":"<<strerror(errno) ;
   }

    LOG(txp,always)<<  "created rdma event channel with fd " << _eventChannel->fd<<std::hex<<" flags="<<flags<<std::dec;
    _pollInfo[eventChannel].fd = _eventChannel->fd; //will poll on this file descriptor
    _pollInfo[eventChannel].events = POLLIN;
     _pollInfo[eventChannel].revents = 0;
    return 0;
}

int RDMACMfship::createId(struct rdma_cm_id *& pCMID)
{
    if (pCMID){
        errno=EEXIST;
        LOG(txp,warning)<< "pCMID exists ="<<pCMID<<" "<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
        return errno;
    }

    // Create the rdma cm id.
    int err = rdma_create_id(_eventChannel, &pCMID, this, RDMA_PS_TCP);
    if (err != 0) {
        LOG(txp,warning)<< "rdma_create_id() "<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
        return errno;
    }

    LOG(txp,always)<< "created rdma cm id " << pCMID;
    return 0;
}

int RDMACMfship::attachRemote() {

	if (_dialRemote) {
        LOG(txp,always)<< "dialing remote";
        int err = prepareClient();
          if (err) return -abs(err);
          err = connect2Remote();
	  if (err) return -abs(err);
	} else {    //  Listen for an incoming connection
        LOG(txp,always)<< "listening for an incoming connection";
		int err = prepareServer();
        return -abs(err);
	}

	return 0;
}

void RDMACMfship::setLocalAddr(in_addr_t addr, uint16_t port) {
	_localAddress.sin_family = _family;
	memcpy(&_localAddress.sin_addr.s_addr, &addr, sizeof(in_addr_t));
	_localAddress.sin_port = htons(port);
}

int RDMACMfship::setLocalAddr(const std::string localAddr, const std::string localPort){
   int err = stringToAddress(localAddr, localPort, &_localAddress);
   if (err != 0) {
       LOG(txp,warning)<< "failed to generate local address "<<__PRETTY_FUNCTION__<<":"<< __LINE__;
   }
   return err;
}

void RDMACMfship::setRemoteAddr(in_addr_t addr, uint16_t port) {
	_remoteAddress.sin_family = _family;
	memcpy(&_remoteAddress.sin_addr.s_addr, &addr, sizeof(in_addr_t));
	_remoteAddress.sin_port = htons(port);
}

int RDMACMfship::setRemoteAddr(const std::string remoteAddr, const std::string remotePort){

   int err = stringToAddress(remoteAddr, remotePort, &_remoteAddress);
   if (err != 0) {
      LOG(txp,warning)<< "failed to generate remote address "<<__PRETTY_FUNCTION__<<":"<< __LINE__;
   }
   return err;
}

int RDMACMfship::RdmaConnection(const std::string localAddr, const std::string localPort, const std::string remoteAddr, const std::string remotePort)
{
   int err = setLocalAddr( localAddr, localPort);
   if (err) return err;
   err = setRemoteAddr(remoteAddr, remotePort);
   return err;
}


int  RDMACMfship::poll4DataIn(){
	if (-1==_pollInfo[completionChannel].fd){
		errno=ENOTCONN;
		return -1;
	}
	const int numFds=numInPollList;
	int timeout = 60000;//1 minute=60000 milliseconds (timout is ms) -1=forever

	int rc=0;

	while (rc==0){
                for (int i=0;i<numInPollList;i++){
                  _pollInfo[i].events=POLLIN;
                  _pollInfo[i].revents=0;
                }

		rc = ::poll(_pollInfo, numFds, timeout);

                if ( (_pollInfo[eventChannel].revents) || (rc==0) ) {//if RDMA event or timeout
                  int err = rdma_get_cm_event(_eventChannel, &_event);
                  if (!err) {
                     LOG(txp,info)<< "poll4DataIN() cmid=0x"<<std::hex<<_event->id<<" listen_id=0x"<<_event->listen_id<<std::dec<<" status="<<_event->status<<" waitForEvent event=" << rdma_event_str(_event->event);
                     if (_event->event == RDMA_CM_EVENT_DISCONNECTED) {
                       //_pollInfo[completionChannel].fd = -1;
                       disconnect();
                       timeout=100;
                       //return -ECONNRESET;
                     }
                     if (_event->event == RDMA_CM_EVENT_DEVICE_REMOVAL) {
                       _pollInfo[completionChannel].fd = -1;
                       return -ENODEV;
                     }
                     if (_event->event == RDMA_CM_EVENT_TIMEWAIT_EXIT) {
                       _pollInfo[completionChannel].fd = -1;
                       return 0;
                     }
                  }
                }

                if (rc==(-1)){//errno came in
			if(errno==EINTR)continue; //ignore an interrupt
			LOG(txp,warning)<< "error polling completion channel using fd " << _completionChannel->fd <<" errno="<<errno<<":"<<strerror(errno);
                        _pollInfo[completionChannel].fd = -1;
			return -errno;
		}

                if ( (_pollInfo[asyncEvent].revents) || (rc==0) ) {//if RDMA event or timeout
                  int rc = handleAsyncEvent(_cmId);
                  if (rc) printf("rc=%d",rc);

                }

                if (rc==(-1)){//errno came in
			if(errno==EINTR)continue; //ignore an interrupt
			LOG(txp,warning)<< "error polling completion channel using fd " << _completionChannel->fd <<" errno="<<errno<<":"<<strerror(errno);
                        _pollInfo[completionChannel].fd = -1;
			return -errno;
		}

                if (rc==0) return 1;

                if (_pollInfo[completionChannel].revents & POLLIN ){
                     struct ibv_cq * cq = reArmCQ();
                     // \TODO in the future plan to pull all of the cq and have other threads process work completions off a list
                     if (cq) return 1;
                     _pollInfo[completionChannel].fd = -1;
		     return -1;
                }

		if (_pollInfo[completionChannel].revents & (POLLERR|POLLHUP|POLLNVAL) ) {//error!
			close(_sockfd);
			errno=ECONNRESET;
			LOGERRNO(txp,error,errno);//syslog that something bad happened
			_pollInfo[completionChannel].fd=-1;
			return -1;
		}
	}
	return rc;
}

int RDMACMfship::prepareServer()
{
  int err=bindAndListenLocalPort();
  if (err) return err;
  return acceptRemote();
}

int RDMACMfship::accept() {
	return accept(_cmId);
}

int RDMACMfship::accept(struct rdma_cm_id * pCmID)
{
   // Accept the connection request.
   struct rdma_conn_param param;
   memset(&param, 0, sizeof(param));
   doPortQuery();//sets _slidNum and _slidList;
   //param.responder_resources = 1;
   //param.initiator_depth = 1;
   param.responder_resources = 16;
   param.initiator_depth = 16;
   lidStruct dataArea;
   param.private_data_len = sizeof(dataArea);

   dataArea.numLids=_slidNum;
   LOG(txp,always)<<"dataArea.numLids="<<dataArea.numLids;
   for (int i=0; i<dataArea.numLids;i++){
      dataArea.lid[i] = _slidList[i];
      LOG(txp,always)<<"accept dataArea.lid[i]="<<dataArea.lid[i];
   }
   param.private_data = (void *)&dataArea;
   param.retry_count = 7;
   param.rnr_retry_count = 7;
   int err = rdma_accept(pCmID, &param);
   if (err != 0) {
      err = abs(err);
      return err;
   }
   printPath(*pCmID);
   return 0;
}

int RDMACMfship::acceptRemote()
{
  int err=waitForConnectRequest();//if valid, switches from  _cmIdListener to _cmId
  if (err) return err;


  /* now know within connection ID which device is being used.  So can set protection domain */
  _pd = ibv_alloc_pd(_cmId->verbs);
  if (!_pd){
     LOG(txp,error)<< "failed to get protection domain "<<__PRETTY_FUNCTION__<<":"<< __LINE__;
     return ENOSYS;
  }
  LOG(txp,always)<< "protection domain="<<_pd;

  _completionChannel = ibv_create_comp_channel(_cmId->verbs);

  setNonBlockMode(1);//nonblocking fd
  _pollInfo[completionChannel].fd = _completionChannel->fd; //will poll on this file descriptor
  _pollInfo[completionChannel].events = POLLIN;
  _pollInfo[completionChannel].revents = 0;
  LOG(txp,always)<<"_completionChannel isNonBlockMode="<<isNonBlockMode()<<" on fd="<<_completionChannel->fd ;

  _sendReceiveCQ = ibv_create_cq(_cmId->verbs, _cqEntries, this, _completionChannel,0);
  err = ibv_req_notify_cq(_sendReceiveCQ,  0);//completion channel is active for cq events
  if (err){
    LOG(txp,warning)<< "ibv_req_notify_cq failed "<<__PRETTY_FUNCTION__<<":"<< __LINE__<<" errno="<<err<<", "<<strerror(err);
    return err;
  }
  err = createQp(1);//always generate work completion for work requests to the send queue
  if (err) return err;
  _regSendMemChunksPtr = new MemChunkRDMA(_maxSendWorkRequests, _sendWorkRequestChunkSize, _cmId->pd);
  _regRDMAChunksPtr    = new MemChunkRDMA(_maxRDMAchunks, _RDMAchunkSize, _cmId->pd);

  err = setUpRcvWorkRequests();
  if (err) return err;
  //need to accept connection--or not
  err=accept();
  if (err) return err;
  err= waitForEstablished();
  if (err) return err;

   err =setAsyncPollFD(_cmId);
   if (err) return err;

   err=armAPM();
   if (err) LOG(txp,always)<<"acceptRemote method armAPM err="<<err;

  return 0; //Log APM error but keep going
}

int RDMACMfship::reject(struct rdma_cm_id * pCmID){
   // Reject a connection request.
   int err = rdma_reject(pCmID, NULL, 0);
   if (err != 0) {
     return err;
   }
   return 0;
}
int RDMACMfship::handleAsyncEvent(struct rdma_cm_id * pCmID){
   struct ibv_async_event l_asyncEvent;
   int migState = getMigState();
   if (_lastMigStateChange != migState){
      _lastMigStateChange = migState;
      dumpMigState(migState);
      if (migState==IBV_MIG_ARMED){
#if 0
         int rc =  forceMigrate();
         printf("forceMigrate rc=%d\n",rc);
#endif
      }
   }

   while (!ibv_get_async_event(pCmID->verbs,&l_asyncEvent)){
     LOG(txp,info)<< "async_event=" << l_asyncEvent.event_type<<":"<< ibv_event_type_str(l_asyncEvent.event_type);
     switch(l_asyncEvent.event_type){
        //QP events (l_asyncEvent.qp is valid struct ibv_qp  *qp
        case IBV_EVENT_QP_FATAL: //QP error state, error occurred on QP
        case IBV_EVENT_QP_REQ_ERR: //invalid request to work queue (bad wookie)
        case IBV_EVENT_QP_ACCESS_ERR: //access violation
           LOG(txp,info)<< "Fatal QP async_event=" << l_asyncEvent.event_type<<":"<< ibv_event_type_str(l_asyncEvent.event_type);
           doQPquery();
           dumpQPAttr(_QPattrPtr);
           //BUGON?
           return -EFAULT;
           break;
	case IBV_EVENT_COMM_EST: //communications established on a QP.  This is good
           break;
	case IBV_EVENT_SQ_DRAINED://need to do this if switching to another connection or graceful shutdown
           LOG(txp,info)<< "SQ drained" << l_asyncEvent.event_type<<":"<< ibv_event_type_str(l_asyncEvent.event_type);
           doQPquery();
           dumpQPAttr(_QPattrPtr);
           // any special handling
           break;
	case IBV_EVENT_PATH_MIG:
           migState = getMigState();
           if (_lastMigStateChange != migState){
             _lastMigStateChange = migState;
              dumpMigState(migState);
           }
           // \TODO exchange valid dlid updates with connection partner
           armAPM();
           break;
	case IBV_EVENT_PATH_MIG_ERR://QP event Automatic Path Migration did not fail over to other path
           break; //eat it, should be going down from a Disconnect
        case IBV_EVENT_QP_LAST_WQE_REACHED://QP event for SRQ association.  Not using SRQ
           break;
        //CQ event
        case IBV_EVENT_CQ_ERR:
            LOG(txp,always)<< "CQ overrun=" << l_asyncEvent.event_type<<":"<< ibv_event_type_str(l_asyncEvent.event_type);
            break;
        //CA event
        case IBV_EVENT_DEVICE_FATAL:
            return -ENODEV;
        // SRQ events, eat these since not using SRQ
        case IBV_EVENT_SRQ_ERR:
	case IBV_EVENT_SRQ_LIMIT_REACHED:
            break;
        //port events
        case IBV_EVENT_PORT_ACTIVE:
           // \TODO exchange valid dlid updates with connection partner; and then reARM
	case IBV_EVENT_PORT_ERR:
           // \TODO exchange valid dlid updates with connection partner; and then reARM
	case IBV_EVENT_LID_CHANGE:
           // \TODO exchange valid dlid updates with connection partner; and then reARM??????
	case IBV_EVENT_PKEY_CHANGE:
           // \TODO Rearm?  ensure pkey of path and alt path are the same
        case IBV_EVENT_GID_CHANGE:
           // \TODO GID change--impact?  How does this affect an existing connection or switchover?
	case IBV_EVENT_SM_CHANGE:
        case IBV_EVENT_CLIENT_REREGISTER:
	    //SM change and reregister should be noted but no actions
            LOG(txp,always)<< "port="<<l_asyncEvent.element.port_num<<" event="<< l_asyncEvent.event_type<<":"<< ibv_event_type_str(l_asyncEvent.event_type);
            break;
        default:
          abort();
          break;
     }
     ibv_ack_async_event(&l_asyncEvent);
   }


   return 0;
}

int RDMACMfship::setAsyncPollFD(struct rdma_cm_id * pCmID){
   LOG(txp,always)<< "Enter setAsyncPollFD cmid="<< pCmID;
   if (!pCmID) return EFAULT;
   if (!pCmID->verbs) return ENODEV;
   int flags = fcntl(pCmID->verbs->async_fd, F_GETFL);
   if (flags==-1){
      LOG(txp,always)<< "error async F_GETFL " << pCmID->verbs->async_fd<< " showsflags " <<" errno="<<errno<<
                    std::hex << flags <<std::dec;
      return errno;
   }
   flags |= O_NONBLOCK;
   int rc = fcntl(pCmID->verbs->async_fd, F_SETFL, flags);

   if (rc) {
       LOG(txp,always)<< "error changing async fd " << pCmID->verbs->async_fd<< " non-blocking mode using flags " << " errno="<<errno<<
                    std::hex << flags <<std::dec;
       return errno;
   }
   _pollInfo[asyncEvent].fd = pCmID->verbs->async_fd;
   rc = handleAsyncEvent(_cmId);
   if (rc) LOG(txp,always)<<"rc="<<rc;
   LOG(txp,always)<<  "_pollInfo[asyncEvent].fd="<<_pollInfo[asyncEvent].fd ;
   return 0;
}

int RDMACMfship::prepareClient(){
  int err = createEventChannel();
  if (err) return err;
  err = createId(_cmId);
  if (err) return err;
  // Resolve the local and remote IP addresses to rdma addresses.
  err = resolveAddress(&_localAddress, &_remoteAddress);
  if (err) return err;
  err = resolveRoute();
  if (err) return err;
  printPath(*_cmId);
  /* now know within connection ID which device is being used.  So can set protection domain */
  _pd = ibv_alloc_pd(_cmId->verbs);
  if (!_pd){
     LOG(txp,warning)<< "failed to get protection domain "<<__PRETTY_FUNCTION__<<":"<< __LINE__;
     return ENOSYS;
  }
  _completionChannel = ibv_create_comp_channel(_cmId->verbs);

  setNonBlockMode(1);//nonblocking fd
  _pollInfo[completionChannel].fd = _completionChannel->fd; //will poll on this file descriptor
  _pollInfo[completionChannel].events = POLLIN;
  _pollInfo[completionChannel].revents = 0;
  LOG(txp,always)<<"_completionChannel isNonBlockMode="<<isNonBlockMode()<<" on fd="<<_completionChannel->fd ;

  _sendReceiveCQ = ibv_create_cq(_cmId->verbs, _cqEntries, this, _completionChannel,0);
  err = ibv_req_notify_cq(_sendReceiveCQ,  0);//completion channel is active for cq events
  if (err){
    LOG(txp,warning)<< "ibv_req_notify_cq failed "<<__PRETTY_FUNCTION__<<":"<< __LINE__<<" errno="<<err<<", "<<strerror(err);
    return err;
  }
  err = createQp(1);//always generate work completion for work requests to the send queue
  if (err) return err;
  _regSendMemChunksPtr = new MemChunkRDMA(_maxSendWorkRequests, _sendWorkRequestChunkSize, _cmId->pd);
  _regRDMAChunksPtr    = new MemChunkRDMA(_maxRDMAchunks, _RDMAchunkSize, _cmId->pd);
  err = setUpRcvWorkRequests();
  return err;
}

int
RDMACMfship::setNonBlockMode(bool mode)
{
   int flags = fcntl(_completionChannel->fd, F_GETFL);
   if (mode) {
      flags |= O_NONBLOCK;
      LOG(txp,always)<<"turning on non-blocking mode for completion channel with fd " << _completionChannel->fd;
   }
   else {
      flags &= ~(O_NONBLOCK);
      LOG(txp,always)<<"turning off non-blocking mode for completion channel with fd " << _completionChannel->fd;
   }
   int rc = fcntl(_completionChannel->fd, F_SETFL, flags);
   if (rc != 0) {
      // e(errno, "fcntl() failed");
      LOG(txp,warning)<< "error changing completion channel with fd " << _completionChannel->fd << " non-blocking mode using flags " <<
                    std::hex << flags <<std::dec;
      return errno;
   }

   return 0;
}

bool
RDMACMfship::isNonBlockMode(void)
{
   int flags = fcntl(_completionChannel->fd, F_GETFL);
   return (flags & O_NONBLOCK ? true : false);
}


// signalSendQueue (sq_sig_all)
// 0 User must decide whether to generate a work completion for successful completions
// o.w. All work requests to the Send queue generate a work completion (and take up CPU bandwidth...)
int RDMACMfship::createQp( bool signalSendQueue)
{
   // Create a queue pair.

   memset(&_qp_init_attr, 0, sizeof _qp_init_attr);
   _qp_init_attr.cap.max_send_wr = 128; // \TODO config
   _qp_init_attr.cap.max_recv_wr = _maxRecvWorkRequests; // \TODO config
   _qp_init_attr.cap.max_send_sge = 4;
   _qp_init_attr.cap.max_recv_sge = 4;

   _qp_init_attr.cap.max_inline_data = 0; // \TODO can this be done on Power?

   _qp_init_attr.qp_context = this; // Save the pointer this object.
   _qp_init_attr.sq_sig_all = signalSendQueue;
   _qp_init_attr.qp_type = IBV_QPT_RC;
   _qp_init_attr.send_cq = _sendReceiveCQ;
   _qp_init_attr.recv_cq = _sendReceiveCQ;
   int rc = rdma_create_qp(_cmId, _pd, &_qp_init_attr);//note that if pd is NULL, default protection domain into _cmId
   if (rc != 0) {
      LOG(txp,warning)<< "rdma_create_qp() failed "<<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
      return errno;
   }
   LOG(txp,always) << "created queue pair " << _cmId->qp->qp_num;
   return 0;
}

int
RDMACMfship::resolveAddress(struct sockaddr_in *localAddr, struct sockaddr_in *remoteAddr)
{
   // Resolve the addresses.
   int rc = rdma_resolve_addr(_cmId, (struct sockaddr *)localAddr, (struct sockaddr *)remoteAddr, _timeout_ms4resolveAddr);
   if (rc != 0) {
       LOG(txp,warning)<< "error resolving remote address " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
      return errno;
   }

   // Wait for ADDR_RESOLVED event.
   int err = waitForEvent();
   if (err != 0) {
      LOG(txp,warning)<< "waiting for ADDR_RESOVLED event failed";
      return err;
   }
   if (_event->event != RDMA_CM_EVENT_ADDR_RESOLVED) {
      LOG(txp,warning)<<  "wrong event " << rdma_event_str(_event->event)<< " is not rdma_event_str(RDMA_CM_EVENT_ADDR_RESOLVED";
      errno=EINVAL;
      return errno;
   }

   // Acknowledge the ADDR_RESOLVED event.
   err = ackEvent();
   if (err != 0) {
      LOG(txp,warning) << "error acking " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< rdma_event_str(_event->event);
      return err;
   }
   LOG(txp,always)<< "resolved to address " << addressToString(&_remoteAddress);

   return 0;
}

int
RDMACMfship::waitForConnectRequest()
{

   // RDMA_CM_EVENT_CONNECT_REQUEST.
   int err = waitForEvent();
   if (err != 0) {
      LOG(txp,error)<< "waiting for RDMA_CM_EVENT_CONNECT_REQUEST event failed";
      return err;
   }
   if (_event->event != RDMA_CM_EVENT_CONNECT_REQUEST) {
      LOG(txp,error)<<  "wrong event " << rdma_event_str(_event->event)<< " is not rdma_event_str(RDMA_CM_EVENT_CONNECT_REQUEST";
      errno=EINVAL;
      return errno;
   }
   else { //update RDMA CM ID and close listener;
     //need to update cmID for the connection
      // \TODO in destructor rdma_destroy_id(_cmId );
      _cmId =_event->id;
      struct rdma_conn_param * conn = &_event->param.conn;//valid for CONNECT_REQUEST and ESTABLISHED
      if (conn->private_data_len){
         LOG(txp,always)<<"RDMA_CM_EVENT_CONNECT_REQUEST private data in rdma_conn_param";

         lidStruct* dataArea =  (lidStruct * )conn->private_data;
         if (_dlidList) {
            delete [] _dlidList;
         }
         _dlidNum = dataArea->numLids;
         LOG(txp,always)<<"dataArea.numLids="<<dataArea->numLids;
         if (_dlidNum) _dlidList = new uint16_t[_dlidNum];
         for (int i=0;i<_dlidNum;i++){
           _dlidList[i]=dataArea->lid[i];
           LOG(txp,always)<<"dlid index="<<i<<" dlid="<<_dlidList[i];
         }
      }
   }

   // Acknowledge the RDMA_CM_EVENT_CONNECT_REQUEST event.  FREES the _event data and becomes NULL;
   err = ackEvent();
   if (err != 0) {
      LOG(txp,warning) << "error acking " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< rdma_event_str(_event->event);
      return err;
   }
   LOG(txp,always)<< "RDMA_CM_EVENT_CONNECT_REQUEST";

   return 0;
}

std::string
RDMACMfship::addressToString(const struct sockaddr_in *address) const
{
   std::ostringstream addr;
   char addrbuf[INET_ADDRSTRLEN];
   inet_ntop(AF_INET, &(address->sin_addr.s_addr), addrbuf, INET_ADDRSTRLEN);
   addr << addrbuf << ":" << address->sin_port;
   return addr.str();
}

int
RDMACMfship::waitForEvent(void)
{
   // This operation can block if there are no pending events available.
   LOG(txp,always) << "waiting for rdma cm event on event channel with fd " << _eventChannel->fd << " ...";

   _event=NULL;
   int err = rdma_get_cm_event(_eventChannel, &_event);
   if (!err) {
        LOG(txp,info)<< "cmid=0x"<<std::hex<<_event->id<<" listen_id=0x"<<_event->listen_id<<std::dec<<" status="<<_event->status<<" waitForEvent event=" << rdma_event_str(_event->event);
       return 0;
   }
   int rc=pollEventIn();
   if (rc<=0) return abs(rc);
    _event=NULL;
   err = rdma_get_cm_event(_eventChannel, &_event);
   if (err != 0) {
      LOG(txp,warning) << "rdma_get_cm_event " <<__PRETTY_FUNCTION__<<":"<< __LINE__<<" errno="<<errno<<":"<<strerror(errno);
      return err;
   }
   else{
     LOG(txp,info)<< "cmid=0x"<<std::hex<<_event->id<<" listen_id=0x"<<_event->listen_id<<std::dec<<" status="<<_event->status<<" waitForEvent event=" << rdma_event_str(_event->event);
   }
   return 0;
}

int  RDMACMfship::pollEventIn(const int timeout){
	if (-1==_pollInfo[eventChannel].fd){
		errno=ENOTCONN;
		return -1;
	}
	const int numFds=1;
	_pollInfo[eventChannel].events=POLLIN;
        _pollInfo[eventChannel].revents=0;
        int rc = ::poll(&_pollInfo[eventChannel], numFds, timeout);
        if (rc>0) {
           if (_pollInfo[completionChannel].revents & (POLLERR|POLLHUP|POLLNVAL) ) return -ENOTCONN;
           return rc;
        }
	if (rc==0) return -ETIME;
	if (rc==(-1) ) return -errno;
        return -1;
}

int
RDMACMfship::ackEvent(void)
{
   if (_event == NULL) {
      LOG(txp,warning) << "no rdma cm event available to ack";
      return ENOENT;
   }

   LOG(txp,always) <<  "acking rdma cm event " << rdma_event_str(_event->event) << " (" << _event->event << ") for rdma cm id " << _event->id;
   int err = rdma_ack_cm_event(_event);
   if (err != 0) {
      err = abs(err);
      LOG(txp,warning) << "rdma_ack_cm_event " <<__PRETTY_FUNCTION__<<":"<< __LINE__<<" err="<<err;
      return err;
   }

   _event = NULL;
   return 0;
}

int
RDMACMfship::resolveRoute(void)
{
   // Resolve a route.
   int rc = rdma_resolve_route(_cmId, _timeout_ms4resolveAddr); // Configurable timeout?
   if (rc != 0) {
      LOG(txp,warning) <<  "error resolving route: " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
      return errno;
   }

   // Wait for ROUTE_RESOLVED event.
   int err = waitForEvent();
   if (err != 0) {
      return err;
   }
   if (_event->event != RDMA_CM_EVENT_ROUTE_RESOLVED) {
      LOG(txp,warning) << "event " << rdma_event_str(_event->event) << " is not RDMA_CM_EVENT_ROUTE_RESOLVED";
      return EINVAL;
   }

   // Acknowledge the ROUTE_RESOLVED event.
   err = ackEvent();
   if (err != 0) {
      LOG(txp,warning) <<  "error with act to ROUTE_RESOLVED";
      return err;
   }
   LOG(txp,always) <<  "resolved route to " << addressToString(&_remoteAddress);

   return 0;
}

int
RDMACMfship::disconnect()
{
  int rc = rdma_disconnect (_cmId);
  if (rc != 0) {
      LOG(txp,warning) <<  "error rdma_disconnect: " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
      return errno;
   }
   LOG(txp,always) <<  "disconnecting _cmId="<<_cmId;
   return 0;
}

int
RDMACMfship::connect2Remote(void)
{
   // Connect to the server.
   struct rdma_conn_param param;
   memset(&param, 0, sizeof(param));
   doPortQuery();//sets _slidNum and _slidList;

/* responder_resources:  Specifies the maximum number of outstanding Remote Direct Memory Access (RDMA) read operations that the local side accepts from the remote side. This property applies only to the RDMA_PS_TCP event. The responder_resources value must be less than or equal to the local RDMA device attribute max_qp_rd_atom and to the remote RDMA device attribute max_qp_init_rd_atom. The remote endpoint can adjust this value when accepting the connection.
http://lxr.free-electrons.com/source/drivers/infiniband/core/cma.c#L827
826 static int cma_modify_qp_rtr(struct rdma_id_private *id_priv,
827                              struct rdma_conn_param *conn_param)
...
861    if (conn_param)
862                 qp_attr.max_dest_rd_atomic = conn_param->responder_resources;
*/
//
/* initiator_depth:  Specifies the maximum number of outstanding RDMA read operations that the local side must read to the remote side. This property applies only to the RDMA_PS_TCP event. The initiator_depth value must be less than or equal to the local RDMA device attribute max_qp_init_rd_atom and to the remote RDMA device attribute max_qp_rd_atom. The remote endpoint can adjust to this value when accepting the connection.
http://lxr.free-electrons.com/source/drivers/infiniband/core/cma.c#L870
869 static int cma_modify_qp_rts(struct rdma_id_private *id_priv,
870                              struct rdma_conn_param *conn_param)
...
886         if (conn_param)
887                 qp_attr.max_rd_atomic = conn_param->initiator_depth;
*/
   //param.responder_resources = 1;
   //param.initiator_depth = 1;
   param.responder_resources = 16;
   param.initiator_depth = 16;
   lidStruct dataArea;
   param.private_data_len = sizeof(dataArea);

   dataArea.numLids=_slidNum;
   LOG(txp,always)<<"dataArea.numLids="<<dataArea.numLids;
   for (int i=0; i<dataArea.numLids;i++){
      dataArea.lid[i] = _slidList[i];
      LOG(txp,always)<<"rdma_connect dataArea.lid[i]="<<dataArea.lid[i];
   }
   param.private_data = (void *)&dataArea;
   param.retry_count = 5;  //retry happens when retry timer pops for not getting a response from remote destination QP RQ logic
   //rnr retry of 7 means infinite
   // RNRs happen when a send request is posted before a corresponding receive request is posted on the peer.
   param.rnr_retry_count = 6; //if there was no receive posted on remote, error out after 6 timeouts

   int rc = rdma_connect(_cmId, &param);
   if (rc != 0) {
      LOG(txp,warning) <<  "rdma connect error " <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno)<<"to:"<<addressToString(&_remoteAddress);
      return errno;
   }
   int err= waitForEstablished();
   if (err) return err;
   err =setAsyncPollFD(_cmId);
   if (err) return err;
   err=armAPM();
   if (err)  if (err) LOG(txp,always)<<"acceptRemote method armAPM err="<<err;
   return 0; //log armAPM error but keep going
}

// \todo figure out why enum b_cm_rej_reason is in per se instead of included as .h
enum ib_cm_rej_reason {
	IB_CM_REJ_NO_QP				= 1,
	IB_CM_REJ_NO_EEC			= 2,
	IB_CM_REJ_NO_RESOURCES			= 3,
	IB_CM_REJ_TIMEOUT			= 4,
	IB_CM_REJ_UNSUPPORTED			= 5,
	IB_CM_REJ_INVALID_COMM_ID		= 6,
	IB_CM_REJ_INVALID_COMM_INSTANCE		= 7,
	IB_CM_REJ_INVALID_SERVICE_ID		= 8,
	IB_CM_REJ_INVALID_TRANSPORT_TYPE	= 9,
	IB_CM_REJ_STALE_CONN			= 10,
	IB_CM_REJ_RDC_NOT_EXIST			= 11,
	IB_CM_REJ_INVALID_GID			= 12,
	IB_CM_REJ_INVALID_LID			= 13,
	IB_CM_REJ_INVALID_SL			= 14,
	IB_CM_REJ_INVALID_TRAFFIC_CLASS		= 15,
	IB_CM_REJ_INVALID_HOP_LIMIT		= 16,
	IB_CM_REJ_INVALID_PACKET_RATE		= 17,
	IB_CM_REJ_INVALID_ALT_GID		= 18,
	IB_CM_REJ_INVALID_ALT_LID		= 19,
	IB_CM_REJ_INVALID_ALT_SL		= 20,
	IB_CM_REJ_INVALID_ALT_TRAFFIC_CLASS	= 21,
	IB_CM_REJ_INVALID_ALT_HOP_LIMIT		= 22,
	IB_CM_REJ_INVALID_ALT_PACKET_RATE	= 23,
	IB_CM_REJ_PORT_CM_REDIRECT		= 24,
	IB_CM_REJ_PORT_REDIRECT			= 25,
	IB_CM_REJ_INVALID_MTU			= 26,
	IB_CM_REJ_INSUFFICIENT_RESP_RESOURCES	= 27,
	IB_CM_REJ_CONSUMER_DEFINED		= 28,
	IB_CM_REJ_INVALID_RNR_RETRY		= 29,
	IB_CM_REJ_DUPLICATE_LOCAL_COMM_ID	= 30,
	IB_CM_REJ_INVALID_CLASS_VERSION		= 31,
	IB_CM_REJ_INVALID_FLOW_LABEL		= 32,
	IB_CM_REJ_INVALID_ALT_FLOW_LABEL	= 33
};


std::string status2String(int status){
  std::ostringstream os;
  switch(status){


         case IB_CM_REJ_NO_QP                 : os <<"IB_CM_REJ_NO_QP";           break;
         case IB_CM_REJ_NO_EEC                : os <<"IB_CM_REJ_NO_EEC";          break;
         case IB_CM_REJ_NO_RESOURCES          : os <<"IB_CM_REJ_NO_RESOURCES";    break;
         case IB_CM_REJ_TIMEOUT               : os <<"IB_CM_REJ_TIMEOUT";         break;
         case IB_CM_REJ_UNSUPPORTED           : os <<"IB_CM_REJ_UNSUPPORTED";     break;
         case IB_CM_REJ_INVALID_COMM_ID       : os <<"IB_CM_REJ_INVALID_COMM_ID";        break;
         case IB_CM_REJ_INVALID_COMM_INSTANCE : os << "IB_CM_REJ_INVALID_COMM_INSTANCE"; break;
         case IB_CM_REJ_INVALID_SERVICE_ID    : os <<"IB_CM_REJ_INVALID_SERVICE_ID (no listener)";          break;
         case IB_CM_REJ_INVALID_TRANSPORT_TYPE: os <<"IB_CM_REJ_INVALID_TRANSPORT_TYPE";                    break;
         case IB_CM_REJ_STALE_CONN            : os <<"IB_CM_REJ_STALE_CONN";          break;
         case IB_CM_REJ_RDC_NOT_EXIST         : os <<"IB_CM_REJ_RDC_NOT_EXIST";       break;
         case IB_CM_REJ_INVALID_GID           : os <<"IB_CM_REJ_INVALID_GID";         break;
         case IB_CM_REJ_INVALID_LID           : os <<"IB_CM_REJ_INVALID_LID";         break;
         case IB_CM_REJ_INVALID_SL            : os <<"IB_CM_REJ_INVALID_SL";          break;
         case IB_CM_REJ_INVALID_TRAFFIC_CLASS : os <<"IB_CM_REJ_INVALID_TRAFFIC_CLASS";    break;
         case IB_CM_REJ_INVALID_HOP_LIMIT     : os <<"IB_CM_REJ_INVALID_HOP_LIMIT";        break;
         case IB_CM_REJ_INVALID_PACKET_RATE   : os <<"IB_CM_REJ_INVALID_PACKET_RATE";      break;
         case IB_CM_REJ_INVALID_ALT_GID       : os <<"IB_CM_REJ_INVALID_ALT_GID";          break;
         case IB_CM_REJ_INVALID_ALT_LID       : os <<"IB_CM_REJ_INVALID_ALT_LID";          break;
         case IB_CM_REJ_INVALID_ALT_SL        : os <<"IB_CM_REJ_INVALID_ALT_SL";           break;
         case IB_CM_REJ_INVALID_ALT_TRAFFIC_CLASS     : os <<"IB_CM_REJ_INVALID_ALT_TRAFFIC_CLASS"; break;
         case IB_CM_REJ_INVALID_ALT_HOP_LIMIT         : os <<"IB_CM_REJ_INVALID_ALT_HOP_LIMIT";     break;
         case IB_CM_REJ_INVALID_ALT_PACKET_RATE       : os <<"IB_CM_REJ_INVALID_ALT_PACKET_RATE";   break;
         case IB_CM_REJ_PORT_CM_REDIRECT              : os <<"IB_CM_REJ_PORT_CM_REDIRECT";          break;
         case IB_CM_REJ_PORT_REDIRECT                 : os <<"IB_CM_REJ_PORT_REDIRECT";             break;
         case IB_CM_REJ_INVALID_MTU                   : os <<"IB_CM_REJ_INVALID_MTU";               break;
         case IB_CM_REJ_INSUFFICIENT_RESP_RESOURCES   : os <<"IB_CM_REJ_INSUFFICIENT_RESP_RESOURCES"; break;
         case IB_CM_REJ_CONSUMER_DEFINED              : os <<"IB_CM_REJ_CONSUMER_DEFINED (remote died/cored)";          break;
         case IB_CM_REJ_INVALID_RNR_RETRY             : os <<"IB_CM_REJ_INVALID_RNR_RETRY";         break;
         case IB_CM_REJ_DUPLICATE_LOCAL_COMM_ID       : os <<"IB_CM_REJ_DUPLICATE_LOCAL_COMM_ID";   break;
         case IB_CM_REJ_INVALID_CLASS_VERSION         : os <<"IB_CM_REJ_INVALID_CLASS_VERSION";     break;
         case IB_CM_REJ_INVALID_FLOW_LABEL            : os <<"IB_CM_REJ_INVALID_FLOW_LABEL";        break;
         case IB_CM_REJ_INVALID_ALT_FLOW_LABEL        : os <<"IB_CM_REJ_INVALID_ALT_FLOW_LABEL";    break;
        default: os<<"Unknown status code"; break;
   }
   return os.str();
}

int RDMACMfship::waitForEstablished(){
    // Wait for ESTABLISHED event.
   int err = waitForEvent();
   if (err != 0) {
      return err;
   }
   if (_event->event != RDMA_CM_EVENT_ESTABLISHED) {//usually RDMA_CM_EVENT_REJECTED for timeout or no listener
      LOG(txp,error) <<  "event " << rdma_event_str(_event->event) << " is not " << rdma_event_str(RDMA_CM_EVENT_ESTABLISHED);
      LOG(txp,error) << status2String(_event->status)<<" status="<<_event->status<<" event " << rdma_event_str(_event->event);
   }
   else {
     LOG(txp,always)<<"RDMA_CM_EVENT_ESTABLISHED localport="<<getLocalPortCM(_cmId)<<" remoteport="<<getRemotePort(_cmId);
     struct rdma_conn_param * conn = &_event->param.conn;//valid for CONNECT_REQUEST and ESTABLISHED
if (conn->private_data_len){
         LOG(txp,always)<<"RDMA_CM_EVENT_ESTABLISHED private data in rdma_conn_param";

         lidStruct* dataArea =  (lidStruct * )conn->private_data;
         if (_dlidList) {
            delete [] _dlidList;
         }
         _dlidNum = dataArea->numLids;
         LOG(txp,always)<<"dataArea.numLids="<<dataArea->numLids;
         if (_dlidNum) _dlidList = new uint16_t[_dlidNum];
         for (int i=0;i<_dlidNum;i++){
           _dlidList[i]=dataArea->lid[i];
           LOG(txp,always)<<"dlid index="<<i<<" dlid="<<_dlidList[i];
         }
      }
   }
   // Acknowledge the ESTABLISHED event.
   err = ackEvent();
   if (err != 0) {
      return err;
   }

   LOG(txp,always) << "connected to " << addressToString(&_remoteAddress);
   return 0;

}

int
RDMACMfship::bindCnxSock()
{
   int err = rdma_bind_addr(_cmIdListener, (struct sockaddr *)&_localAddress);
   if (err != 0) {
      LOG(txp,warning) << "error binding to address " << addressToString(&_localAddress) <<" errno="<<errno<<", "<<strerror(errno);
      //abort();
      return errno;
   }
   if (_localAddress.sin_port){
      LOG(txp,always) <<"rdma cm id="<<_cmIdListener<<" bound rdma cm id to address " << addressToString(&_localAddress)<<" input port="<< _localAddress.sin_port ;
   }
   else {
      LOG(txp,always) <<"rdma cm id="<<_cmIdListener<<" bound rdma cm id to address " << addressToString(&_localAddress)<<" input port=generic";// \TODO get the generic port!!!!!!
   }
   return 0;
}

int
RDMACMfship::listen(int backlog)
{
   // Start listening for connections.
   int err = rdma_listen(_cmIdListener, backlog);
   if (err) {
      LOG(txp,error) <<  "rdma_listen" <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno);
      return errno;
   }
   LOG(txp,always) <<  "rdma_listen cm id=" << _cmIdListener;
   return 0;
}

int RDMACMfship::setUpRcvWorkRequests(){
      uint64_t memLength = (uint64_t)_maxRecvWorkRequests * _recvWorkRequestChunkSize;
      char * lMemPtr = (char*)mmap(NULL, memLength,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS|MAP_LOCKED,0,0);
      if (lMemPtr==MAP_FAILED){
        return ENOMEM;
      }
      _regRcvMemPtr = ibv_reg_mr(_cmId->pd, lMemPtr, memLength, IBV_ACCESS_LOCAL_WRITE);//Local write needed for receive requests
      LOG(txp,always)<<"_regRcvMemPtr="<<std::hex<<_regRcvMemPtr<<"reg address="<<_regRcvMemPtr->addr <<" lkey="<<_regRcvMemPtr->lkey<<std::dec<<" length="<<_regRcvMemPtr->length<<std::hex<<" _recvWorkRequestChunkSize="<<_recvWorkRequestChunkSize<<std::dec<<" _maxRecvWorkRequests="<<_maxRecvWorkRequests;
      struct ibv_recv_wr * lBadWorkRequest;

      struct ibv_sge lSge;
      lSge.lkey = _regRcvMemPtr->lkey;
      lSge.addr = (uint64_t)lMemPtr;
      lSge.length = _recvWorkRequestChunkSize;

      struct ibv_recv_wr lWorkRequest;
      lWorkRequest.wr_id=lSge.addr;
      lWorkRequest.next=NULL; //doing one at a time, no linked list here
      lWorkRequest.sg_list=&lSge;
      lWorkRequest.num_sge=1;

      for(unsigned int i=0; i<_maxRecvWorkRequests;i++){
        int errnoPostRecv = ibv_post_recv(_cmId->qp, &lWorkRequest,&lBadWorkRequest);
        FL_Write6(FLTxp, TXP_PRCV0, "PostRecv0 wr_id=0x%16X, address=0x%llx, lkey=0x%llx, _regRcvMemPtr->lkey=0x%llx length=%lld ChunkSize=%lld errno=%lld", lWorkRequest.wr_id, lSge.addr, lSge.lkey,_regRcvMemPtr->lkey,lSge.length,errnoPostRecv);
        if (errnoPostRecv){
           LOG(txp,error)<<"Original ibv_post_recv wr_id="<<std::hex<<lWorkRequest.wr_id<<" address="<<lSge.addr<<" lkey="<<lSge.lkey<<std::dec<<" length="<<lSge.length<<" errnoPostRecv="<<errnoPostRecv<<":"<<strerror(errnoPostRecv);
           return errnoPostRecv;
        }
        lSge.addr += lSge.length;
        lWorkRequest.wr_id=lSge.addr;
      }

      return 0;
}

int RDMACMfship::checkBadRecvAddr(void * pAddr)const{
  uint64_t base= (uint64_t)_regRcvMemPtr->addr;
  uint64_t memLength = (uint64_t)_regRcvMemPtr->length;
  uint64_t end = base+memLength;
  uint64_t val = (uint64_t)pAddr;
  if (  val < base) {abort(); return -EFAULT;}
  if (val > end) {abort(); return -EINVAL; }
  return 0;

}

int RDMACMfship::putBackRcvWorkRequest(char* pAddr)const {
      if ( checkBadRecvAddr(pAddr) ) abort();
      struct ibv_recv_wr * lBadWorkRequest;

      struct ibv_sge lSge;
      lSge.lkey = _regRcvMemPtr->lkey;
      lSge.addr = (uint64_t)pAddr;
      lSge.length = _recvWorkRequestChunkSize;

      struct ibv_recv_wr lWorkRequest;
      lWorkRequest.wr_id=lSge.addr;
      lWorkRequest.next=NULL; //doing one at a time, no linked list here
      lWorkRequest.sg_list=&lSge;
      lWorkRequest.num_sge=1;
      int errnoPostRecv = ibv_post_recv(_cmId->qp, &lWorkRequest,&lBadWorkRequest);
      FL_Write6(FLTxp, TXP_PRECV, "PostRecv wr_id=0x%llX, address=0x%llx, lkey=0x%llx, _regRcvMemPtr->lkey=0x%llx length=%lld ChunkSize=%lld errno=%lld", lWorkRequest.wr_id, lSge.addr, lSge.lkey,_regRcvMemPtr->lkey,lSge.length,errnoPostRecv);

      if (errnoPostRecv){
           LOG(txp,always)<<"Repost ibv_post_recv wr_id="<<std::hex<<lWorkRequest.wr_id<<" address="<<lSge.addr<<" lkey="<<lSge.lkey<<std::dec<<" length="<<lSge.length<<" errnoPostRecv="<<errnoPostRecv<<":"<<strerror(errnoPostRecv);
           return errnoPostRecv;
      }

      return 0;
}

struct ibv_cq * RDMACMfship::reArmCQ(){
   // Get the notification event from the completion channel.
   struct ibv_cq * cq = NULL;
   void *CQcontext = NULL;
   if (ibv_get_cq_event(_completionChannel, & cq, &CQcontext) != 0) {
      if (errno != EAGAIN) LOG(txp,warning) <<  "ibv_get_cq_event" <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errno<<", "<<strerror(errno)<<" fd=" << _completionChannel->fd;
      // \todo start teardown?
   }
   else {
      _numEvents++;//count to ack later via ibv_req_notify_cq
   }
   if (_numEvents >= 8) {
        ibv_ack_cq_events( cq, _numEvents);
        _numEvents=0;
   }
   int err = ibv_req_notify_cq( cq, 0);// solictedOnly=0 so completion event for any new CQE
   if (err) LOG(txp,warning)<< "ibv_req_notify_cq failed "<<__PRETTY_FUNCTION__<<":"<< __LINE__<<" errno="<<err<<", "<<strerror(err);

   return  cq;
}

/*
typedef struct cqe{
  ::memItem mi;
  struct ibv_wc wc;
} cqe;
*/
memItemPtr RDMACMfship::getReadChunk(){
    struct ibv_wc wc;
    bzero(&wc,sizeof(struct ibv_wc) );
    int numberOfCompletions = ibv_poll_cq(_sendReceiveCQ,1,&wc);
    while(numberOfCompletions>0){
        FL_Write6(FLTxp, TXP_WC, "wc.status=%lld, opcode=%lld, wr_id=0x%llx, vendor_err=%lld, _sendReceiveCQ=%llx, completions=%d", wc.status, wc.opcode, wc.wr_id, wc.vendor_err, (uint64_t)_sendReceiveCQ, numberOfCompletions);
       LOG(txp,debug)<<"wc.opcode="<< wc.opcode<< " status="<<wc.status<<"("<<ibv_wc_status_str(wc.status)<<") wr_id="<<std::hex<<wc.wr_id<<std::dec<<" vendor_err="<<wc.vendor_err;

      if (wc.status != IBV_WC_SUCCESS){
        LOG(txp,always)<< "status="<<wc.status<<"("<<ibv_wc_status_str(wc.status)<<") wr_id="<<std::hex<<wc.wr_id<<std::dec<<" vendor_err="<<wc.vendor_err<<" wc.byte_len="<< wc.byte_len;
        //log opcode, status,wr_id, qp_num, vendor_err
        //abort();
        //return NULL;
      }

      else if ( wc.opcode == IBV_WC_RECV  ){
          memItemPtr mi = new ::memItem( (char *)wc.wr_id, wc.byte_len, _recvWorkRequestChunkSize);
          mi->opcode=wc.opcode;
          //mi->next=NULL;

          return mi;
      }
      else if ( wc.opcode == IBV_WC_SEND){
            //put back send chunk using wr_id
            //lWorkRequest.wr_id = (uint64_t)(lMip) on ibv_post_send
            memItemPtr lMip = (memItemPtr) wc.wr_id;
            MemChunkRDMAptr mcrp = (MemChunkRDMAptr)lMip->handle;
            lMip->opcode=wc.opcode;
            mcrp->putChunk(lMip);
            //_regSendMemChunksPtr->putChunk(lMip);

            // \TODO maybe consider a separate completion queue and thread for send requests

      }
      else if ( wc.opcode == IBV_WC_RDMA_WRITE) {
         memItemPtr mi = (memItemPtr) wc.wr_id;
         mi->opcode=wc.opcode;
         return mi; //continue processing
       }
      else if ( wc.opcode == IBV_WC_RDMA_READ) {
         memItemPtr mi = (memItemPtr) wc.wr_id;
         mi->opcode=wc.opcode;
         return mi; //continue processing
       }
      else if ( wc.opcode == IBV_WC_RECV_RDMA_WITH_IMM ){
         abort();
      }
      else {
        LOG(txp,error)<<"wc.opcode="<< wc.opcode<< " status="<<wc.status<<"("<<ibv_wc_status_str(wc.status)<<") wr_id="<<std::hex<<wc.wr_id<<std::dec<<" vendor_err="<<wc.vendor_err<<" wc.byte_len="<< wc.byte_len;
        //log opcode, status,wr_id, qp_num, vendor_err
        abort();
        return NULL;
      }

      bzero(&wc,sizeof(struct ibv_wc) );
      numberOfCompletions = ibv_poll_cq(_sendReceiveCQ,1,&wc);
    }
    return NULL;
}

int RDMACMfship::rdmaReadFromTo(memItemPtr pRemoteMip,memItemPtr pRecvMsgMip){
     memItemPtr l_localMip4RDMA  =   pRecvMsgMip->next;
     if (!l_localMip4RDMA ) abort();
     struct ibv_send_wr lWorkRequest;
     struct ibv_send_wr * lBadWorkRequest;

     struct ibv_sge sge;
     sge.addr = (uintptr_t)l_localMip4RDMA->address;
     sge.length = l_localMip4RDMA->length;
     sge.lkey   = l_localMip4RDMA ->lkey;

     memset(&lWorkRequest,0,sizeof(lWorkRequest) );
     lWorkRequest.wr_id = (uint64_t)pRecvMsgMip;  //to re-incarnate original message from receive buffer
     lWorkRequest.next=NULL;  //last and only work request
     lWorkRequest.sg_list = &sge;
     lWorkRequest.num_sge = 1;
     lWorkRequest.opcode  = IBV_WR_RDMA_READ;
     lWorkRequest.send_flags = _write_send_flags; //want a cqe entry notification
     lWorkRequest.wr.rdma.remote_addr = (uint64_t)pRemoteMip->address;
     lWorkRequest.wr.rdma.rkey = pRemoteMip-> rkey;
     int errnoSend = ibv_post_send(_cmId->qp, &lWorkRequest, &lBadWorkRequest);
     FL_Write6(FLTxp, TXP_RDMAREAD, "remote@=%p local@=%p length=%x lkey=%x rkey=%x wr_id=%x",lWorkRequest.wr.rdma.remote_addr,sge.addr, sge.length, sge.lkey, lWorkRequest.wr.rdma.rkey, lWorkRequest.wr_id  );
     if (errnoSend) {
              LOG(txp,warning) <<  "ibv_post_send" <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoSend<<", "<<strerror(errnoSend);
              return  -errnoSend;
     }
     return 0;
}

int RDMACMfship::rdmaWriteToFrom(memItemPtr pRemoteMip,memItemPtr pRecvMsgMip){
     memItemPtr l_localMip4RDMA  =   pRecvMsgMip->next;
     if (!l_localMip4RDMA ) abort();
     struct ibv_send_wr lWorkRequest;
     struct ibv_send_wr * lBadWorkRequest;

     struct ibv_sge sge;
     sge.addr = (uintptr_t)l_localMip4RDMA->address;
     sge.length = l_localMip4RDMA->length;
     sge.lkey   = l_localMip4RDMA ->lkey;

     memset(&lWorkRequest,0,sizeof(lWorkRequest) );
     lWorkRequest.wr_id = (uint64_t)pRecvMsgMip;  //to re-incarnate original message from receive buffer
     lWorkRequest.next=NULL;  //last and only work request
     lWorkRequest.sg_list = &sge;
     lWorkRequest.num_sge = 1;
     lWorkRequest.opcode  = IBV_WR_RDMA_WRITE;
     lWorkRequest.send_flags = IBV_SEND_SIGNALED; //want a cqe entry notification
     lWorkRequest.wr.rdma.remote_addr = (uint64_t)pRemoteMip->address;
     lWorkRequest.wr.rdma.rkey = pRemoteMip-> rkey;
     int errnoSend = ibv_post_send(_cmId->qp, &lWorkRequest, &lBadWorkRequest);
     FL_Write6(FLTxp, TXP_RDMAWRITE, "remote@=%p local@=%p length=%x lkey=%x rkey=%x wr_id=%x",lWorkRequest.wr.rdma.remote_addr,sge.addr, sge.length, sge.lkey, lWorkRequest.wr.rdma.rkey, lWorkRequest.wr_id  );
     if (errnoSend) {
              LOG(txp,always) <<  "ibv_post_send" <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoSend<<", "<<strerror(errnoSend);
              return  -errnoSend;
     }
     return 0;
}

void RDMACMfship::freeReadChunk(memItemPtr& mip){
   if (mip->next)abort();
   int RC = RDMACMfship::putBackRcvWorkRequest(mip->address);
   if (RC) LOG(txp,warning) <<  "ibv_post_recv" <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<RC<<", "<<strerror(RC);
   if (mip->next)abort();
   delete mip;
   mip=NULL;
}


ssize_t RDMACMfship::write(txp::Msg* pMsg) {
        memItemPtr lMip =  _regSendMemChunksPtr->getChunk();
        return write(pMsg, lMip);

}

ssize_t RDMACMfship::write(txp::Msg* pMsg, memItemPtr mip){

	char l_MsgId[64] = {'\0'};
	pMsg->msgIdToChar(pMsg->getMsgId(), l_MsgId, sizeof(l_MsgId));

	pMsg->allocateHeapBuffer(mip->address,mip->chunkSize);
	int32_t l_DataLen = pMsg->serializeWithValuesToHeapBuffer();
        if (l_DataLen <= 0) abort();

        char * l_Data = pMsg->getDataBufferPtr();
        if (l_Data != mip->address) abort();

        struct ibv_send_wr * lBadWorkRequest;

        if (!mip->lkey) abort();

        struct ibv_sge lSge;
        lSge.lkey = mip->lkey;
        lSge.addr = (uint64_t)mip->address;
        lSge.length = l_DataLen;

        struct ibv_send_wr lWorkRequest;
        // http://manpages.ubuntu.com/manpages/saucy/man3/ibv_post_send.3.html
        lWorkRequest.wr_id = (uint64_t)(mip); //need for correlating
        lWorkRequest.next=NULL;  //last and only work request
        lWorkRequest.sg_list=&lSge;
        lWorkRequest.num_sge=1; //using a single sge
        lWorkRequest.opcode=IBV_WR_SEND;//imm_data unused
        lWorkRequest.imm_data= 0;

        lWorkRequest.send_flags =  _write_send_flags;

       FL_Write6(FLTxp, TXP_POST_SEND1, "wr_id=0x%08X, num_sge=%d, opcode=%d, _write_send_flags=0x%08X, imm_data=%d lkey=%d", lWorkRequest.wr_id, lWorkRequest.num_sge, lWorkRequest.opcode,  lWorkRequest.send_flags, lWorkRequest.imm_data,mip->lkey);

        int errnoSend = ibv_post_send(_cmId->qp, &lWorkRequest, &lBadWorkRequest);

        //l_RC2 = writeUnlock(pMsg);  if (l_RC2) printf("problem with writeUnlock");

	FL_Write6(FLTxp, TXP_SND1, "Msg#=0x%08X, RMsg#=0x%08X, MsgId=%lld, #Attrs=%d, errno=%d l_Datalen=%d", pMsg->getMsgNumber(), pMsg->getRequestMsgNumber(), pMsg->getMsgId(), pMsg->getNumberOfAttributes(), errnoSend,l_DataLen);
        if (errnoSend) {
             LOG(txp,warning) <<  "ibv_post_send" <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoSend<<", "<<strerror(errnoSend);
             abort();
             return (ssize_t) -errnoSend;
        }

	return  l_DataLen;

}


int RDMACMfship::read(txp::Msg* &pMsg, char* pDataBuffer, const size_t pDataBufferSize, const txp::DeserializeOption pDeserializeOption){
        ssize_t l_TotalMsgSize;
        int l_RC = txp::Msg::deserializeToMsg(pMsg, pDataBuffer, pDataBufferSize, l_TotalMsgSize, pDeserializeOption);
        errno = -l_RC;
        if (l_RC) {
           abort();
           if (pMsg) {
              delete pMsg;
              pMsg=NULL;
           }
        }
        return l_RC;

return 0;
}

ssize_t RDMACMfship::writeWithBuffer(txp::Msg* pMsg, memItemPtr mipWithData ) {

	char l_MsgId[64] = {'\0'};
	pMsg->msgIdToChar(pMsg->getMsgId(), l_MsgId, sizeof(l_MsgId));

       struct ibv_send_wr * lBadWorkRequest;
       struct ibv_send_wr lWorkRequest;
        // http://manpages.ubuntu.com/manpages/saucy/man3/ibv_post_send.3.html




        if (!mipWithData->length){

          return write(pMsg, mipWithData );// \TODO caller should check and instead use write(msg) and retain RDMA buffer??
        }
        else if (mipWithData->length > _recvWorkRequestChunkSize){//trivially, need to do an RDMA transfer
          return write(pMsg);
        }
        else {
         size_t l_msgLengthWithData = pMsg->getMsgLengthWithDataValues(); // \TODO remove hardcode of 64

         uint64_t offset=mipWithData->length & 63;
         if (offset) offset = 64 - offset;
         offset += mipWithData->length;

         if (l_msgLengthWithData + offset> _recvWorkRequestChunkSize) {
           return write(pMsg);
         }
         else{
            char * l_msgBuff = mipWithData->address + offset;
            uint32_t l_remains = mipWithData->chunkSize - offset;
            pMsg->allocateHeapBuffer(l_msgBuff,l_remains);
            int32_t l_DataLen = pMsg->serializeWithValuesToHeapBuffer(); // \TODO remove hardcode of 64
            if (l_DataLen <= 0) abort();

            if (!mipWithData->lkey) abort();

            struct ibv_sge lSge[2];
            lSge[1].lkey = mipWithData->lkey;
            lSge[1].addr = (uint64_t)mipWithData->address;
            lSge[1].length = mipWithData->length;

            lSge[0].lkey = mipWithData->lkey;
            lSge[0].addr = (uint64_t)l_msgBuff;
            lSge[0].length = l_DataLen;

            lWorkRequest.wr_id = (uint64_t)(mipWithData); //need for correlating
            lWorkRequest.next=NULL;  //last and only work request
            lWorkRequest.sg_list=&lSge[0];
            lWorkRequest.opcode=IBV_WR_SEND_WITH_IMM;
            if (mipWithData->length){
              lWorkRequest.num_sge=2; //using 2 of sge
              lWorkRequest.imm_data= 1;  // Data for write is within request
            }
            else {
              lWorkRequest.num_sge=1; //No data, using only for msg, so only one sge
              lWorkRequest.imm_data= 1;  // Data for write is within request
            }
            lWorkRequest.send_flags =  _write_send_flags;
            // libibverbs is thread safe, so no write lock required
            int errnoSend = ibv_post_send(_cmId->qp, &lWorkRequest, &lBadWorkRequest);
            FL_Write6(FLTxp, TXP_SNDP2, "Msg#=0x%08X, RMsg#=0x%08X, MsgId=0x%08X, #Attrs=%d, errno=%d l_Datalen=%d", pMsg->getMsgNumber(), pMsg->getRequestMsgNumber(), pMsg->getMsgId(), pMsg->getNumberOfAttributes(), errnoSend,l_DataLen);
            if (errnoSend) {
              LOG(txp,warning) <<  "ibv_post_send" <<__PRETTY_FUNCTION__<<":"<< __LINE__<< " errno="<<errnoSend<<", "<<strerror(errnoSend);
              return (ssize_t) -errnoSend;
            }
            return  l_DataLen;
          }
       }


	return 0;

}

int RDMACMfship::setDeviceAttr(){
   if (!_cmId) return EINVAL;
   if (!_cmId->verbs) return ENOTCONN;
   if (!_ibvDeviceAttrPtr) {
     _ibvDeviceAttrPtr = new(struct ibv_device_attr);
     int rcQueryDevice = ibv_query_device(_cmId->verbs, _ibvDeviceAttrPtr);
     if (rcQueryDevice) return rcQueryDevice;
   }
   return 0;
}

int RDMACMfship::canDoAPM(){
  int rc=0;
  rc=setDeviceAttr() ;
  if (rc ) {
     LOG(txp,always)<<"canDoAPM setDeviceAttr() rc="<<rc;
     return 0;
  }
  if ( ! (_ibvDeviceAttrPtr->device_cap_flags & IBV_DEVICE_AUTO_PATH_MIG) ){
      LOG(txp,always)<<"BV_DEVICE_AUTO_PATH_MIG is off flag is "<<  (int)(_ibvDeviceAttrPtr->device_cap_flags| IBV_DEVICE_AUTO_PATH_MIG);
      return 0;
  }
  return 1;
}

int RDMACMfship::doPortQuery()
{
    int rc = 0;

    if (!_ibvDeviceAttrPtr)
    {
        rc = setDeviceAttr();
    }

    if (!rc)
    {
        int queryErrnoPort = 0;
        int portNum=0;

        _slidNum = (int)_ibvDeviceAttrPtr->phys_port_cnt;
        if (!_slidList)
        {
            _slidList = new uint16_t[_slidNum];
        }

        if (canDoAPM())
        {
            if (!_portAttrArray)
            {
                _portAttrArray = new struct ibv_port_attr[(int)_ibvDeviceAttrPtr->phys_port_cnt];
            }

            for (int i=0; (!rc) && i<(int)_ibvDeviceAttrPtr->phys_port_cnt; i++)
            {
                portNum = i+1;
                queryErrnoPort = ibv_query_port(_cmId->verbs, portNum, _portAttrArray+i);
                if (queryErrnoPort)
                {
                    rc = queryErrnoPort;
                    LOG(txp,error) << "doPortQuery() portNum=" << portNum << ", queryErrnoPort=" << queryErrnoPort;
                }

                if (!rc)
                {
                    _slidList[i]=0;
                    if (_portAttrArray[i].state == IBV_PORT_ACTIVE)
                    {
                        _slidList[i] = _portAttrArray[i].lid;
                    }
                    LOG(txp,always) << "doPortQuery() index i=" << i << " _slidList[i]=" << _slidList[i] << " portNum=" << portNum;
                }
            }
        }
    }

    return rc;
}

int RDMACMfship::doQPquery(){
  if (!_QPattrPtr)_QPattrPtr= new  struct ibv_qp_attr;
  memset(_QPattrPtr,0,sizeof(struct ibv_qp_attr) );
  if (!_QPintattrPtr)_QPintattrPtr = new struct ibv_qp_init_attr;
  memset(_QPintattrPtr,0,sizeof(struct ibv_qp_init_attr) );
  int attr_mask = ALL_QP_ATT_MASK;
  int rcErrnoQueryQP = ibv_query_qp(_cmId->qp, _QPattrPtr, attr_mask, _QPintattrPtr);
  return rcErrnoQueryQP;
}


int dumpQPAttr(struct ibv_qp_attr* pAttr){
   LOG(txp,always)
    <<" qp_state="<<pAttr->qp_state
    <<" cur_qp_state="<<pAttr->cur_qp_state
    <<" path_mtu="<<pAttr->path_mtu
    <<" path_mig_state="<<pAttr->path_mig_state
    <<" qkey="<<pAttr->qkey
    <<" rq_psn="<<pAttr->rq_psn
    <<" sq_psn="<<pAttr->sq_psn
    ;
    dumpMigState(pAttr->path_mig_state);
    LOG(txp,always)
    <<" dest_qp_num="<<pAttr->dest_qp_num
    <<" qp_access_flags="<<pAttr->qp_access_flags
    <<" pkey_index="<<pAttr->pkey_index
    <<" alt_pkey_index="<<pAttr->alt_pkey_index
    <<" sq_draining="<<(int)pAttr->sq_draining
    ;
    LOG(txp,always)
    <<" max_rd_atomic="<<(int)pAttr->max_rd_atomic
    <<" max_dest_rd_atomic="<<(int)pAttr->max_dest_rd_atomic
    <<" min_rnr_timer="<<(int)pAttr->min_rnr_timer
    <<" port_num="<<(int)pAttr->port_num
    <<" timeout="<<(int)pAttr->timeout
    <<" retry_cnt="<<(int)pAttr->retry_cnt
    <<" rnr_retry="<<(int)pAttr->rnr_retry
    <<" alt_port_num="<<(int)pAttr->alt_port_num
    <<" alt_timeout="<<(int)pAttr->alt_timeout
    ;
   return 0;
}


int RDMACMfship::findAltPort(int currentPort){
   int indexInUse = currentPort-1;
   int altPort=0;
   for (int i=0; (i<(int)_ibvDeviceAttrPtr->phys_port_cnt) && (altPort==0);i++){
     if (i==indexInUse) continue;
     if (_portAttrArray[i].state==IBV_PORT_ACTIVE) altPort=(i+1);
   }
   return altPort;
}

int RDMACMfship::findAltdlid(int currentdlid){
   for (int i=0; i<_dlidNum;i++){
     if (_dlidList[i]==currentdlid)continue;
     if (_dlidList[i] ) return _dlidList[i];
   }
   return 0;
}

#if 0
enum ibv_mig_state {
	IBV_MIG_MIGRATED,
	IBV_MIG_REARM,
	IBV_MIG_ARMED
};
#endif

int RDMACMfship::forceMigrate(){
LOG(txp,always)<<"armAPM() enter";
struct ibv_qp_attr l_qp_attr;
memset(&l_qp_attr,0,sizeof(l_qp_attr) );
l_qp_attr.path_mig_state = IBV_MIG_MIGRATED;
int qpModifyErrno=ibv_modify_qp(_cmId->qp, &l_qp_attr, IBV_QP_PATH_MIG_STATE);
return qpModifyErrno;
}

int RDMACMfship::armAPM(){
   LOG(txp,always)<<"armAPM() enter";
   int RCerrno=doPortQuery();
   if (!RCerrno) RCerrno=doQPquery();
   if (RCerrno) {
      return RCerrno;
   }

   struct ibv_qp_attr l_qp_attr;
   int l_attr_mask=IBV_QP_ALT_PATH ;

   memcpy(&l_qp_attr.alt_ah_attr,&_QPattrPtr->ah_attr,sizeof(struct ibv_ah_attr) ); //copy primary path to proposed alt_path
   l_qp_attr.alt_pkey_index = _QPattrPtr->pkey_index;
   l_qp_attr.alt_timeout= _QPattrPtr->timeout;
   l_qp_attr.path_mig_state = IBV_MIG_REARM;
   LOG(txp,always)<<"ack timeout="<<(int)_QPattrPtr->timeout<<" ack retry count="<<(int)_QPattrPtr->retry_cnt<<" min_rnr_timer="<<(int)_QPattrPtr->min_rnr_timer<<" rnr_retry="<< (int)_QPattrPtr->rnr_retry ;

   int useAltPort=findAltPort(_QPattrPtr->port_num);
   if (useAltPort) {
      l_qp_attr.alt_ah_attr.port_num=useAltPort;
      l_qp_attr.alt_port_num=useAltPort;
      l_attr_mask |=  IBV_QP_PATH_MIG_STATE;
      LOG(txp,always)<<"mask IBV_QP_PATH_MIG_STATE added port="<<(int)l_qp_attr.alt_port_num;
   }
   else{
      l_qp_attr.alt_port_num=_QPattrPtr->port_num;
      l_qp_attr.alt_ah_attr.port_num=l_qp_attr.alt_port_num;
       LOG(txp,always)<<"alt port using original port_num="<<_QPattrPtr->port_num<<" l_qp_attr.port_num="<<(int)l_qp_attr.alt_port_num;
   }
   int altDlid = findAltdlid(l_qp_attr.alt_ah_attr.dlid);
   if (altDlid){
      l_qp_attr.alt_ah_attr.dlid=altDlid;
      l_attr_mask |=  IBV_QP_PATH_MIG_STATE;
      LOG(txp,always)<<"mask IBV_QP_PATH_MIG_STATE dlid="<<altDlid;
   }
   else {
     LOG(txp,always)<<"ZERO altdlid="<<altDlid<<" current value l_qp_attr.alt_ah_attr.dlid="<<l_qp_attr.alt_ah_attr.dlid;
   }
   if (l_attr_mask & IBV_QP_PATH_MIG_STATE) {
     int qpModifyErrno=ibv_modify_qp(_cmId->qp, &l_qp_attr, l_attr_mask);
     return qpModifyErrno;
   }
   return ENETDOWN;
}
