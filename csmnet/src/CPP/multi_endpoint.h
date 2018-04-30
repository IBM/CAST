/*================================================================================

    csmnet/src/CPP/multi_endpoint.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_MULTI_ENDPOINT_H_
#define CSMNET_SRC_CPP_MULTI_ENDPOINT_H_

#include <sys/epoll.h>

#include <map>
#include <unordered_set>

#include "endpoint_multi_unix.h"
#include "endpoint_pipe.h"
#include "message_ack.h"

// * manage multiple endpoints of the same type
// * handle state per endpoint (e.g. half-received messages)
// * notice/notify when endpoints disconnect/disappear
// * select the correct endpoint based on address field in header (i.e. next-hop routing)

namespace csm {
namespace network {

/** @def CSM_SEC_TIMEOUT
 * @brief Timeout specification for network operations in seconds
 */
#define CSM_SEC_TIMEOUT ( 60 )

/** @def CSM_MILLISEC_TIMEOUT
 * @brief Timeout specification for network operations in milliseconds
 */
#define CSM_MILLISEC_TIMEOUT ( CSM_SEC_TIMEOUT * 1000 )

/** @def CSM_MULTENDPOINT_MAX_EVENTS
 * @brief Maximim number of events that can get fetched by epoll
 */
#define CSM_MULTENDPOINT_MAX_EVENTS ( 128 )

/** @def CSM_NETWORK_EVENT_TIMEOUT
 * @brief epoll_wait timeout (in milli seconds) in case the netmgr is idle
 */
#define CSM_NETWORK_EVENT_TIMEOUT ( 1000 )

typedef std::pair< csm::network::AddressCode, csm::network::Endpoint *> EndpointTypePair;
typedef std::map<csm::network::AddressCode, csm::network::Endpoint*> EndpointList;

class EpollWrapper {

  int _CtrlSock;
  int _ActiveEvents;
  int _CurrentEvent;
  int _CompletedEvent;
  struct epoll_event _EpollEvents[ CSM_MULTENDPOINT_MAX_EVENTS ];
  uint32_t _EventMask;
  csm::network::Endpoint *_ActiveEP;
  uint32_t _ActiveEPevents;
  std::mutex _Lock;

public:
  EpollWrapper( const uint32_t aEventMask = EPOLLIN | EPOLLERR | EPOLLRDHUP );
  ~EpollWrapper();

  int GetEpollSocket() const { return _CtrlSock; };
  int Add ( const int aSocket, void *ptr );
  int Add( const csm::network::Endpoint *aEndpoint );
  int Del( const int aSocket );
  inline int Del( const csm::network::Endpoint *aEndpoint )
  {
    std::lock_guard<std::mutex> guard( _Lock );
    return DelNoLock( aEndpoint );
  }

  void* WaitEvent( const bool aBlock );
  csm::network::Endpoint * NextActiveEP( const bool aBlock = false, uint32_t *aEvents = nullptr );

  inline int AckEvent( )
  {
    std::lock_guard<std::mutex> guard( _Lock );
    return AckEventNoLock();
  }

  int FlushCurrentEvents();

private:
  int DelNoLock( const csm::network::Endpoint *aEndpoint );
  int AckEventNoLock();
  int CreateEpollSocket();

};

// - maintains a list of endpoints and provides a unified API to all of them
// - Endpoint selection via remote address (except for passive endpoints)
// - uses epoll to avoid unnecessary non-blocking recv calls
//
class MultiEndpoint : public csm::network::NetworkCtrlPath {
  EndpointList _EPL; // map of endpoints, key is created from type and address info
  EpollWrapper _Epoll;

  EndpointList _PassiveEPL;  // server listen-only endpoints
  EpollWrapper _PassiveEpoll;

  EndpointPipe *_OutboundPipe; // pipe descriptors to wake up for outbound activity
  EpollWrapper _ActivityEpoll; // collects the events from all endpoints, plus outbound activity signalling

  std::mutex _EndpointLock;

  friend class MultiEndpointTest;
protected:
  EndpointMultiUnix *_Unix;

public:
  MultiEndpoint( );
  MultiEndpoint( const MultiEndpoint * aMultiEndpoint );
  ~MultiEndpoint( ) noexcept(false);

  //////////////////////////////////////////
  // EndpointList maintenance

  // create and add a new endpoint to the list
  Endpoint* NewEndpoint( const Address_sptr aAddr, const EndpointOptions_sptr aOptions );
  Endpoint* NewEndpoint( Endpoint *aEndpoint );

  // remove and destroy endpoints
  int DeleteEndpoint( const Address *aAddr, const std::string &where = "" );
  int DeleteEndpoint( const Endpoint *aEndpoint, const std::string &where = "" );

  // wipe out all endpoints
  int Clear();

  // check for empty endpoint list
  bool Empty() const { return _EPL.empty() && _PassiveEPL.empty(); }


  //////////////////////////////////////////
  // Communication

  // accept any pending new connection requests
  // accepts, creates and inserts the new endpoint into list
  Endpoint* Accept( const bool aBlocking = false );

  // message based send
  ssize_t SendTo( const csm::network::MessageAndAddress &aMsgAddr );
  ssize_t SendTo( const csm::network::Message &aMsg,
                  const Address_sptr aRemoteAddr );

  // message based receive
  ssize_t RecvFrom( csm::network::MessageAndAddress &aMsgAddr );

  /* data synchronization, e.g. flush any buffers, send/recv pending requests, ... */
  int Sync( const SyncAction aSync = SYNC_ACTION_ALL );

  // retrieve access to particular endpoint based on given address
  // - allow upper layers to access endpoints directly to remove lookup
  //   requirements for each and every operation
  Endpoint* GetEndpoint( const csm::network::Address_sptr aAddress ) const;

  // this version of GetEndpoint will directly reach into the map without detour through MakeKey() of address
  // however, that limits the options to find Unix endpoints - so be careful
  Endpoint* GetEndpoint( const csm::network::AddressCode aKey ) const;

  int WaitForActivity();
  int WakeUp();

  // print a Dump() of all endpoints in the list
  void Log();
private:
  Endpoint* ProcessPassiveUnix( );
  Endpoint* ProcessPassive( csm::network::Endpoint *aEndpoint, const uint32_t aEvents );
  bool CheckAndAddEndpoint( csm::network::Endpoint *aEndpoint );
  int DeleteEndpointNoLock( const Address *aAddr, const std::string &where );

};


}  // namespace network
} // namespace csm

#endif /* CSMNET_SRC_CPP_MULTI_ENDPOINT_H_ */
