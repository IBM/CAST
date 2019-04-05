/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/csm_infrastructure_test_agg.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
// implement the CSM api node attributes update command...
//

#include <malloc.h>

#include "csm_infrastructure_test.h"
#include "include/csm_event_type_definitions.h"
#include "csmi_forward_handler.h"


void CSM_INFRASTRUCTURE_TEST_AGG::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  if ( isSystemEvent(aEvent) )
  {
    TestSystemEvent(aEvent);
    return;
  }

  EventContextTestAgg_sptr context = aEvent.GetEventContext() ?
      std::dynamic_pointer_cast<EventContextTestAgg>( aEvent.GetEventContext() ) :
      EventContextTestAgg_sptr( new EventContextTestAgg( this, INITIAL_STATE, CopyEvent(aEvent) ));

  LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: TEST_SETUP = " << context->GetAuxiliaryId();

  csm::daemon::DaemonStateAgg * daemonState = dynamic_cast<csm::daemon::DaemonStateAgg *> (GetDaemonState());

  if ( !daemonState )
  {
      LOG(csmd, error) << "Aggregator Daemon State could not be retrieved.";
      return;
  }

  switch (context->GetAuxiliaryId())
  {
    case INITIAL_STATE:
    {
      if ( !isNetworkEvent(aEvent) ) break;

      csm::network::Message msg = GetNetworkMessage(aEvent);

      // Forward the received request from csm api client to the master if it's originating at a local address type
      // context is important so that we can find the client's address when the reply is back
      csm::network::Address_sptr reqAddr = GetNetworkAddress( aEvent );
      if(  reqAddr == nullptr )
      {
        LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_AGG: Triggered by request with nullptr addr. Skipping any processing of request.";
        return;
      }

      if( reqAddr->GetAddrType() == csm::network::AddressType::CSM_NETWORK_TYPE_LOCAL )
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_AGG: Received local request. Forwarding to master.";

        csm::daemon::NetworkEvent *FwdEvent = CreateNetworkEvent( msg, _AbstractMaster, context );
        if( FwdEvent != nullptr)
        {
          LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: Forward client request to Master";
          postEventList.push_back( FwdEvent );
        }
        else
        {
          LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_AGG: Error creating request to Master";
          postEventList.push_back( CreateErrorEvent(EINVAL,
                                                    "Failed master request creation.",
                                                    aEvent ) );
        }
        postEventList.push_back( CreateTimerEvent(_utility_timeout, context, LOCAL_RESPONSE) );
        context->SetAuxiliaryId(LOCAL_RESPONSE);
        return;
      }

      // find all primary connection entries (connected and disconnected)
      csm::daemon::AddressListType cnList;
      daemonState->GetAllEPs(cnList, csm::daemon::ConnectionType::PRIMARY, true );
      std::vector<csm::daemon::ConnectedNodeStatus_sptr> cnDisconnectedList;
      daemonState->GetAllDisconnectedEPs(cnDisconnectedList, csm::daemon::ConnectionType::PRIMARY );

      // find all secondary connection entries (connected and disconnected)
      csm::daemon::AddressListType cnList_sec;
      daemonState->GetAllEPs(cnList_sec, csm::daemon::ConnectionType::SECONDARY, true );
      std::vector<csm::daemon::ConnectedNodeStatus_sptr> cnDisconnectedList_sec;
      daemonState->GetAllDisconnectedEPs(cnDisconnectedList_sec, csm::daemon::ConnectionType::SECONDARY);


      // now gather the information from the DamonState regarding the HealthCheckData
      HealthCheckData data;
      data = CreateHCDAndSetLDaemon(msg);

      // populate the AggInfo
      AggInfo_sptr aggInfo = boost::make_shared<AggInfo>( data._local );
      data._agg_info.push_back(aggInfo);

      // set up the test specific context
      context->SetNumCNs( cnList.size() + cnList_sec.size() );
      LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: Expecting total of " << context->GetNumCNs() << " replies from Agents";

      // cache in the context
      context->_cached_data = data;
      context->SetComputeExpect( true, cnList );
      context->SetComputeExpect( false, cnList_sec );
      for( auto it : cnDisconnectedList )
      {
        std::lock_guard<std::mutex> guard(_Lock);
        std::string nodeID = it->_NodeID;
        if( nodeID.empty() )
          nodeID = it->_NodeAddr->Dump();
        context->_cached_data._agg_info[0]->_PrimaryComputes.push_back(
            boost::make_shared<ComputeInfo>( nodeID, "n/a", it->_Bounced, false, csm::daemon::ConnectionType::ANY )
        );
      }
      for( auto it : cnDisconnectedList_sec )
      {
        std::lock_guard<std::mutex> guard(_Lock);
        std::string nodeID = it->_NodeID;
        if( nodeID.empty() )
          nodeID = it->_NodeAddr->Dump();
        context->_cached_data._agg_info[0]->_SecondaryComputes.push_back(
            boost::make_shared<ComputeInfo>( nodeID, "n/a", it->_Bounced, false, csm::daemon::ConnectionType::ANY )
        );
      }

      LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_AGG: Test expects " << context->GetNumCNs() << " active CNs. "
          << " disconnected CNs: " << cnDisconnectedList.size() + cnDisconnectedList_sec.size();

      // if there are no known connected compute nodes, we just return what we got from daemonState
      if (cnList.size() + cnList_sec.size() <= 0) // no connected CN. Send a reply back to Master
      {
        csm::network::Message replyMsg(GetNetworkMessage(aEvent));
        replyMsg.SetResp();
        replyMsg.SetData( CSMI_BASE::ConvertToBytes<HealthCheckData>(data) );
        replyMsg.CheckSumUpdate();
        csm::network::MessageAndAddress content(replyMsg, _AbstractMaster);

        postEventList.push_back( CreateNetworkEvent(content, context) );
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG::Send a reply right back to Master as no connected CN";
      }
      else
      {
        postEventList.push_back( CreateTimerEvent(_agg_timeout, context, WAIT_STATE) );
      }

      // set the MessageId to 0 with the same context in all the outbound messages to CNs.
      // Will need to convert back to the original MessageId of the aEvent
      //   before sending a reply back to Master
      for ( auto it : cnList )
      {
        csm::daemon::NetworkEvent *event;
        if ( (event = ForwardNetworkEventWithMessageId0(aEvent, it, context)) != nullptr)
        {
          postEventList.push_back(event);
        }
      }
      for ( auto it : cnList_sec )
      {
        // context does matter here. Will use context to handle multiple CNs later
        csm::daemon::NetworkEvent *event;
        if ( (event = ForwardNetworkEventWithMessageId0(aEvent, it, context)) != nullptr)
        {
          postEventList.push_back(event);
        }
      }

      context->SetAuxiliaryId(WAIT_STATE);
      break;
    }
    case WAIT_STATE:
    {
      // for the csm_infrastructure_test case, we should aggregate here before replying to the Master
      // because the master does not know how many CNs in the system.
      if (isNetworkEvent(aEvent))
      {
        csm::network::Address_sptr compAddr = GetNetworkAddress(aEvent);

        bool allreplies = context->IncrementRecvCNs( compAddr );

        ComputeInfo data;
        try
        {
          CSMI_BASE::ConvertToClass<ComputeInfo>( GetNetworkMessage(aEvent).GetData(), data );
        }
        catch( std::exception &e )
        {
          LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_AGG: failed to deserialize data from compute: " << compAddr->Dump();

        }
        // won't use the hostname value here as this is just to confirm we can get response from CN
        if(context->_cached_data._agg_info.size() == 1)
        {
          csm::daemon::ConnectedNodeStatus *CNState = daemonState->GetNodeInfo( compAddr );
          if( CNState != nullptr )
          {
             std::lock_guard<std::mutex> guard(_Lock);
             data.SetBounced( CNState->_Bounced );

             // check and update the connection type (in case there was a failover
             csm::daemon::ConnectionType::CONN_TYPE conn_type = data.GetConnectionType();
             data.SetConnectionType( CNState->_ConnectionType );

             LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_AGG: received compute info for " << CNState->_NodeID
                 << " as " << conn_type << " via " << CNState->_ConnectionType << " link.";

             if( conn_type == csm::daemon::ConnectionType::PRIMARY )
               context->_cached_data._agg_info[0]->_PrimaryComputes.push_back( boost::make_shared<ComputeInfo>( data ) );
             else
               context->_cached_data._agg_info[0]->_SecondaryComputes.push_back( boost::make_shared<ComputeInfo>( data ) );
          }
          else
          {
            LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_AGG: cannot find a UID for Addr=" << compAddr->Dump();
          }
        }
        else
        {
          if (context->_cached_data._agg_info.size() != 1)
          {
            LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_AGG: context->_cached_data._agg_info should be just one element";
          }
        }

        if( ! allreplies )
        {
          size_t num = context->GetNumCNs() - context->GetRecvCNs();
          LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: Expecting " <<  num << " more replies from CN (RecordCNs="
                        << context->GetRecvCNs() << " numCNs= " << context->GetNumCNs() << ")";
          //return;
          break;
        }
      }
      else if ( isTimerEvent(aEvent) )
      {
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: A Timeout Event...";
      }
      else break;

      // ok, now it is time to get the correct MessageId for the reply to Master
      uint64_t reqMsgId = 0;
      if ( !GetMessageIDFromRequestInContext(context, reqMsgId) )
      {
        LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_AGG: Cannot find the message id in the original request";
        return;
      }

      // add non-responding (timeout) primary and secondary computes
      for( auto it : context->GetComputeExpect( true ) )
      {
        csm::daemon::ConnectedNodeStatus *CNState = daemonState->GetNodeInfo( it );
        if( CNState != nullptr )
        {
           std::lock_guard<std::mutex> guard(_Lock);
           std::string nodeID = CNState->_NodeID;
           if( nodeID.empty() )
             nodeID = CNState->_NodeAddr->Dump();
           context->_cached_data._agg_info[0]->_PrimaryComputes.push_back(
               boost::make_shared<ComputeInfo>( nodeID,
                                                "timeout",
                                                CNState->_Bounced,
                                                false,
                                                csm::daemon::ConnectionType::PRIMARY));
        }
      }
      for( auto it : context->GetComputeExpect( false ) )
      {
        csm::daemon::ConnectedNodeStatus *CNState = daemonState->GetNodeInfo( it );
        if( CNState != nullptr )
        {
           std::lock_guard<std::mutex> guard(_Lock);
           std::string nodeID = CNState->_NodeID;
           if( nodeID.empty() )
             nodeID = CNState->_NodeAddr->Dump();
           context->_cached_data._agg_info[0]->_SecondaryComputes.push_back(
               boost::make_shared<ComputeInfo>( nodeID,
                                                "timeout",
                                                CNState->_Bounced,
                                                false,
                                                csm::daemon::ConnectionType::SECONDARY));
        }
      }

      // prepare the response to the master
      csm::network::Message msg;
      if (isTimerEvent(aEvent))
        msg = GetNetworkMessage( *(context->GetReqEvent()) );
      else if( isNetworkEvent(aEvent))
        msg = GetNetworkMessage(aEvent);
      else
      {
        LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_AGG: Invalid event type when preparing response. Will drop. This will cause timeout.";
        return;
      }

      // check if the original request is coming from a local csm api client
      csm::network::Address_sptr addr = GetNetworkAddress( *(context->GetReqEvent()) );
      if (addr->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL)
      {
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: Forward a single aggregated reply to a local client";
      }
      else
      {
        addr = _AbstractMaster;
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: Forward a single aggregated reply to the Master";
      }

      msg.SetData( CSMI_BASE::ConvertToBytes<HealthCheckData>(context->_cached_data) );
      msg.SetMessageID(reqMsgId);
      msg.CheckSumUpdate();
      postEventList.push_back( CreateNetworkEvent(msg, addr, context) );

      context->SetAuxiliaryId(DONE_STATE);

      break;
    }
    case DONE_STATE:
      if( ! isTimerEvent( aEvent ) )
      {
        LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_AGG: a non-timer event triggered the health test after completion. Late response from compute?";
      }
      else
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_AGG: Timer event triggered after completion as expected.";
      }
      break;

    case LOCAL_RESPONSE:
    {
      LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_AGG: Forwarding response from Master to the local client";
      if( isTimerEvent( aEvent ) )
      {

        const csm::daemon::TimerEvent* timeEvent = dynamic_cast<const csm::daemon::TimerEvent*>( &aEvent );
        if( !timeEvent || timeEvent->GetContent().GetTargetStateId() != LOCAL_RESPONSE )
          return;

        // handle timeout
        csm::daemon::NetworkEvent *resp = CreateTimeoutNetworkResponse( aEvent,
                                                                        ETIMEDOUT,
                                                                        "Infrastructure test timeout on Aggregator" );
        if( resp != nullptr )
          postEventList.push_back( resp );
        else
        {
          LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_AGG: Failed to create timeout error response. No response will be sent.";
        }
      }
      else
      {
        if( !isNetworkEvent( aEvent ) )
          break;

        csm::network::Message msg = GetNetworkMessage(aEvent);
        // start with the cached data in case the master responds with an error
        HealthCheckData data;
        if (msg.GetErr())
        {
          LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_AGG: Get an Error NetworkEvent...";

          data = context->_cached_data;
          // either ECOMM or ETIMEOUT
          csmi_err_t *err = csmi_err_unpack(msg.GetData().c_str(), msg.GetDataLen());
          if (err)
          {
            data._errmsg = std::string(err->errmsg);
            csmi_err_free(err);
          }
          else data._errmsg = std::string("Failed to retrieve the error payload");

          msg = GetNetworkMessage(*(context->GetReqEvent()));
          msg.SetResp();
          msg.SetData( CSMI_BASE::ConvertToBytes<HealthCheckData>(data) );
        }
        else
        {
          // retrieve the data from msg and overwrite the local daemon info
          data = CreateHCDAndSetLDaemon( msg );
        }
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_AGG: Send a response to the requester";

        postEventList.push_back (CreateNetworkEvent(msg, GetNetworkAddress(*(context->GetReqEvent())), context) );
      }

      // set the state to done
      context->SetAuxiliaryId(DONE_STATE);
      break;
    }

    default:
    break;
  }

}
