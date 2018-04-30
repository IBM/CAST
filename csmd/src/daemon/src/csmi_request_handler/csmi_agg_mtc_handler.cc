/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_agg_mtc_handler.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef logprefix
#define logprefix "AGG-MTC"
#endif

#ifndef mtccomp
#define mtccomp csmd
#endif

#include "csm_pretty_log.h"
#include "logging.h"

#include "csmi_agg_mtc_handler.h"


void CSMI_AGG_MTC_HANDLER::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  csm::daemon::EventContext_sptr context = aEvent.GetEventContext();

  if( isTimerEvent( aEvent ) )
  {
    if( ( context != nullptr ) && ( context->GetAuxiliaryId() > 0 ) )
    {
      csm::daemon::NetworkEvent *reqEvent = dynamic_cast<csm::daemon::NetworkEvent*>( context->GetReqEvent() );
      if( reqEvent != nullptr )
      {
        csm::network::MessageAndAddress msgAddr = reqEvent->GetContent();
        CSMLOG( mtccomp, warning ) << "Request timeout detected. cmd="
          << csm::network::cmd_to_string( msgAddr._Msg.GetCommandType() )
          << " msgId=" << msgAddr._Msg.GetMessageID();
        postEventList.push_back( CreateErrorEvent(ETIMEDOUT, "Request timeout detected.", msgAddr ));
        return;
      }
      else
      {
        CSMLOG( mtccomp, error ) << "Failed to extract original request from context. Can't reply to sender.";
        return;
      }
    }
    else
    {
      CSMLOG( mtccomp, debug ) << "Timer event from completed MTC.";
      return;
    }
  }
  
  if ( !isNetworkEvent(aEvent) )
  {
    CSMLOG( mtccomp, error ) << "Expecting a NetworkEvent. Received: " << EventTypeToString( aEvent.GetEventType() );
    return;
  }

  csm::network::Address_sptr peer = GetNetworkAddress( aEvent );
  if( peer == nullptr )
  {
    CSMLOG( mtccomp, error ) << "Network event has no peer address. Can't continue processing. This will likely cause timeouts elsewhere.";
    return;
  }

  CSMLOG( mtccomp, debug ) << "Processing... " << ( (context == nullptr) ? "fanOut" : "fanIn" );
  csm::network::Message inMsg = GetNetworkMessage( aEvent );

  switch( peer->GetAddrType() )
  {
    case csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL: // requests from local clients are going to be forwarded to master
      if( inMsg.GetErr() )
      {
        CSMLOG( mtccomp, warning ) << "Received an error msg from local client. " << peer->Dump()
          << ". Dropping. cmd=" << csm::network::cmd_to_string( inMsg.GetCommandType() );
        break;
      }
      context = CreateContext(aEvent, this, 0);
      if( context != nullptr )
      {
        postEventList.push_back( CreateNetworkEvent( inMsg, _AbstractMaster ) );

        const int timeout = csm_get_client_timeout( inMsg.GetCommandType() ) - 1000;
        CSMLOG( mtccomp, debug ) << "Setting timeout to " << timeout << " milliseconds.";

        postEventList.push_back( CreateTimerEvent(timeout, context, 1 ) );
        context->SetAuxiliaryId( 1 );
      }
      break;

    case csm::network::AddressType::CSM_NETWORK_TYPE_AGGREGATOR:
      if (context == nullptr) // a new multi-cast request from Master
      {
        // error msg without context - can't handle
        if( inMsg.GetErr() )
        {
          CSMLOG( mtccomp, warning ) << "Inbound error message from master without context. Dropping. cmd="
            << csm::network::cmd_to_string( inMsg.GetCommandType() );
          break;
        }

        // any new msg (no context) that hits the MTC handler from the master
        // has to be a multicast message
        std::vector< std::string > node_list;
        csm::network::Message outMsg;
        if ( !DecodeMulticastMessage( inMsg, node_list, outMsg) )
        {
          CSMLOG( mtccomp, warning ) << "Failed to Decode a multicast msg: cmd="
              << csm::network::cmd_to_string( inMsg.GetCommandType() );
          // return error code to master...
          postEventList.push_back( CreateErrorEvent(EINVAL, "Multicast msg decode.", aEvent ) );
          return;
        }

        CSMLOG( mtccomp, debug ) << "NodeList size=" << node_list.size();

        context = CreateContext(aEvent, this, 0);
        int responses = 0;
        csm::daemon::DaemonStateAgg * daemonState = (csm::daemon::DaemonStateAgg *) GetDaemonState();
        for( auto node : node_list )
        {
          const csm::network::Address_sptr addr = daemonState->GetAddrForCN( node );
          if( addr == nullptr )
          {
            CSMLOG( mtccomp, debug ) << "this aggregator is not responsible for " << node;
            continue;
          }
          csm::daemon::ConnectedNodeStatus *nInfo = daemonState->GetNodeInfo( addr );
          if( nInfo == nullptr )
          {
            CSMLOG( mtccomp, debug ) << "requested node " << node << " is in UNKNOWN state.";
            // possibly inconsistent state: MTC failure; check if and how the 2 lists might get out of sync
            continue;
          }
          if( nInfo->_NodeMode != csm::daemon::RUN_MODE::READY_RUNNING )
          {
            CSMLOG( mtccomp, debug ) << "requested node " << node << " is DISCONNECTED.";
            continue;
          }
          if( nInfo->_ConnectionType == csm::daemon::ConnectionType::SECONDARY )
          {
            CSMLOG( mtccomp, trace ) << "connection to node " << node << " is marked SECONDARY. Skipping send.";
            continue;
          }


          csm::daemon::NetworkEvent *event;
          if( ( event = ForwardNetworkEventWithMessageId0(outMsg, addr, context)) != nullptr )
          {
            // the message sent to compute should not have MTC bit set
            CSMLOG( mtccomp, debug) << "Sending a mtcmsg to " << addr->Dump();
            postEventList.push_back(event);
            ++responses;
          }
          else
          {
            CSMLOG( mtccomp, warning ) << "Failed to prepare outbound message to " << addr->Dump();
            postEventList.clear();
            responses = 0;
            postEventList.push_back( CreateErrorEvent(EINVAL, "Multicast msg forward.", aEvent ) );
            // don't stop fan-out of mtc just because of one msg failing
          }
        }
        // if we post anything, we should set a timeout before getting back to master
        if( responses > 0 )
        {
          const int timeout = csm_get_agg_timeout( inMsg.GetCommandType());
          CSMLOG( mtccomp, info ) << "Expecting " << responses <<
            " responses. Setting timeout to " << timeout << " milliseconds.";

          postEventList.push_back( CreateTimerEvent(timeout, context, responses-1 ) );
          context->SetAuxiliaryId( responses );
        }
      }
      else // response from master; forward to the original requester
      {
        CSMLOG( mtccomp, debug ) << "Forwarding a reply to original requestor...";
        csm::daemon::NetworkEvent *request = (csm::daemon::NetworkEvent*) context->GetReqEvent();
        if( request && ( !inMsg.GetErr() || request->GetContent().GetAddr() != _AbstractMaster ))
        {
          postEventList.push_back( ForwardReplyNetworkEvent(aEvent, context) );
        }
      }
      break;

    case csm::network::AddressType::CSM_NETWORK_TYPE_PTP: // Handle any compute node traffic
      if( context == nullptr ) // new request (or late response from compute without context)
      {
        if( !inMsg.GetResp() )  // regular request from compute, need to forward
        {
          CSMLOG( mtccomp, debug ) << "Forwarding Compute request " << csm::network::cmd_to_string( inMsg.GetCommandType() )
            << " to master.";
          context = CreateContext(aEvent, this, 0);
          postEventList.push_back(CreateNetworkEvent( inMsg, _AbstractMaster,  context));
        }
        else
        {
          CSMLOG( mtccomp, warning )
            <<"response message cmd=" << csm::network::cmd_to_string( inMsg.GetCommandType() )
            <<" without context from compute " << peer->Dump() << " Dropping.";
        }
        return;
      }
      else // reply from compute
      {
        int responses = context->GetAuxiliaryId();
        if( responses > 0 )
          context->SetAuxiliaryId( responses - 1 );
        else
        {
          CSMLOG( mtccomp, warning ) << "Unexpected additional response from " << peer->Dump()
            << ". MTC Response refcounter problem or duplicate response. cmd=" << csm::network::cmd_to_string( inMsg.GetCommandType() )
            << " counter=" << responses;
          return;
        }

        // get the message id in the original multi-cast message
        uint64_t reqMsgId;
        if ( !GetMessageIDFromRequestInContext(context, reqMsgId) )
        {
          CSMLOG( mtccomp, warning ) << "No messageID in the original MTC request. Can't forward response cmd=" << csm::network::cmd_to_string( inMsg.GetCommandType() )
              << " from " << peer->Dump();
          return;
        }

        // before forwarding a reply to Master, need to revert back to the original message id
        inMsg.SetMessageID(reqMsgId);
        inMsg.CheckSumUpdate();

        CSMLOG( mtccomp, debug ) << "Forward a reply from Agent to Master...";
        postEventList.push_back( CreateNetworkEvent( inMsg, _AbstractMaster, nullptr) );
      }
      break;
    default:
      break;
  }
}
#undef CSMI_TIMEOUT
