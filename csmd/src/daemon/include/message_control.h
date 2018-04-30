/*================================================================================

    csmd/src/daemon/include/message_control.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_MESSAGE_CONTROL_H_
#define CSMD_SRC_DAEMON_INCLUDE_MESSAGE_CONTROL_H_

#include <stack>
#include <unordered_map>
#include <mutex>
#include <queue>

#include "csm_timing.h"
#include "throttle.h"
#include "csm_core_event.h"
#include "include/virtual_network_channel.h"

// generate message IDs
// keep context of outgoing requests
// match message IDs

/*
 * Message ID handling
 *  - CSMI client uses messageID, but not globally unique (impossible to get that without special protocol)
 *  - daemon creates unique messageID by using client Address, global ID, and client's messageID
 *  - inbound requests generate the ID (if from a client) - this happens in "FindContext()"
 *  - outbound messages try to restore the client MsgID - this happens in "StoreContext()"
 *  - if request is single phase, handlers can preserve messageID
 *  - if handlers create new sub-requests, new messageID should to be created
 *    - handler sets messageID to 0
 *    - network layer generates messageID for outbound request
 *  - if handlers change messageID, they need to keep track of original request (think of onion)
 *  -   (todo: need function to push/pop message context/headers)
 */

/*
 * Context feature:
 *  - handlers can create/allocate arbitrary request contexts derived from EventContext class
 *  - assign this context to any of the returned events
 *  - if there's a response to that event, your handler will be called with access to that context
 */


namespace csm {
namespace daemon {

/* \brief garbage collection interval in seconds
 * determines the number of seconds to wait before the next garbage collection
 */
static const uint32_t DEFAULT_GC_INTERVAL = 5;

/* \brief if half of the msgID space is consumed within this time, we create a warning
 */
static const uint32_t MSGID_HALF_TIME_LIMIT = 300;

/* @brief number of bits of the daemonID prefix of a message ID
 */
#if (defined CSM_MULTI_COMPUTE_PER_NODE) || (defined CSM_MULTI_AGGREGATOR_PER_NODE)
static const uint64_t MSGID_BITS_PER_PID = 16;
static const uint64_t MSGID_BITS_PER_DAEMON_ID = 35ull;
#else
static const uint64_t MSGID_BITS_PER_PID = 6;
static const uint64_t MSGID_BITS_PER_DAEMON_ID = 32ull;
#endif

#define MSGID_START_RANDOM_MAX ( 16384 )

/* @brief largest seq number that can fit into the seq number section of the msgID
 */
static const uint64_t MSG_SEQ_NUMBER_MAX = (1ull << (64-MSGID_BITS_PER_DAEMON_ID ))-1ull;

/* @brief seq number half-point where we need to check for throttling
 */
static const uint64_t MSG_SEQ_NUMBER_HALFPOINT = (MSG_SEQ_NUMBER_MAX >> 1);


struct MessageContextContainer
{
  typedef std::chrono::time_point< std::chrono::system_clock > TimeType;
  csm::network::MessageAndAddress_sptr _MsgAddr;
  csm::daemon::EventContext_sptr _Context;
  csm::daemon::VirtualNetworkChannel_sptr _Connection;
  TimeType _Timeout;
  uint32_t _RespCount;

  MessageContextContainer( const csm::network::MessageAndAddress_sptr i_MsgAddr,
                           const csm::daemon::EventContext_sptr i_Context,
                           const csm::daemon::VirtualNetworkChannel_sptr i_Connection,
                           const uint32_t i_RespCount,
                           const uint32_t i_Timeout )
  : _MsgAddr( i_MsgAddr ), _Context( i_Context ), _Connection( i_Connection ), _RespCount( i_RespCount )
  {
    _Timeout = std::chrono::system_clock::now() + std::chrono::seconds( i_Timeout );
    LOG( csmd, trace ) << "Create MsgCtxContainer: (" << _MsgAddr << "," << (void*)(_Context.get()) << ")";
  }
  ~MessageContextContainer()
  {
    LOG( csmd, trace ) << "Destroying MsgCtxContainer: (" << _MsgAddr << "," << (void*)(_Context.get()) << ")";
    _MsgAddr = nullptr;
    _Context = nullptr;
    _Connection = nullptr;
    _RespCount = 0;
  }
};

typedef std::shared_ptr<MessageContextContainer> MessageContextContainer_sptr;

typedef std::unordered_map<uint64_t, csm::daemon::MessageContextContainer_sptr> MessageContextStore;
typedef std::unordered_map<uint64_t, uint64_t> ClientMessageIDMapping;

class MessageControl
{
  typedef std::chrono::time_point< std::chrono::system_clock > TimeType;
  MessageContextStore _OutboundContextStore;
  std::queue<uint64_t> _GarbageCollectedIDs;
  MessageContextStore _InboundContextStore;
  ClientMessageIDMapping _MessageIDMap;
  std::mutex _Lock;
  uint64_t _DaemonID;
  uint64_t _MsgIDSeqNumber;
  TimeType _MsgIDTimer;
  TimeType _GCTimer;
  csm::daemon::Throttle<100> _Throttle;

public:
  MessageControl( const uint64_t aDaemonID = 0 );
  ~MessageControl();

  MessageControl & operator=( const MessageControl &i_MsgCtl );

  uint64_t CreateMessageID();

  // may change the message header if messageID was zero
  uint64_t StoreContext( csm::network::MessageAndAddress_sptr aMsgAddr,
                         csm::daemon::EventContext_sptr aContext,
                         uint32_t aRespCount = 1,
                         uint32_t aExpiration = CSM_CONTEXT_TIMEOUT_MIN_SECONDS,
                         csm::daemon::VirtualNetworkChannel_sptr connection = nullptr );
  csm::daemon::EventContext_sptr FindContext( csm::network::MessageAndAddress_sptr aMsgAddr );
  csm::daemon::MessageContextContainer_sptr FindMsgAndCtx( const uint64_t i_MsgId,
                                                           const bool i_ForceDelete = false);
  csm::daemon::MessageContextContainer_sptr FindMsgAndCtx( csm::network::MessageAndAddress_sptr aMsgAddr,
                                                           const bool i_ForceDelete = false );

  void InboundMessageIDMapping( csm::network::MessageAndAddress_sptr aMsgAddr );
  void OutboundMessageIDMapping( csm::network::MessageAndAddress_sptr aMsgAddr );

  /* Cleanup of stored messageID->context mappings
   * - aTimeLimit puts a boundary on the milliseconds that GC can take to prevent GC from blocking the netmgr
   * - returns true if the GC completed within the limit
   * - returns false if the GC did not complete within the limit
   */
  bool GarbageCollect( const uint64_t aTimeLimit = 10 );
  bool HasStoredContext() const { return _OutboundContextStore.size() + _InboundContextStore.size() + _MessageIDMap.size() > 0; }

private:
  uint64_t CreateMessageIDMapping( const uint64_t aClientMsgId );
  uint64_t GetMessageIDMapping( const uint64_t aDaemonMsgId );
  uint64_t GetAndCleanMessageIdMapping( const uint64_t aDaemonMsgId );
  bool GCCleanup( const TimeType &aStartTime, const TimeType &aDeadline );
  bool GCScan( const TimeType &aStartTime, const TimeType &aDeadline );

};


}  // namespace daemon
} // namespace csm


#endif /* CSMD_SRC_DAEMON_INCLUDE_MESSAGE_CONTROL_H_ */
