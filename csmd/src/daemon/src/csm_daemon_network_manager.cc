/*================================================================================

    csmd/src/daemon/src/csm_daemon_network_manager.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <memory>

#ifdef logprefix
#undef logprefix
#endif
#define logprefix "NETMGR"
#include "csm_pretty_log.h"

#include "csmnet/include/csm_network_config.h"
#include "csmnet/src/CPP/csm_network_exception.h"
#include "csmnet/src/CPP/address.h"
#include "csmnet/src/CPP/csm_multicast_message.h"

#include "csm_daemon_config.h"
#include "include/run_mode.h"
#include "include/csm_daemon_network_manager.h"
#include "include/csm_event_routing.h"
#include "include/csm_system_event.h"

#include "include/virtual_network_channel.h"
#include "include/virtual_network_channel_pool.h"
#include "csmi_request_handler/helpers/EventHelpers.h"

// limit the amount of system events to prevent head of line blocking caused by a ctrl event storm
#define CSM_MAX_CTRL_EVENTS 50

// if there are more than this number of send requests queued up, then stop the send-batch and process
// accepts and recvs before continuing with the next batch
#define CSM_NETMGR_SEND_BATCH 50
#define CSM_NETMGR_RECV_BATCH 50

namespace csm {
namespace daemon {

const int DEFAULT_NETWORK_POOL_SIZE = 1;
VirtualNetworkChannelPool *VirtualNetworkChannelPool::_Instance = nullptr;

void NetworkManagerMain( csm::daemon::EventManagerNetwork *aMgr )
{
  aMgr->GreenLightWait();
  CSMLOG(csmd,info) << "Starting Network manager thread (" << boost::this_thread::get_id() << ")";

  csm::network::ReliableMsg *epl = aMgr->GetEndpointList();
  if( epl == nullptr )
  {
    CSMLOG(csmd, error) << "FATAL: Unable to access network mgr from worker thread. Exiting.";
    return;
  }

  // enable the daemon signal set to listen for SIGALRM
  csm::daemon::RetryBackOff *retry = aMgr->GetRetryBackoff();

  uint64_t busyLoops = 0;
  while( aMgr->GetThreadKeepRunning() )
  {
    bool idle = true;

    if( aMgr->GetRunMode() == csm::daemon::RUN_MODE::READY_RUNNING_JOB )
    {
      aMgr->GreenLightWait();
      LOG( csmd, trace ) << "NetMgr wakeup after Greenlight.";
    }

    if( ! aMgr->IsARunningMode() )
    {
      // process system events even if we're in disconnected mode to flush the pipeline
      if(( aMgr->GetRunMode() == csm::daemon::RUN_MODE::DISCONNECTED) && ( aMgr->ProcessNetCtlEvents() == true ))
        retry->AgainOrWait( false );
      if( ! aMgr->GetThreadKeepRunning() )
        break;
    }

    //////////////////////////////////////////
    // Endpoint maintenance activity
    try
    {
      idle &= aMgr->EndpointMaintenance();
      LOG( csmd, trace ) << "NetMgr: EndpointMaintenance complete idle=" << idle;
    }
    catch( csm::network::Exception &e )
    {
      CSMLOG( csmd, error ) << "Caught network error ACCEPT: " << e.what();
    }

    //////////////////////////////////////////
    // Receive/inbound Activity
    try
    {
      int recvActivity = 0;
      do
      {
        if( aMgr->EndpointRecvActivity() == false )
          ++recvActivity;
        else
          break;
      } while(( recvActivity > 0 ) && ( recvActivity < CSM_NETMGR_RECV_BATCH ));

      idle &= ( recvActivity == 0 );

      LOG( csmd, trace ) << "NetMgr: EndpointRecvActivity complete idle=" << idle;
    }
    catch ( Exception &e )
    {
      CSMLOG( csmd, error ) << "Unhandled recv activity exception." << e.what();
    }

    //////////////////////////////////////////
    // Send/outbound activity
    try
    {
      // send in batches in case we have a huge amount of sends
      int sendActivity = 0;
      do
      {
      if( aMgr->EndpointSendActivity() == false )
        ++sendActivity;
      else
        break;
      } while(( sendActivity > 0 ) && (sendActivity < CSM_NETMGR_SEND_BATCH ));

      idle &= ( sendActivity == 0 );

      LOG( csmd, trace ) << "NetMgr: EndpointSendActivity complete idle=" << idle;
    }
    catch ( ... )
    {
      CSMLOG( csmd, error ) << "Unhandled send activity exception.";
    }

    ///////////////////////////////////////////
    // do other maintenance when idle or when running for too long
    if(( idle ) || ( ((++busyLoops) & 0xFFFF) == 0 ))
    {
      idle &= aMgr->EndpointIdleMaintenance();
      LOG( csmd, trace ) << "NetMgr: EndpointIdleMaintenance complete idle=" << idle;
    }

    //////////////////////////////////////////////////////////////
    // reduce the 100% cpu polling based on idle-loop detection
    if( !idle )
      retry->Reset();
    else
    {
      // if we were idle during jitter window open, then just wait for interrupt (jitter window closing)
      // and let the beginning of the next loop go to wait for green light condition (next jitter window open)
      retry->AgainOrWait( aMgr->GetRunMode() == csm::daemon::RUN_MODE::READY_RUNNING_JOB );
      if( ! aMgr->GetThreadKeepRunning() )
        break;
    }
  }
}

EventManagerNetwork::EventManagerNetwork( csm::daemon::ConnectionHandling *aConnMgr,
                                          csm::daemon::DaemonState *aDaemonState,
                                          csm::daemon::RetryBackOff *i_MainIdleLoopRetry )
: _KeepThreadRunning( true ), _ReconnectInterval( 10000 )
{
  _Config = csm::daemon::Configuration::Instance();

  _Source = new csm::daemon::EventSourceNetwork( i_MainIdleLoopRetry );

  _DaemonState = aDaemonState;

  _ConnMgr = aConnMgr;
  _EndpointList = _ConnMgr->GetEndpointList();
  
  // drop list of knows endpoints and types
  _EndpointList->Log();

  _IdleLoopRetry = new csm::daemon::RetryBackOff( "NetMgr", csm::daemon::RetryBackOff::SleepType::SOCKET,
                                                  csm::daemon::RetryBackOff::SleepType::INTERRUPTIBLE_SLEEP,
                                                  1, 10000, _Config->GetTweaks()._NetMgr_polling_loops,
                                                  _EndpointList );
  if( _IdleLoopRetry == nullptr )
    throw csm::daemon::Exception("Failed to allocate IdleRetry manager for network events.");

  _Sink = new csm::daemon::EventSinkNetwork( _IdleLoopRetry );
  if( _Sink == nullptr )
    throw csm::daemon::Exception("Failed to allocate Event Sink for network events.");

  _MessageControl = csm::daemon::MessageControl( _Config->GetDaemonState()->GetDaemonID() );

  CreateNetworkConnectionPool();
  _Config->SetNetConnectionPool( _NetworkChannelPool );
  _NetMgrChannel = _NetworkChannelPool->GetDedicatedChannel();
  if( _NetMgrChannel == nullptr )
    throw csm::daemon::Exception("Failed to reserve network mgr channel from connection pool.");

  _ReadyToRun = false;

  _Thread = new boost::thread( NetworkManagerMain, this );
  _IdleLoopRetry->SetThread( _Thread );

  Unfreeze();
}

}  // namespace csm::daemon
}

bool csm::daemon::EventManagerNetwork::CheckAndGenerateComputeActions( csm::daemon::DaemonStateMaster *masterState )
{
  if( masterState == nullptr )
    return true;
  int count = 0;
  csm::daemon::ComputeActionEntry_t ca( "", csm::daemon::COMPUTE_ACTION_UNDEF );
  do
  {
    csm::daemon::NetworkEvent *netevent = nullptr;
    ca = masterState->GetNextComputeAction();
    switch( ca.second )
    {
      case csm::daemon::COMPUTE_UP:
        CSMLOG( csmd, debug ) << "Action on node:" << ca.first << ": UP";
        netevent = csm::daemon::helper::CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_UP, ca.first,
            "", "",
            std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_NONE ) );
        ++count;
        break;
      case csm::daemon::COMPUTE_LOST_CONNECTION:
      case csm::daemon::COMPUTE_DOWN:
        CSMLOG( csmd, debug ) << "Action on node:" << ca.first << ": DOWN";
        netevent = csm::daemon::helper::CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_DOWN, ca.first,
            "", "",
            std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_NONE ) );
        ++count;
        break;
      case csm::daemon::COMPUTE_FULL_REDUNDANCY:
        CSMLOG( csmd, debug ) << "Action on node:" << ca.first << ": FULL_REDUNDANCY";
        netevent = csm::daemon::helper::CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_FULL_REDUNDANCY, ca.first,
            "", "",
            std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_NONE ) );
        ++count;
        break;
      case csm::daemon::COMPUTE_LOST_REDUNDANCY:
        CSMLOG( csmd, debug ) << "Action on node:" << ca.first << ": LOST_REDUNDANCY";
        netevent = csm::daemon::helper::CreateRasEventMessage(CSM_RAS_MSG_ID_STATUS_LOST_REDUNDANCY, ca.first,
            "", "",
            std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_NONE ) );
        ++count;
        break;
      default:
        break;
    }
    if( netevent != nullptr )
      _Source->QueueEvent( netevent );

  } while( ca.second != csm::daemon::COMPUTE_ACTION_UNDEF );

  return (count == 0);
}

bool csm::daemon::EventManagerNetwork::HandleDisconnect( csm::network::NetworkCtrlInfo_sptr info_itr )
{
  if( info_itr->_Address != nullptr )
  {
    bool notifyRequired = false;

    // for now, filter local addresses, since we don't need the disconnects to be noticed by handlers
    if( info_itr->_Address->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL)
    {
      notifyRequired = true;
      csm::daemon::ConnectedNodeStatus *nodeInfo = _DaemonState->GetNodeInfo( info_itr->_Address );
      notifyRequired = (( nodeInfo != nullptr ) && ( nodeInfo->_NodeMode == csm::daemon::RUN_MODE::READY_RUNNING ));
    }

    _ConnMgr->ProcessSystemEvent( csm::daemon::SystemContent::DISCONNECTED,
                                  info_itr->_Address,
                                  _MessageControl.CreateMessageID() );

    if( notifyRequired )
    {
      NotifyRegisteredContext( csm::daemon::SystemContent::DISCONNECTED,
                               info_itr->_Address );
      if( _Config->GetRole() == CSM_DAEMON_ROLE_MASTER )
        CheckAndGenerateComputeActions( dynamic_cast<csm::daemon::DaemonStateMaster*>(_DaemonState) );
    }
  }
  return true;
}



bool csm::daemon::EventManagerNetwork::ProcessNetCtlEvents()
{
  bool idle = true;
  bool conn_updates = false;

  csm::network::NetworkCtrlInfo_sptr info_itr=nullptr;
  // counter to limit the amount of system events
  for( int counter = CSM_MAX_CTRL_EVENTS;
      ( counter > 0 );
      --counter )
  {
    info_itr = _EndpointList->GetCtrlEvent();
    if( info_itr == nullptr )
      break;

    CSMLOG( csmd, debug) << "Processing a network event type: " << info_itr->_Type << " ptr=@" << (void*)(info_itr.get());
    switch( info_itr->_Type )
    {
      case csm::network::NET_CTL_UNVERIFIED:
      {
        // a new connection is established. Update the global state
        csm::network::Address_sptr addr = info_itr->_Address;

        csm::daemon::ConnectedNodeStatus *nodeInfo = _DaemonState->GetNodeInfo( info_itr->_Address );
        if(( nodeInfo != nullptr ) &&
            ( nodeInfo->_NodeAddr != nullptr ) &&
            ( nodeInfo->_NodeMode == csm::daemon::RUN_MODE::READY_RUNNING ))
        {
          CSMLOG( csmd, info ) << "Removing existing endpoint to replace " << nodeInfo->_NodeAddr->Dump()
              << " with " << addr->Dump();
          _EndpointList->DeleteEndpoint( nodeInfo->_NodeAddr.get(), "NetMgr" );
          csm::network::NetworkCtrlInfo_sptr old_info =
              std::make_shared<csm::network::NetworkCtrlInfo>( csm::network::NET_CTL_DISCONNECT, 0, nodeInfo->_NodeAddr );
          HandleDisconnect( old_info );
          conn_updates = true;
        }

        if(( _Config->GetRole() == CSM_DAEMON_ROLE_AGGREGATOR ) &&
            ( addr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_PTP ))
        {
          _DaemonState->AddEP(addr, csm::daemon::ConnectionType::SECONDARY, RUN_MODE::DISCONNECTED );
          CSMLOG( csmd, info ) << "New unconfirmed initial connection from compute node " << info_itr->_Address->Dump()
              << ". assume secondary+disconnected until ctrl msg specifies type.";
        }
        else
        {
          // do not log client addresses at info level
          if( addr->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL )
          {
            CSMLOG( csmd, info ) << "New connection from type= " << info_itr->_Address->GetAddrType()
              << " remote_addr="<< info_itr->_Address->Dump();
          }
          else
          {
            CSMLOG( csmd, debug ) << "New connection from type= " << info_itr->_Address->GetAddrType()
              << " remote_addr="<< info_itr->_Address->Dump();
          }

          _DaemonState->AddEP(addr, csm::daemon::ConnectionType::SINGLE, RUN_MODE::DISCONNECTED );
        }
        break;
      }
      case csm::network::NET_CTL_CONNECT:
        if (info_itr->_Address)
        {
          if(( _Config->GetRole() == CSM_DAEMON_ROLE_AGGREGATOR ) &&
              ( info_itr->_Address->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_PTP ))
          {
            CSMLOG( csmd, trace ) << "ComputeInfo: " << info_itr->_VersionData._Hostname << ":" << info_itr->_VersionData._Sequence;
            dynamic_cast<csm::daemon::DaemonStateAgg*>(_DaemonState)->SetNodeInfo( info_itr->_Address, info_itr->_VersionData._Hostname );
            conn_updates = true;
          }
          // first check if it's a critical connection and let the connection mgr handle it
          _ConnMgr->ProcessSystemEvent( csm::daemon::SystemContent::CONNECTED,
                                        info_itr->_Address,
                                        _MessageControl.CreateMessageID() );

          // for now, filter local addresses, since we don't need the disconnects to be noticed by handlers
          if( info_itr->_Address->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL)
          {
            NotifyRegisteredContext( csm::daemon::SystemContent::CONNECTED, info_itr->_Address );
            if( _Config->GetRole() == CSM_DAEMON_ROLE_MASTER )
              CheckAndGenerateComputeActions( dynamic_cast<csm::daemon::DaemonStateMaster*>(_DaemonState) );
          }
        }
        break;

      case csm::network::NET_CTL_DISCONNECT:
        HandleDisconnect( info_itr );
        conn_updates = true;
        break;

      case csm::network::NET_CTL_RESTARTED:  // we found a freshly discovered daemon (need to send it's inventory
        if( _Config->GetRole() == CSM_DAEMON_ROLE_AGGREGATOR )
        {
          if(( info_itr->_Address ) && ( info_itr->_Address->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
            NotifyRegisteredContext( csm::daemon::SystemContent::RESTARTED, info_itr->_Address );
        }
        break;
      case csm::network::NET_CTL_FAILOVER:
        switch( _Config->GetRole() )
        {
          case CSM_DAEMON_ROLE_AGGREGATOR:
            _DaemonState->SetConnectionTypeEP( info_itr->_Address, csm::daemon::ConnectionType::PRIMARY );
            if(( info_itr->_Address ) && ( info_itr->_Address->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
              NotifyRegisteredContext( csm::daemon::SystemContent::FAILOVER, info_itr->_Address );
            break;
          case CSM_DAEMON_ROLE_AGENT:
            csm::daemon::Exception("BUG: Invalid event NET_CTL_FAILOVER for csm-compute daemon.");
            break;
          default:
            break;
        }
        break;
      case csm::network::NET_CTL_TIMEOUT:
      {
        csm::daemon::MessageContextContainer_sptr msgCtx = _MessageControl.FindMsgAndCtx( info_itr->_MsgId, true );
        if( msgCtx != nullptr )
          QueueSendErrorEvent( *msgCtx->_MsgAddr, msgCtx, "ACK Timed out", ETIMEDOUT );
        else
          CSMLOG( csmd, warning ) << "Potential ACK/Context issue: ACK Timed out, but no context found. Check ACK_TIMEOUT < CTX_CLEANUP_INTERVAL";

        if(( info_itr->_Address ) && ( info_itr->_Address->GetAddrType() != csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL ))
          NotifyRegisteredContext( csm::daemon::SystemContent::ACK_TIMEOUT, info_itr->_Address );

        CSMLOG( csmd, debug ) << "ACK timeout detected for msgID: " << info_itr->_MsgId;
        break;
      }

      case csm::network::NET_CTL_FATAL:
      {
        CSMLOG( csmd, error ) << "Fatal error! Endpoint addr: " << (info_itr->_Address != nullptr ? info_itr->_Address->Dump() : "n/a" );
        _ConnMgr->ProcessSystemEvent( csm::daemon::SystemContent::FATAL, info_itr->_Address );
        break;
      }
      default:
        CSMLOG( csmd, error ) << "Unrecognized network ctrl event type: " << info_itr->_Type;
        break;
    }
    idle = false;
  }
  if(( conn_updates ) && ( _Config->GetRole() == CSM_DAEMON_ROLE_AGGREGATOR ))
    CreateComputeSetUpdateMsg();

  return idle;
}


bool csm::daemon::EventManagerNetwork::EndpointMaintenance()
{
  // check for new connections
  bool idle = ( _EndpointList->Accept( false ) == nullptr );

  // check for disconnections and other errors
  idle &= ( _EndpointList->Sync( csm::network::SYNC_ACTION_MAINTENANCE ) == 0 );  // time to do some maintenance
  idle &= ProcessNetCtlEvents();
  return idle;
}

bool csm::daemon::EventManagerNetwork::EndpointRecvActivity()
{
  bool idle = true;
  csm::network::MessageAndAddress_sptr data_sptr = std::make_shared<csm::network::MessageAndAddress>();
  csm::daemon::EventContext_sptr ctx = nullptr;
  csm::daemon::MessageContextContainer_sptr msgContext = nullptr;

  if( data_sptr == nullptr )
    throw csm::daemon::Exception("Failed to allocate message. Out of mem?");

  int rc = 0;
  try
  {
    rc = _EndpointList->RecvFrom( *data_sptr );
    if( rc && (data_sptr->_Msg.Validate() ))
    {
      if(( data_sptr->_Msg.GetCommandType() == CSM_CMD_NODESET_UPDATE ) && ( _Config->GetRole() == CSM_DAEMON_ROLE_MASTER))
      {
        csm::daemon::DaemonStateMaster *masterState = dynamic_cast<csm::daemon::DaemonStateMaster*>(_DaemonState);
        masterState->UpdateAggregator( data_sptr->GetAddr(), data_sptr->_Msg );
        CheckAndGenerateComputeActions( masterState );
        return true;
      }
      _MessageControl.InboundMessageIDMapping( data_sptr );
      msgContext = _MessageControl.FindMsgAndCtx( data_sptr );
      if( msgContext )
      {
        CSMLOG( csmd, debug ) << "Found MessageContextContainer: conn=" << msgContext->_Connection->GetId();
        ctx = msgContext->_Context;
      }
      else
        msgContext = std::make_shared<csm::daemon::MessageContextContainer>( data_sptr,
                                                                             nullptr,
                                                                             _NetMgrChannel,
                                                                             1,
                                                                             CSM_CONTEXT_TIMEOUT_MIN_SECONDS );
    }
  }
  catch( csm::network::Exception &e )
  {
    CSMLOG( csmd, warning ) << "Recv exception: " << e.what();
    rc = e.GetErrno();
    if( rc > 0 )
      rc = -rc;
    if( rc == 0 )
      rc = -EBADMSG;
  }
  // check for any abandoned recvs to clean up in the dedicated channel
  if( rc == 0 )
  {
    rc = _NetMgrChannel->Recv( *data_sptr, ctx, 0 );
    // returns either 0 (just header), >0 (with payload), or -ENOMSG (no data)
    if( rc >= 0 )
    {
      rc += sizeof( csm_network_header_t );
      msgContext = std::make_shared<csm::daemon::MessageContextContainer>( data_sptr,
                                                                           ctx,
                                                                           _NetMgrChannel,
                                                                           1,
                                                                           CSM_CONTEXT_TIMEOUT_MIN_SECONDS );
    }
    if( rc < 0 )  rc = 0;  // make sure, no error is signaled - we're just idle if nothing is to clean up
  }

  if( rc < 0 )
  {
    // we probably never get here because RecvFrom() will throw exception on fail! double check!
    CSMLOG( csmd, critical ) << "CSM BUG in error handling: recv. rc<0 must cause exception earlier!!!!";
    msgContext = std::make_shared<csm::daemon::MessageContextContainer>( data_sptr,
                                                                         nullptr,
                                                                         _NetMgrChannel,
                                                                         1,
                                                                         CSM_CONTEXT_TIMEOUT_MIN_SECONDS );
    QueueInboundErrorEvent( data_sptr, msgContext, "CSM BUG in error handling: rc<0 must cause exception earlier!!!!", errno );
  }

  if( rc > 0 )
  {
    QueueRecvEvent( data_sptr, msgContext );
  }
  idle &= (rc == 0 );

  idle &= ( _EndpointList->Sync( csm::network::SYNC_ACTION_MAINTENANCE ) == 0 );  // time to do some maintenance
  idle &= ProcessNetCtlEvents();
  return idle; // && ! _MessageControl.HasStoredContext();
}

bool csm::daemon::EventManagerNetwork::EndpointSendActivity()
{
  bool idle = true;
  
  csm::daemon::VirtualNetworkChannel_sptr virtChannel = nullptr;
  csm::daemon::EventContext_sptr context = nullptr;
  csm::network::MessageAndAddress_sptr data_sptr;

  if( ! GetOutboundActivity( virtChannel, context, data_sptr ) )
    return idle;

  csm::daemon::MessageContextContainer_sptr msgContext =
      std::make_shared<csm::daemon::MessageContextContainer>( data_sptr,
                                                              context,
                                                              virtChannel,
                                                              1,
                                                              CSM_CONTEXT_TIMEOUT_MIN_SECONDS );

  if( data_sptr == nullptr )
  {
    CSMLOG(csmd, error) << "BUG: Found outbound network event without data. Event will be ignored.";
    return idle;
  }
  if( ! data_sptr->_Msg.Validate() )
  {
    CSMLOG(csmd, error) << "BUG: Found invalid outbound message! Returning send error to handler.";
    QueueInboundErrorEvent( data_sptr, msgContext, "Send: Invalid Message", EBADMSG );
    return true;
  }

  csm::network::Address_sptr addr = data_sptr->GetAddr();
  if (addr == nullptr)
  {
    CSMLOG(csmd, warning) << "Found outbound network event without destination address!";

    if( virtChannel != _NetMgrChannel )
      throw csm::daemon::Exception("BUG: nullptr address on virtChannel send() should have been detected earlier.");

    QueueInboundErrorEvent( data_sptr, msgContext, "Send to empty address", EDESTADDRREQ );
    return false;
  }
  
  uint32_t expectedResponses = 1;

  csm::daemon::AddressListType destAddrList;
  csm::network::Address_sptr send_addr = addr;

  try
  {
    // turn abstract address into a real one
    if( addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_ABSTRACT )
      expectedResponses = ExtractDestinationAddresses( data_sptr, msgContext, destAddrList );
    else
      expectedResponses = CreateDestinationAddresses( data_sptr, destAddrList );

  }
  catch( csm::daemon::Exception &e )
  {
    CSMLOG( csmd, error ) << "Failure while resolving destination address: " << addr->Dump();
    return false;
  }

  if( expectedResponses > 0 )
  {
    expectedResponses = std::max( (uint32_t)destAddrList.size(), expectedResponses );
  }
  else  // this message got filtered based on the destination address (e.g. secondary compute while primary connection active, or msg drop)
  {
    CSMLOG( csmd, debug ) << "Message " << data_sptr->_Msg.GetMessageID()
      << " to " << send_addr->Dump()
      << " got filtered or queued locally (inactive secondary or self-send)";
    return true;
  }

  if( destAddrList.empty() )
  {
    // address resolves to an empty list, create a send error for the sending handler
    CSMLOG( csmd, error ) << "Requested address " << send_addr->Dump() << " currently has no existing destination on this system.";
    QueueSendErrorEvent( *data_sptr, msgContext, "Send error.", EADDRNOTAVAIL );
    return false;
  }

  // set the context expiration; response msgs should have a shortened timeout
  // they only need to stay to handle ACK timeouts
  uint32_t ContextExpiration = csm_context_timeout( 
        data_sptr->_Msg.GetCommandType(), 
        data_sptr->_Msg.GetResp());

  if( _MessageControl.StoreContext( data_sptr,
                                    context,
                                    expectedResponses,
                                    ContextExpiration,
                                    virtChannel ) == 0 )
        throw csm::network::Exception("Failed to store event context.");
  _MessageControl.OutboundMessageIDMapping( data_sptr );

  std::string addr_string;
  for( auto addr_it: destAddrList )
  {
    int rc = -1;
    if( addr_it == nullptr )
    {
      CSMLOG( csmd, error ) << "BUG: Nullptr in abstract address resolution. Some node went down or lost track otherwise.";
      addr_string = "null";
    }
    else
    {
      addr = addr_it;
      CSMLOG(csmd, debug) << "NetMgr Sending msg to " << addr->Dump();

      data_sptr->SetAddr( addr );

      try
      {
        rc = _EndpointList->SendTo( *data_sptr );
        addr_string = addr->Dump();
      }
      // SendTo() should only fail if the endpoint is down. So we need to shut it down here.
      catch (csm::network::ExceptionEndpointDown &e )
      {
        rc = e.GetErrno();
        // make sure the context is removed from storage in case of error
        csm::daemon::MessageContextContainer_sptr ctx = _MessageControl.FindMsgAndCtx( data_sptr );
        CSMLOG( csmd, error ) << "Endpoint error: " << e.what()  << " endpoint addr: "<< addr->Dump();

        if( destAddrList.size() == 1 )
          QueueSendErrorEvent( *data_sptr, msgContext, "Send error.", ECOMM );
      }
      // catch everything else while printing less details
      catch( csm::network::Exception &e )
      {
        csm::daemon::EventContext_sptr ctx = _MessageControl.FindContext( data_sptr );
        CSMLOG( csmd, error ) << "Network problem.: " << e.what();
      }
    }
    if( rc <= 0 )
    {
      csm::daemon::MessageContextContainer_sptr ctx = _MessageControl.FindMsgAndCtx( data_sptr );
      CSMLOG(csmd, error) << "Send error detected for msgId: " <<  data_sptr->_Msg.GetMessageID()
        << " addr=" << addr_string
        << " rc=" << rc << "(" << strerror( -rc ) << ")";

      QueueSendErrorEvent( *data_sptr, msgContext, "Send error.", ECOMM );
    }
    idle = false;
  }
  
  idle &= ( _EndpointList->Sync( csm::network::SYNC_ACTION_MAINTENANCE ) == 0 );  // time to do some maintenance
  idle &= ProcessNetCtlEvents();
  return idle;
}

bool csm::daemon::EventManagerNetwork::EndpointIdleMaintenance()
{
  try
  {
    bool GC_Complete = _MessageControl.GarbageCollect( 10000 );
    if(( GetRunMode() == csm::daemon::RUN_MODE::PARTIAL_CONNECT ) &&
       ( _ReconnectInterval.IsReady() ))
    {
      _ConnMgr->ProcessSystemEvent( csm::daemon::SystemContent::RETRY_CONNECT,
                                    nullptr );

      // in case we transitioned to fully connected:
      if( GetRunMode() == csm::daemon::RUN_MODE::READY_RUNNING )
      {
        csm::network::MessageAndAddress msgAddr( _DaemonState->GetLocalInventory(), _DaemonState->GetSecondaryAddress() );
        CSMLOG( csmd, debug ) << "CONNECT event to aggregator: " << _DaemonState->GetSecondaryAddress()->Dump() << " sending inventory.";
        _EndpointList->SendTo( msgAddr );
      }
    }
    return GC_Complete;
  }
  catch ( csm::network::ExceptionEndpointDown &e )
  {
    CSMLOG( csmd, error ) << "BUG: DAEMON EXCEPTION ENDPOINT DOWN. THIS SHOULD NOT HAPPEN! " << e.what();
  }
  catch ( csm::network::ExceptionProtocol &e )
  {
    CSMLOG( csmd, error ) << "BUG: DAEMON EXCEPTION PROTOCOL. THIS SHOULD NOT HAPPEN! " << e.what();
  }
  catch ( csm::network::Exception &e )
  {
    CSMLOG( csmd, error ) << "Caught network error: " << e.what();
  }
  return false;  // getting here after error and thus not idle
}

void csm::daemon::EventManagerNetwork::CreateNetworkConnectionPool()
{
  std::string netPoolSizeString = _Config->GetValueInConfig( std::string("csm.net.connection_pool_size") );
  int netPoolSize = csm::daemon::DEFAULT_NETWORK_POOL_SIZE;

  if( netPoolSizeString.empty() )
  {
    CSMLOG(csmnet, debug) << "Using default csm.net.connection_pool_size=" << netPoolSize;
  }
  else
    netPoolSize = std::stoi( netPoolSizeString );

  _NetworkChannelPool = csm::daemon::VirtualNetworkChannelPool::Init( netPoolSize, _IdleLoopRetry );
}


csm::daemon::AddressListType csm::daemon::EventManagerNetwork::GetMasterAddress()
{
  csm::daemon::AddressListType list;
  switch( _Config->GetRole() )
  {
    case CSM_DAEMON_ROLE_AGENT: list.push_back( _DaemonState->GetActiveAddress() ); break;
    case CSM_DAEMON_ROLE_UTILITY:
    case CSM_DAEMON_ROLE_AGGREGATOR: list.push_back( _Config->GetConfiguredMasterAddress() ); break;
    case CSM_DAEMON_ROLE_MASTER: break; // empty list
    default: break;
  }
  return list;
}

csm::daemon::AddressListType csm::daemon::EventManagerNetwork::GetAggregatorAddress()
{
  csm::daemon::AddressListType aggList;
  switch( _Config->GetRole() )
  {
    case CSM_DAEMON_ROLE_AGENT: aggList.push_back( _DaemonState->GetActiveAddress() ); break;
    case CSM_DAEMON_ROLE_UTILITY: aggList.push_back( _Config->GetConfiguredMasterAddress() ); break;
    case CSM_DAEMON_ROLE_AGGREGATOR: break; // self-send, empty list
    case CSM_DAEMON_ROLE_MASTER:
    {
      csm::daemon::DaemonStateMaster *ds = dynamic_cast<csm::daemon::DaemonStateMaster*>( _Config->GetDaemonState() );
      ds->GetAggregators( aggList, csm::daemon::ConnectionType::SINGLE, true );
      break;
    }
    default: break;
  }
  return aggList;
}

/*
 * returns true if the message is supposed to pass through the filtering of secondary traffic
 */
