/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_forward/CSMIStatefulForwardResponse.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIStatefulForwardResponse.h"

void StatefulForwardResponse::Process(
    csm::daemon::EventContextHandlerState_sptr& ctx,
    const csm::daemon::CoreEvent &aEvent,
    std::vector<csm::daemon::CoreEvent*>& postEventList ) 
{
    // Verify that this is a network message being received by the handler, 
    // reject non network events.
    if ( !aEvent.HasSameContentTypeAs(csm::daemon::helper::NETWORK_EVENT_TYPE) )
    {
        LOG(csmapi, warning) << "StatefulForwardResponse: expects a Network event.";
        return;
    }

    // Cast the event and retrieve the message.
    csm::daemon::NetworkEvent *networkEvent = (csm::daemon::NetworkEvent *)&aEvent;
    csm::network::MessageAndAddress content  = networkEvent->GetContent();

    // The forwarding event.
    csm::daemon::NetworkEvent *forwardEvent = nullptr;

    // If a message error was found forward the error to the origination point.
    // Else forward the message to the origination point.
    if ( content._Msg.GetErr() || (! content._Msg.Validate()) )
    {
        LOG(csmapi, warning) << "StatefulForwardResponse: got an error NetworkEvent.";

        std::string dataStr = content._Msg.GetData();
        csmi_err_t* err = csmi_err_unpack( dataStr.c_str(), dataStr.size());
        

        if ( err )
        {
            // TODO renable GetCmdErr
            csmi_cmd_err_t csmi_err = csm::daemon::helper::GetCmdErr(err->errcode);
            if ( csmi_err != CSMERR_NOTDEF )
            {
                uint32_t bufLen;
                char* buffer = csmi_err_pack(csmi_err, err->errmsg, &bufLen);    
                if ( buffer )
                {
                    dataStr = std::string(buffer, bufLen);
                    free(buffer);
                }
            }
            csmi_err_free(err);
        }
        else
        {
            LOG(csmapi, error) << "StatefulForwardResponse: Unable to unpack the error payload.";
        }
        
        // Specialized Network event creation for forwarding.
        const csm::daemon::NetworkEvent *ctxEvent = 
            dynamic_cast<const csm::daemon::NetworkEvent*>(ctx->GetReqEvent());

        // if the context can be cast create a network event.
        if ( ctxEvent )
        {
            csm::network::MessageAndAddress ctxContent = ctxEvent->GetContent();

            csm::network::Message msg = ctxContent._Msg;
            msg.SetErr();
            msg.SetResp();
            msg.SetData(dataStr);
            msg.CheckSumUpdate();
            forwardEvent = csm::daemon::helper::CreateNetworkEvent(
                msg, ctxContent.GetAddr());
        }
    }
    else
    {
       LOG(csmapi, debug) <<  "StatefulForwardResponse: Forwarding a reply from Master to client.";
       forwardEvent = csm::daemon::helper::ForwardReplyNetworkEvent(aEvent, ctx);
    }


    // Regardless of success, kill this handler.
    ctx->SetAuxiliaryId(GetFinalState());

    // If the forward event was set, push it to the event queue
    if ( forwardEvent )
    {
        postEventList.push_back(forwardEvent);
    }
    else
    {
        LOG(csmapi, error) << "StatefulForwardResponse: Failed to post a valid event.";
    }
}


