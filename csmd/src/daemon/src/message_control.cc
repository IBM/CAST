/*================================================================================

    csmd/src/daemon/src/message_control.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <chrono>
#include <vector>
#include "logging.h"

#include "csmnet/src/CPP/csm_network_exception.h"
#include "csmnet/src/CPP/csm_network_msg_cpp.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/csm_message_and_address.h"

#include "include/virtual_network_channel.h"
#include "include/message_control.h"

csm::daemon::MessageControl::MessageControl( const uint64_t aDaemonID )
: _DaemonID( aDaemonID ),
  _MsgIDSeqNumber( random() % MSGID_START_RANDOM_MAX ),  // randomize the start-ID
  _Throttle( std::string("client requests"),
             csm::daemon::MSG_SEQ_NUMBER_HALFPOINT,
             csm::daemon::MSGID_HALF_TIME_LIMIT )
{
  _GCTimer = std::chrono::system_clock::now() + std::chrono::seconds( csm::daemon::DEFAULT_GC_INTERVAL );
  _MsgIDTimer = std::chrono::system_clock::now() + std::chrono::seconds( csm::daemon::MSGID_HALF_TIME_LIMIT );
}

csm::daemon::MessageControl::~MessageControl()
{
  std::lock_guard<std::mutex> guard( _Lock );
  _InboundContextStore.clear();
  _OutboundContextStore.clear();
}

uint64_t
csm::daemon::MessageControl::CreateMessageID()
{
  ++_MsgIDSeqNumber;
  uint64_t id = ( _DaemonID << (63-MSGID_BITS_PER_DAEMON_ID) ) + ( _MsgIDSeqNumber & csm::daemon::MSG_SEQ_NUMBER_MAX );
  LOG( csmd, debug ) << "Creating new messageID=" << id;

  if( ( _MsgIDSeqNumber & csm::daemon::MSG_SEQ_NUMBER_HALFPOINT ) == 0 )
    _Throttle.Update();

  _Throttle.CheckAndDo();

  return id;
}

uint64_t
csm::daemon::MessageControl::StoreContext( csm::network::MessageAndAddress_sptr aMsgAddr,
                                           csm::daemon::EventContext_sptr aContext,
                                           uint32_t aRespCount,
                                           uint32_t aExpiration,
                                           csm::daemon::VirtualNetworkChannel_sptr connection )
{
  // check message ID and create a new Id if it's zero
  uint64_t msgId = aMsgAddr->_Msg.GetMessageID();
  if( msgId == 0 )
    msgId = CreateMessageID();

  // update the messageID if needed
  if( msgId != aMsgAddr->_Msg.GetMessageID() )
  {
    aMsgAddr->_Msg.SetMessageID( msgId );
    aMsgAddr->_Msg.CheckSumUpdate();
  }

  // keep the context for multicast messages
  uint32_t respCount = aRespCount;

  csm::daemon::MessageContextContainer_sptr msgCtx =
      std::make_shared<csm::daemon::MessageContextContainer>( aMsgAddr,
                                                              aContext,
                                                              connection,
                                                              respCount,
                                                              aExpiration );
  std::lock_guard<std::mutex> guard( _Lock );
  _OutboundContextStore[ msgId ] = msgCtx;
  LOG( csmd, debug ) << "Storing message context for MSGID=" << msgId << " expResp=" << respCount
      << " expires=" << aExpiration
      << " VChan=" << connection->GetId();

  return msgId;
}

csm::daemon::EventContext_sptr
csm::daemon::MessageControl::FindContext( csm::network::MessageAndAddress_sptr aMsgAddr )
{
  csm::daemon::MessageContextContainer_sptr msgCtx = FindMsgAndCtx( aMsgAddr->_Msg.GetMessageID() );
  if( msgCtx != nullptr )
    return msgCtx->_Context;
  else
    return nullptr;
}

csm::daemon::MessageContextContainer_sptr
csm::daemon::MessageControl::FindMsgAndCtx( csm::network::MessageAndAddress_sptr aMsgAddr, const bool i_ForceDelete )
{
  return FindMsgAndCtx( aMsgAddr->_Msg.GetMessageID(), i_ForceDelete );
}

csm::daemon::MessageContextContainer_sptr
csm::daemon::MessageControl::FindMsgAndCtx( const uint64_t i_MsgId, const bool i_ForceDelete )
{
  // check message header for request/response flag
  // see if we had stored an outbound context
  try
  {
    std::lock_guard<std::mutex> guard( _Lock );
    csm::daemon::MessageContextContainer_sptr ctxInfo = _OutboundContextStore.at( i_MsgId );
    if( ctxInfo == nullptr )
      throw csm::daemon::Exception("BUG: Stored nullptr context container found.");

    // forced delete after timeout (e.g. also for multicast timeouts
    if( ctxInfo->_RespCount > 0 )
      --ctxInfo->_RespCount;

    if(( ctxInfo->_RespCount == 0 ) || ( i_ForceDelete ))
      _OutboundContextStore.erase( i_MsgId );

    LOG( csmd, trace ) << "Found message context for MSGID=" << i_MsgId << " expResp=" << ctxInfo->_RespCount
        << " VChan=" << ctxInfo->_Connection->GetId();
    return ctxInfo;
  }
  catch ( std::out_of_range &e )
  {
    LOG( csmd, debug ) << "No stored message context for MSGID=" << i_MsgId;
  }

  return csm::daemon::MessageContextContainer_sptr(nullptr);
}

bool
csm::daemon::MessageControl::GCCleanup( const TimeType &aStartTime,
                                        const TimeType &aDeadline )
{
  // remove the expired msgIDs from context store
  bool completed = true;
  uint64_t removed = 0;
  uint64_t toRemoveEntries = _GarbageCollectedIDs.size();
  while( _GarbageCollectedIDs.size() > 0 )
  {
    uint64_t msgId = _GarbageCollectedIDs.front();
    LOG( csmd, trace ) << "GC: Cleaning up timed-out msgID-ctx: " << msgId;
    FindMsgAndCtx( msgId, true );
    _GarbageCollectedIDs.pop();

    TimeType current;
    // every now and then, check if we're taking too long...
    if(( ((++removed) & 0x3F) == 0 ) && ( ( current = std::chrono::system_clock::now()) >= aDeadline))
    {
      completed = false;
      LOG( csmd, info ) << "GC: reached limit for cleanup after " << removed << " of " << toRemoveEntries
          << " expired entries. remaining=" << _OutboundContextStore.size()
          << " exceeded deadline by " << (double)std::chrono::duration_cast<std::chrono::microseconds>( current - aDeadline ).count() << "us";

      break;
    }
  }

  // did we remove anything, then do some logging
  if( removed > 0 )
  {
    TimeType GC_Finished = std::chrono::system_clock::now();
    uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(GC_Finished-aStartTime).count();

    LOG( csmd, debug ) << "GC: removed " << removed << " of " << toRemoveEntries
        << " expired contexts in " << micros/1000.
        << " ms. remaining=" << _OutboundContextStore.size();
  }
  return completed;
}

bool
csm::daemon::MessageControl::GCScan( const TimeType &aStartTime,
                                     const TimeType &aDeadline )
{
  bool completed = true;

  std::lock_guard<std::mutex> guard( _Lock );

  // scan the map for expired entries
  LOG( csmd, trace ) << "GC: starting scan. pending CTX=" << _OutboundContextStore.size();

  static uint64_t lastMsgId = 0;
  uint64_t iteration = 0;
  uint64_t newRemoveEntries = 0;

  auto startLocation = ( lastMsgId == 0 ) ? _OutboundContextStore.begin() : _OutboundContextStore.find( lastMsgId );
  if( startLocation == _OutboundContextStore.end() )
  {
    LOG( csmd, debug ) << "GC: StartLocation would be at the end. Resetting...";
    startLocation = _OutboundContextStore.begin();
  }
  lastMsgId = 0;
  for( auto it=startLocation;
      it != _OutboundContextStore.end();
      ++it  )
  {
    ++iteration;
    csm::daemon::MessageContextContainer_sptr ctx = it->second;

    if( ctx == nullptr )
    {
      _Lock.unlock();
      throw csm::daemon::Exception("BUG: found nullptr MsgCtxContainter.");
    }

    if( ctx->_Timeout <= aStartTime )
    {
      ++newRemoveEntries;
      _GarbageCollectedIDs.push( it->first );
    }
    else // only check and store the exit position if the current position is not an expired entry
    {
      TimeType current;
      // every now and then, check if we're taking too long...
      if(( (iteration & 0x7F) == 0 ) && ( ( current = std::chrono::system_clock::now()) >= aDeadline))
      {
        completed = false;
        // if we found less than 1/10th of the map, we continue from here next time
        if( iteration < (_OutboundContextStore.size() - (iteration >> 1) ) )
          lastMsgId = it->first;

        LOG( csmd, info ) << "GC: reached limit for scanning after " << iteration << "/" << _OutboundContextStore.size()
            << " steps with " << newRemoveEntries
            << " expired found."
            << " exceeded deadline by " << (double)std::chrono::duration_cast<std::chrono::microseconds>( current - aDeadline ).count() << "us";

        break;
      }
    }
  }
  // reset the lastMsgId entry in case we made it to the end
  if( completed == true )
  {
    LOG( csmd, debug ) << "GC: completed scan after scanning " << iteration << ". " << newRemoveEntries << " expired entries found. Next GC will start scan from beginning";
    lastMsgId = 0;
  }

  LOG( csmd, trace ) << "GC: scan complete=" << completed << " with remaining CTX=" << _OutboundContextStore.size();
  return completed;
}

bool
csm::daemon::MessageControl::GarbageCollect( const uint64_t aTimeLimit )
{
  bool completed = true;

  TimeType GC_Time = std::chrono::system_clock::now();
  TimeType GC_Limit = GC_Time + std::chrono::microseconds( aTimeLimit );

  /* GC Strategy:
   * First we delete all garbage-collected entries
   * Then we scan for new expired entries
   * This prevents double-scanning in case we exceed the time limit for cleanup
   * It also automatically uses the remaining (or all) time for scanning
   * and in extreme cases spread the cleanup over several calls to GC
   * The scan remembers the last scan position if nothing was found
   * and less than 40% were scanned before the deadline
   */
  completed &= GCCleanup( GC_Time, GC_Limit );
  if(( _OutboundContextStore.size() > 0 ) && ( _GCTimer <= GC_Time ))
    completed &= GCScan( GC_Time, GC_Limit );

  // set the time for the next GC
  if( _GCTimer <= GC_Time )
    _GCTimer += std::chrono::seconds( csm::daemon::DEFAULT_GC_INTERVAL );
  return completed;
}