bool csm::daemon::EventManagerNetwork::FilterSecondary( csm::daemon::ConnectedNodeStatus *i_NodeInfo,
                                                        const csm::network::MessageAndAddress_sptr i_MsgAddr ) const
{
  // todo: check nodeinfo for whether we need to filter secondary msgs or not
  enum csm_csmi_cmds_t cmd = i_MsgAddr->_Msg.GetCommandType();
  bool passThrough = false;
  switch( cmd )
  {
    case CSM_infrastructure_test:
    case CSM_CMD_ECHO:
    case CSM_TEST_MTC:
    case CSM_CMD_INV_get_node_inventory:
      passThrough = true;
      break;
    default:
      break;
  }

  LOG( csmd, trace ) << "Message to: " << i_MsgAddr->GetAddr()->Dump()
      << " cmd:" << csm::network::cmd_to_string( cmd )
      << " passThrough=" << passThrough;
  return passThrough;
}

int csm::daemon::EventManagerNetwork::CreateDestinationAddresses( csm::network::MessageAndAddress_sptr i_MsgAddr,
                                                                  AddressListType &destAddrList )
{
  if( i_MsgAddr->GetAddr()->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL )
  {
    destAddrList.push_back( i_MsgAddr->GetAddr() );
    return 1;
  }
  if( ! i_MsgAddr->_Msg.GetMulticast() )
  {
    csm::daemon::ConnectedNodeStatus *node_info = _DaemonState->GetNodeInfo( i_MsgAddr->GetAddr() );
    if(( node_info != nullptr ) &&
        (( node_info->_ConnectionType == csm::daemon::ConnectionType::SECONDARY ) && ( ! FilterSecondary( node_info, i_MsgAddr ) )) )
      return 0;
    else
    {
      destAddrList.push_back( i_MsgAddr->GetAddr() );
      return 1;
    }
  }

  CSMLOG( csmd, debug ) << "Assembling destinations for multicast msg";
  std::vector<std::string> nodeList;
  uint32_t nodeStrLen;
  csm::network::ExtractMulticastNodelist( i_MsgAddr->_Msg, nodeList, &nodeStrLen );

  for( auto node : nodeList )
  {
    csm::network::Address_sptr dest = _DaemonState->GetNextHopAddress( node );
    if( dest != nullptr )
      destAddrList.push_back( dest );
  }

  return nodeList.size();
}

