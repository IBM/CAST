/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_network_message_state.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_NETWORK_MESSAGE_STATE_H__
#define __CSMI_NETWORK_MESSAGE_STATE_H__

#include "csmi_handler_state.h"
#include "helpers/EventHelpers.h"


/** @brief The base class for states that expect Network events. */
class NetworkMessageState: public CSMIHandlerState
{
    using CSMIHandlerState::CSMIHandlerState;

public:
    /** @brief Invoked by the Handler's Process Function.
     *
     * Note: This is the finalization of the Process function.
     *
     * @param ctx The context of the event. Cast in the invoking function to 
     *  prevent every implementation of Process from having perform error checking.
     * @param aEvent The event that triggered this call.
     * @param postEventList The list of events to push new events to.
     */
    virtual void Process( 
        csm::daemon::EventContextHandlerState_sptr& ctx,
        const csm::daemon::CoreEvent &aEvent, 
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final
    {
        LOG(csmapi, trace) << "Enter NetworkMessageState State ID: " << ctx->GetAuxiliaryId();
        
        // If the Content type of this event is not a NetworkEvent, react.
        if ( !aEvent.HasSameContentTypeAs(csm::daemon::helper::NETWORK_EVENT_TYPE) )
        {

            if ( aEvent.HasSameContentTypeAs(csm::daemon::helper::TIMER_EVENT_TYPE) )
            {
                this->HandleTimeout(ctx, aEvent, postEventList); 
                return;
            }
            
            LOG(csmapi, error) << "NetworkMessageState: Expected a NetworkMessage";

            ctx->SetErrorCode(CSMERR_BAD_EVENT_TYPE);
            ctx->AppendErrorMessage("Did not recieve a NETWORK_EVENT_TYPE");
            this->HandleError( ctx, *(ctx->GetReqEvent()), postEventList );
    
            return; 
        }

        // TODO is there a case where the event can't be cast?
        csm::daemon::NetworkEvent *network_event = (csm::daemon::NetworkEvent *)&aEvent;
        csm::network::MessageAndAddress content = network_event->GetContent();

        // Set the user ID of the context.
        ctx->SetPrivateCheckConfig(content._Msg.GetUserID(), 
            content._Msg.PrivateRequiresUserCompare());
    
        if (content._Msg.GetErr()) 
        {
            LOG(csmapi, error) << ctx << 
                "NetworkMessageState: An error was detected while receiving a "
                "Network Message.";

            // Unpack the error
            std::string dataStr = content._Msg.GetData();
            csmi_err_t* error = csmi_err_unpack( dataStr.c_str(), dataStr.size());

            this->ProcessError(ctx, error);

            if ( ctx->GetErrorCode() >= csmi_cmd_err_t_MAX)
            {
                ctx->SetErrorCode(CSMERR_GENERIC);
            }

            if ( ctx->GetErrorCode() == CSMERR_TIMEOUT )
            {
                this->HandleTimeout(ctx, aEvent, postEventList);
            }

            // TODO Pass the error message for multicast events?
            csmi_err_free(error);
            this->HandleError( ctx, *(ctx->GetReqEvent()), postEventList );
            return;
        }
        
        // Handle the network message, but if an error happened deal with it.
        if ( ! this->HandleNetworkMessage(content, postEventList, ctx) )
            this->HandleError( ctx, *(ctx->GetReqEvent()), postEventList );
    }


protected:
    /** @brief Handles the parsing and storage of the recieved NetworkMessage.
     *  @note If an error occurs, place the error details in the ctx object and
     *      return false. Allow the process function to perform error handling.
     *  
     *  @param[in] content The message received by the CSMIHandlerState.
     *  
     *  @param[in] postEventList The Event Queue.  
     * @todo is the postEventList Thread-Safe?
     *  @param[in,out] ctx The context of the event, holds details persistent across states.
     *
     *  @return true Everything went as planned and no errors were found.
     *  @return false An error was detected and details were placed in the context.
     */
    virtual bool HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr& ctx ) = 0;


    /**
     * @brief Processes an error before responding to it, generally just appends the error message.
     *
     * This should only be reimplemented if the API requires advanced details about the 
     * source of the message and the message is known to have source information in
     * its message.
     *
     * @param[in,out] ctx The context object, this will track the metadata extracted from 
     *      the @p error struct.
     * @param[in]     error The Error object representing the 
     */
    virtual void ProcessError(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        const csmi_err_t* error) 
    { 
        if ( error )
        {
            ctx->AppendErrorMessage( std::string( error->errmsg ) );
            if (error->errcode == ETIMEDOUT )
            {
                ctx->SetErrorCode( CSMERR_TIMEOUT );
            }
            else
            {
                ctx->SetErrorCode( error->errcode );
            }
        }
        else 
        {
            ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR );
            ctx->AppendErrorMessage( "Unable to unpack error message" );
        }
    };
};

#endif