void
csm::daemon::MessageControl::InboundMessageIDMapping( csm::network::MessageAndAddress_sptr aMsgAddr )
{
  // if we're receiving data from a local address, we need to reverse the messageID mapping between client and daemon
  if(( aMsgAddr->GetAddr() != nullptr ) && ( aMsgAddr->GetAddr()->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL ))
  {
    if( ! aMsgAddr->_Msg.Validate() )
      throw csm::daemon::Exception("BUG: An invalid client message made it up to the inbound msgID mapping.");

    uint64_t cltMsgId = aMsgAddr->_Msg.GetMessageID();
    uint64_t dmnMsgId = CreateMessageIDMapping( cltMsgId );

    aMsgAddr->_Msg.SetMessageID( dmnMsgId );
    aMsgAddr->_Msg.CheckSumUpdate();

    LOG( csmd, debug ) << "Reverse Mapping setup client-to-daemon msgId: " << cltMsgId << " -> " << dmnMsgId;
  }
}
void
csm::daemon::MessageControl::OutboundMessageIDMapping( csm::network::MessageAndAddress_sptr aMsgAddr )
{
  // if we're sending data out to a local address, we need to reverse the messageID mapping between client and daemon
  if(( aMsgAddr->GetAddr() != nullptr ) && ( aMsgAddr->GetAddr()->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL ))
  {
    uint64_t dmnMsgId = aMsgAddr->_Msg.GetMessageID();
    uint64_t cltMsgId = GetAndCleanMessageIdMapping( dmnMsgId );
    if( cltMsgId != 0 )
    {
      aMsgAddr->_Msg.SetMessageID( cltMsgId );
      aMsgAddr->_Msg.CheckSumUpdate();
    }
    LOG( csmd, debug ) << "Reverse Mapping retrieve daemon-to-client msgId: " << dmnMsgId << " -> " << cltMsgId;
  }
}