int csm::daemon::EventManagerNetwork::ExtractDestinationAddresses( csm::network::MessageAndAddress_sptr i_MsgAddr,
                                                                   csm::daemon::MessageContextContainer_sptr i_MsgContext,
                                                                   AddressListType &o_DestAddrList )
{
  int expectedResponses = 1;
  csm::network::Address_sptr addr = i_MsgAddr->GetAddr();
  csm::network::AddressAbstractType name = std::dynamic_pointer_cast<csm::network::AddressAbstract>( addr )->_AbstractName;

  // check for self sends on master or aggregator
  if( (( _Config->GetRole() == CSM_DAEMON_ROLE_MASTER ) && ( name == csm::network::ABSTRACT_ADDRESS_MASTER)) ||
      (( _Config->GetRole() == CSM_DAEMON_ROLE_AGGREGATOR ) && ( name == csm::network::ABSTRACT_ADDRESS_AGGREGATOR)) )
    name = csm::network::ABSTRACT_ADDRESS_SELF;

  switch( name )
  {
    case csm::network::ABSTRACT_ADDRESS_MASTER:
      o_DestAddrList = GetMasterAddress();
      break;

    case csm::network::ABSTRACT_ADDRESS_AGGREGATOR:
    {
      o_DestAddrList = GetAggregatorAddress();
      if( _Config->GetRole() == CSM_DAEMON_ROLE_MASTER )
        expectedResponses = 5000;  // todo: temporary setting until we have more detailed info about aggregators
      break;
    }

    case csm::network::ABSTRACT_ADDRESS_BROADCAST:
      expectedResponses = _DaemonState->GetAllEPs( o_DestAddrList, csm::daemon::ConnectionType::ANY, true );
      if( _Config->GetRole() == CSM_DAEMON_ROLE_MASTER )
        expectedResponses = 5000;  // todo: temporary setting until we have more detailed info about aggregators
      break;

    case csm::network::ABSTRACT_ADDRESS_SELF:
      QueueRecvEvent( i_MsgAddr, i_MsgContext );
      expectedResponses = 0; // self-sends do not expect responses, no need to trigger storage of context here
      break;

    case csm::network::ABSTRACT_ADDRESS_NONE:
      // nothing to do here - just drop
      break;

    default:
      throw csm::network::Exception( "Unrecognized abstract address type." );
      //break;
  }
  CSMLOG( csmd, debug ) << "Resolved " << name << " to " << o_DestAddrList.size() << " destinations.";
  if( o_DestAddrList.size() > 0 )
    CSMLOG( csmd, debug ) << "   first destination: " << o_DestAddrList[0]->Dump();
  return expectedResponses;
}

