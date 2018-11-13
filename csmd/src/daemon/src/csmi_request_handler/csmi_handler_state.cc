/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_handler_state.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi_handler_state.h"

bool CSMIHandlerState::PushEvent( 
    csm::daemon::CoreEvent *reply, 
    uint64_t errorType, 
    const std::string& errorMessage, 
    csm::daemon::EventContextHandlerState_sptr ctx,
    std::vector<csm::daemon::CoreEvent*>& postEventList, 
    bool errorOnFail )
{

    // Verify that a DBReqEvent was successfully created:
    // If it was successful push the event onto the list and return true.
    // Else handle the error.
    if ( reply )
    {
        ctx->SetAuxiliaryId( GetSuccessState() );
        postEventList.push_back(reply);
    }
    else
    {
        ctx->SetErrorCode(errorType);
        ctx->SetErrorMessage(errorMessage);
            
        // If the flag was set, set the final state and make an error event.
        // Else set the state to the Failure state and move on.
        if( errorOnFail )
        {
            ctx->SetAuxiliaryId( GetFinalState() );
            uint32_t bufferLen = 0;
            char* buffer = ctx->GetErrorSerialized(&bufferLen);

            postEventList.push_back( csm::daemon::helper::CreateErrorEvent(
                buffer, bufferLen,  *(ctx->GetReqEvent())));

            if ( buffer ) free(buffer);
        }
        else
            ctx->SetAuxiliaryId( GetFailureState() );
    }

    return reply != nullptr;
}

bool CSMIHandlerState::PushDBReq(
    csm::db::DBReqContent const &dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    bool errorOnFail )
{
    csm::daemon::DBReqEvent *dbevent =
        new csm::daemon::DBReqEvent(
            dbPayload, 
            csm::daemon::EVENT_TYPE_DB_Request, 
            ctx);

    return PushEvent(dbevent, CSMERR_DB_ERROR, "Unable to build the DBReqEvent!",
        ctx, postEventList, errorOnFail);

}
    
bool CSMIHandlerState::PushMCAST( 
    csm::network::Message message,
    std::vector<std::string>& targets,
    csm::daemon::EventContextHandlerState_sptr ctx,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    uint64_t targetState,
    bool errorOnFail )
{
    if( targets.size() == 0 )
    {
        ctx->SetErrorCode(CSMERR_MULTI_GEN_ERROR);
        ctx->SetErrorMessage("Multicast had no targets!");
        return false;
    }

    // Sort the targets.
    std::sort(targets.begin(), targets.end());

    // Set up the message count trackers.
    ctx->SetExpectedNumResponses(targets.size());
    ctx->SetReceivedNumResponses(0);
    message.SetReservedID(ctx->GetRunID());

    csm::daemon::NetworkEvent *reply = 
        csm::daemon::helper::CreateMulticastEvent(message, targets, ctx); 
   
    bool success = PushEvent( reply, CSMERR_MULTI_GEN_ERROR, "Unable to build the Multicast!",
        ctx, postEventList, errorOnFail);

    // If the multicast was a success spawn a timeout.
    if ( success )
    {
        // If the state was not set the Push Event will have already set to success.
        if ( targetState != UINT64_MAX ) ctx->SetAuxiliaryId( targetState );

        PushTimeout( ctx, postEventList );
    }

    return success;
}
       
bool CSMIHandlerState::ForwardToMaster( 
    csm::network::Message message,
    csm::daemon::EventContextHandlerState_sptr ctx,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    bool errorOnFail )
{
    // Track success.
    bool success = false;

    // Validate the message first.
    if ( message.Validate() )
    {
        CSMI_BASE* handler = static_cast<CSMI_BASE*>(ctx->GetEventHandler());
        
        csm::network::MessageAndAddress content( message, handler->GetAbstractMaster() );
        
        // Generate the event then push it to the master.
        csm::daemon::NetworkEvent *event = csm::daemon::helper::CreateNetworkEvent(
            content,
            ctx);

        success = PushEvent(
            event,
            CSMERR_CMD_UNKNOWN,
            "Unable to forward message to the Master Daemon.",
            ctx,
            postEventList,
            errorOnFail);

        ctx->SetAuxiliaryId( GetAlternateState() );

        LOG(csmapi,debug) <<  "Forwarding message to master daemon.";
    }
    else
    {
        ctx->SetErrorCode(CSMERR_GENERIC); // TODO better error code.
        ctx->SetErrorMessage("Unable to validate forwarded network message.");
    }
    return success;
}

bool CSMIHandlerState::PushReply(
    const char* buffer,
    const uint32_t bufferLength,
    csm::daemon::EventContextHandlerState_sptr ctx,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    bool byAggregator )
{
    csm::daemon::NetworkEvent *reply = nullptr;

    if ( !byAggregator )
    {
        reply = csm::daemon::helper::CreateReplyNetworkEvent( 
            buffer, bufferLength, *(ctx->GetReqEvent()), ctx, false);
    }
    else
    {
        // TODO is there a better way?
        CSMI_BASE* handler = static_cast<CSMI_BASE*>(ctx->GetEventHandler());
        csm::network::Message message = dynamic_cast<const csm::daemon::NetworkEvent *>( 
            ctx->GetReqEvent())->GetContent()._Msg;

        // Set as a resonse and give it the new data.
        message.SetFlags(CSM_HEADER_RESP_BIT);
        message.SetData(std::string(buffer, bufferLength));
        message.CheckSumUpdate();

        csm::network::MessageAndAddress content( message, handler->GetAbstractAggregator() );
        reply = csm::daemon::helper::CreateNetworkEvent(content, ctx);
    }
     
    bool success = PushEvent(
        reply, 
        CSMERR_CMD_UNKNOWN, 
        "Unable to generate reply message, throwing error instead.", 
        ctx, 
        postEventList,
        true);

    ctx->SetAuxiliaryId( GetFinalState() );

    // XXX Does this need a return?
    return success;
}

