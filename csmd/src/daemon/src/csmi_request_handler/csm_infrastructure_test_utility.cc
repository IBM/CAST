/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_infrastructure_test_utility.cc

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

void CSM_INFRASTRUCTURE_TEST_UTILITY::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  //uint64_t flow_timer = 5000; // 5 seconds
  
  if ( isSystemEvent(aEvent) )
  {
    TestSystemEvent(aEvent);
    return;
  }

  csm::daemon::EventContext_sptr context = aEvent.GetEventContext();

  //LOG(csmd, error) << "!!!! " << context->GetAuxiliaryId(); // cause a seg fault in this thread

  //note: Only Master will start with INITIAL_TEST and the rest will start with FLOW test
  if (context == nullptr) context = EventContextTest_sptr(new EventContextTest(this, INITIAL_STATE, CopyEvent(aEvent)));
  EventContextTest_sptr ctx = std::dynamic_pointer_cast<EventContextTest> (context);
    
  LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_UTILITY: AuxiliaryID = " << ctx->GetAuxiliaryId();

  csm::daemon::NetworkEvent *event = nullptr;
  switch (ctx->GetAuxiliaryId())
  {
    case INITIAL_STATE:
    {
      ctx->SetAuxiliaryId(WAIT_STATE);

      if( !isNetworkEvent( aEvent ) )
      {
        LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Invalid event in Infrastructure test. Expected network event.";
        return;
      }

      csm::network::MessageAndAddress msgAddr = GetNetworkContent( aEvent );

      // an initial request from csm api client. expected no payload.
      // now we can create our own defined HealthCheckData data and append it in the msg

      if( msgAddr._Msg.GetCommandType() != CSM_infrastructure_test)
      {
        postEventList.push_back( CreateErrorEvent(EINVAL,
                                                  "FATAL: called infrastructure test with wrong command type. ",
                                                  aEvent ) );
        return;
      }

      // will add the local daemon info in the HealthCheckData
      ctx->_cached_data = CreateHCDAndSetLDaemon(msgAddr._Msg);

      // if this request comes from a local client, then we're the intiator of the request
      // otherwise, this is the masters request to respond to the infrastructure-test
      if( msgAddr.GetAddr()->GetAddrType() == csm::network::CSM_NETWORK_TYPE_LOCAL )
      {
        // Forward the received request from csm api client to the MQTT/master.
        // context is important so that we can find the client's address when the reply is back
        if ( (event = CreateNetworkEvent(msgAddr._Msg, _AbstractMaster,
                                         ctx)) != nullptr)
        {
          LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Starting tests. Forwarding request to master";
          postEventList.push_back(event );
        }
        else
        {
          LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Error creating request to Master";
          postEventList.push_back( CreateErrorEvent(EINVAL,
                                                    "Failed master request creation.",
                                                    aEvent ) );
        }

        postEventList.push_back( CreateTimerEvent(_utility_timeout, ctx, WAIT_STATE) );
        ctx->SetAuxiliaryId(WAIT_STATE);
      }
      else  // non-local request from master
      {
        ctx->_cached_data._util_info.push_back( boost::make_shared<UtilInfo>( ctx->_cached_data._local ) );
        std::string dataStr = CSMI_BASE::ConvertToBytes<HealthCheckData>( ctx->_cached_data );

        if ( (event = CreateReplyNetworkEvent( dataStr.c_str(),
                                               dataStr.length(),
                                               aEvent,
                                               ctx,
                                               false ) ) != nullptr)
        {
          LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Responding to Master: " << event->GetContent().GetAddr()->Dump();
          postEventList.push_back( event );
          ctx->SetAuxiliaryId(DONE_STATE);
        }
        else
          LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Failed to create response to Master.";
      }
      break;
    }
    
    case WAIT_STATE:
    {
      if( isTimerEvent( aEvent ) )
      {
        if( dynamic_cast<const csm::daemon::TimerEvent*>( &aEvent )->GetContent().GetTargetStateId() != WAIT_STATE )
          return;

        // handle timeout
        csm::daemon::NetworkEvent *resp = CreateTimeoutNetworkResponse( aEvent,
                                                                        ETIMEDOUT,
                                                                        "Infrastructure test timeout on Utility" );
        if( resp != nullptr )
          postEventList.push_back( resp );
        else
        {
          LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Failed to create timeout error response. No response will be sent.";
        }
      }
      else
      {
        csm::network::Message msg = GetNetworkMessage(aEvent);
        // start with the cached data in case the master responds with an error
        HealthCheckData data;
        if (msg.GetErr())
        {
          LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Get an Error NetworkEvent...";

          data = ctx->_cached_data;
          // either ECOMM or ETIMEOUT
          csmi_err_t *err = csmi_err_unpack(msg.GetData().c_str(), msg.GetDataLen());
          if (err)
          {
            data._errmsg = std::string(err->errmsg);
            csmi_err_free(err);
          }
          else data._errmsg = std::string("Failed to retrieve the error payload");

          msg = GetNetworkMessage(*(ctx->GetReqEvent()));
          msg.SetResp();
          msg.SetData( CSMI_BASE::ConvertToBytes<HealthCheckData>(data) );
        }
        else
        {
          // retrieve the data from msg and overwrite the local daemon info
          data = CreateHCDAndSetLDaemon( msg );
        }
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Send a response to the requester";

        postEventList.push_back (CreateNetworkEvent(msg, GetNetworkAddress(*(ctx->GetReqEvent())), ctx) );
      }

      // set the state to done
      ctx->SetAuxiliaryId(DONE_STATE);
      
      break;
    }
    
    case DONE_STATE:
      if( ! isTimerEvent( aEvent ) )
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Triggered in DONE_STATE by non-timer event.";
        return;
      }
      else
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_UTILITY: Timer event after test completion as expected.";
        return;
      }
      break;
    default:
      LOG( csmd, warning ) << "FATAL: Invalid state in Infrastructure test: "+ std::to_string(ctx->GetAuxiliaryId() );
      postEventList.push_back( CreateErrorEvent(EINVAL,
                                                std::string( "FATAL: Invalid state in Infrastructure test: " ) + std::to_string(ctx->GetAuxiliaryId() ),
                                                aEvent ) );
      break;
  }
}