void csm::daemon::EventManagerNetwork::QueueFailoverMsg( csm::network::Address_sptr addr )
{
  if( addr == nullptr )
    throw csm::daemon::Exception( "Trying failover to nullptr address." );

  CSMLOG( csmd, debug ) << "Creating FAILOVER message for Aggregator: " << addr->Dump() << " to signal primary address.";
  csm::network::MessageAndAddress evData;
  evData.SetAddr( addr );
  evData._Msg.Init( CSM_CMD_CONNECTION_CTRL,
                    CSM_HEADER_INT_BIT,
                    CSM_PRIORITY_DEFAULT,
                    0,
                    0x0, 0x0,
                    geteuid(), getegid(),
                    std::string( CSM_FAILOVER_MSG ));
  _EndpointList->SendTo( evData );
}

bool csm::daemon::EventManagerNetwork::CreateComputeSetUpdateMsg()
{
  csm::daemon::DaemonStateAgg *ads = dynamic_cast<csm::daemon::DaemonStateAgg*>( _DaemonState );
  if( ads == nullptr )
    return false;

  csm::daemon::ComputeSet *cs = ads->GetComputeSet();
  if( cs == nullptr )
    return false;

  CSMLOG( csmd, trace ) << "NodeSetStatus: " << cs->GetUncommittedUpdates();

  if( cs->GetUncommittedUpdates() == 0 )
  {
    return false;
  }

  std::string msgData = csm::daemon::ComputeSet::ConvertDiffToBytes( *cs );
  csm::network::MessageAndAddress evData;
  evData.SetAddr( GetMasterAddress().front() );
  evData._Msg.Init( CSM_CMD_NODESET_UPDATE,
                    CSM_HEADER_INT_BIT,
                    CSM_PRIORITY_NO_ACK,
                    _MessageControl.CreateMessageID(),
                    0x0, 0x0,
                    geteuid(), getegid(),
                    msgData );
  CSMLOG( csmd, debug ) << "Creating NODESET_UPDATE message for Master: " << evData.GetAddr()->Dump()
      << " content: " << msgData;
  if( _EndpointList->SendTo( evData ) >= 0 )
    cs->Commit();
  return true;
}