void CSMIHandlerState::PushRASEvent(
    csm::daemon::EventContextHandlerState_sptr ctx,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    const std::string &msg_id,
    const std::string &location_name,
    const std::string &raw_data,
    const std::string &kvcsv)
{
    CSMI_BASE* handler = static_cast<CSMI_BASE*>(ctx->GetEventHandler());
    csm::daemon::NetworkEvent *reply = 
        csm::daemon::helper::CreateRasEventMessage(msg_id, location_name, raw_data, 
                kvcsv, handler->GetAbstractMaster());

    if ( reply )
        postEventList.push_back(reply);
    else
        LOG(csmapi, error) <<  "Unable to push RAS";
}

void CSMIHandlerState::DefaultHandleError( 
    csm::daemon::EventContextHandlerState_sptr ctx,
    const csm::daemon::CoreEvent &aEvent,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    bool byAggregator ) 
{
    // Prepend the error message.
    std::string prependString = ctx->GenerateUniqueID() + ";" ;
    ctx->PrependErrorMessage( prependString, ' ');

    LOG(csmapi, error) <<  ctx->GetErrorMessage();

    ctx->SetAuxiliaryId( GetFinalState() );

    if ( !byAggregator )
    {
        uint32_t bufferLen = 0;
        char* buffer = ctx->GetErrorSerialized(&bufferLen);
        
        postEventList.push_back( csm::daemon::helper::CreateErrorEvent(
            buffer, bufferLen,  *(ctx->GetReqEvent())));

        if ( buffer ) free(buffer);
    }
    else
    {
        postEventList.push_back(
            csm::daemon::helper::CreateErrorEventAgg(
                ctx->GetErrorCode(),
                ctx->GetErrorMessage(),
                aEvent,
                ctx ) );
    }

    // Clear the user data, this will invoke its destructor if present.
    ctx->SetUserData(nullptr);
}

void CSMIHandlerState::PushTimeout (
    csm::daemon::EventContextHandlerState_sptr ctx,
    std::vector<csm::daemon::CoreEvent*>& postEventList)
{
    if ( GetTimeoutLength() != csm::daemon::helper::BAD_TIMEOUT_LEN )
        postEventList.push_back(
            csm::daemon::helper::CreateTimerEvent(
                GetTimeoutLength(),
                ctx->GetAuxiliaryId(),
                ctx ) );
}

void CSMIHandlerState::HandleTimeout(
    csm::daemon::EventContextHandlerState_sptr ctx,
    const csm::daemon::CoreEvent &aEvent,
    std::vector<csm::daemon::CoreEvent*>& postEventList )
{
    LOG(csmapi, trace) << "HandleTimeout: Enter";
    // FIXME Potential segfault?
    csm::daemon::TimerContent timerContent = ((csm::daemon::TimerEvent*)&aEvent)->GetContent();

    // XXX Apparently it's possible for the context to be empty?
    // If we're in the completed state and ended up here something else
    // is going to take precedence over a timeout.
    if ( ctx && ctx->GetAuxiliaryId() != timerContent.GetTargetStateId( ) ) 
    {
        //LOG(csmapi, warning) << 
        //    "HandleTimeout: context state doesn't match timer target state, discarding.";
        return;
    }
    
    // TODO make the Error message parameterized:
    ctx->SetErrorCode(CSMERR_TIMEOUT);
    ctx->AppendErrorMessage( 
        "Timeout Occured in State: " + std::to_string( ctx->GetAuxiliaryId() ) );

    // If there's no special timeout state, just send an erorr event and complete.
    if( GetTimeoutState()  == GetFinalState() )
    {
        LOG(csmapi, trace) << "HandleTimeout: Transitioning to Final State";
        uint32_t bufferLen = 0;
        char* buffer = ctx->GetErrorSerialized(&bufferLen);
        csm::daemon::CoreEvent *reply = csm::daemon::helper::CreateErrorEvent(
            buffer, bufferLen,  *(ctx->GetReqEvent()));
        if ( buffer ) free(buffer);

        ctx->SetAuxiliaryId( GetTimeoutState() );

        if (reply)
            postEventList.push_back(reply);
        else
            LOG(csmapi, error) << "Failed to push timeout response!"; // TODO Error code handle?
    }
    else
    {
        LOG(csmapi, trace) << "HandleTimeout: Transitioning to state " << ctx->GetAuxiliaryId();
        GenerateTimeoutResponse( ctx, postEventList );
    }

    LOG(csmapi, trace) << "HandleTimeout: Exit";
}