uint64_t
csm::daemon::MessageControl::CreateMessageIDMapping( const uint64_t aClientMsgId )
{
  std::lock_guard<std::mutex> guard( _Lock );
  uint64_t dmnMsgId = CreateMessageID();

  _MessageIDMap[ dmnMsgId ] = aClientMsgId;

  return dmnMsgId;
}

uint64_t
csm::daemon::MessageControl::GetMessageIDMapping( const uint64_t aDaemonMsgId )
{
  uint64_t ret = 0;
  csm::daemon::ClientMessageIDMapping::iterator it = _MessageIDMap.find( aDaemonMsgId );
  if( it != _MessageIDMap.end() )
    ret = it->second;

  return ret;
}

uint64_t
csm::daemon::MessageControl::GetAndCleanMessageIdMapping( const uint64_t aDaemonMsgId )
{
  std::lock_guard<std::mutex> guard( _Lock );
  uint64_t ret = GetMessageIDMapping( aDaemonMsgId );
  if( ret > 0 )
    _MessageIDMap.erase( aDaemonMsgId );
  return ret;
}

csm::daemon::MessageControl &
csm::daemon::MessageControl::operator=( const MessageControl &i_MsgCtl )
{
  std::lock_guard<std::mutex> guard( _Lock );
  _OutboundContextStore = i_MsgCtl._OutboundContextStore;
  _InboundContextStore = i_MsgCtl._InboundContextStore;
  _MessageIDMap = i_MsgCtl._MessageIDMap;
  _DaemonID = i_MsgCtl._DaemonID;
  _MsgIDSeqNumber = i_MsgCtl._MsgIDSeqNumber;
  _GCTimer = i_MsgCtl._GCTimer;

  return *this;
}
