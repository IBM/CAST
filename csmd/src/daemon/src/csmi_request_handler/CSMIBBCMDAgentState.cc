/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBCMDAgentState.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "CSMIBBCMDAgentState.h"
#include "helpers/csm_handler_exception.h"
#include "helpers/AgentHandler.h"

#include "csmi/src/bb/include/csmi_bb_type_internal.h"
#include "csmi/include/csm_api_consts.h"
#include "csmi/include/csm_api.h"
#include <syslog.h>



#define STATE_NAME "BBCMDAgentState:"

bool BBCMDAgentState::HandleNetworkMessage(
    const csm::network::MessageAndAddress content,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Enter";

    // Return status of this function.
    bool success = false;
    
    // EARLY RETURN This needs to invoke the error handler NOT through the aggregator.
    if ( content.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP )
    {
        //LOG(csmapi,error) << STATE_NAME " Expecting a PTP connection.";
        ctx->SetErrorCode(CSMERR_BAD_ADDR_TYPE);
        ctx->SetErrorMessage("Message: This API may not be called from a Compute Node");
        return false;
    }

    // EARLY RETURN
    if ( content._Msg.GetDataLen() == 0  )
    {
        ctx->SetErrorCode(CSMERR_PAYLOAD_EMPTY);
        ctx->SetErrorMessage("Message: Payload received was empty on " + 
            csm::daemon::Configuration::Instance()->GetHostname());
        return false;
    }

    // Receive the payload after we know this is a valid invocation.
    csmi_bb_cmd_payload_t *bb_cmd = nullptr;
    
    // EARLY RETURN
    if( csm_deserialize_struct( csmi_bb_cmd_payload_t, &bb_cmd,
            content._Msg.GetData().c_str(), content._Msg.GetDataLen() ) != 0 )
    {
        ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Message: Unable to Burst Buffer Command struct");
        return false;
    }

    if ( bb_cmd->hostname )
        free(bb_cmd->hostname);
    
    // Clone the hostname for RAS reporting.
    bb_cmd->hostname = 
        strdup( csm::daemon::Configuration::Instance()->GetHostname().c_str() );
    
    char* cmd_out = nullptr;
    bb_cmd->bb_cmd_int = csm::daemon::helper::ExecuteBB(bb_cmd->bb_cmd_str, &cmd_out, bb_cmd->bb_cmd_int);
    
    // Copy the command string.
    if (bb_cmd->bb_cmd_str) free(bb_cmd->bb_cmd_str);
    bb_cmd->bb_cmd_str = cmd_out;

    // If the initialization or reversion was successful reply to the master daemon.
    if ( bb_cmd->bb_cmd_int  == 0 )
    {
        // Return the results to the Master via the Aggregator.
        char *buffer          = nullptr;
        uint32_t bufferLength = 0;

        csm_serialize_struct( csmi_bb_cmd_payload_t, bb_cmd, &buffer, &bufferLength );

        if( buffer )
        {
            // Push the reply through the aggregator.
            this->PushReply(
                buffer,
                bufferLength,
                ctx,
                postEventList,
                true);

            free( buffer );

            success=true;
            LOG(csmapi,info) << ctx->GetCommandName() << ctx <<
                "; Message: Agent completed successfully;";
                
        }
        else
        {
            ctx->SetErrorCode(CSMERR_MSG_PACK_ERROR);
            ctx->SetErrorMessage("Message: Unable pack response to the Master Daemon;");
        }
    }
    else
    {
        ctx->SetErrorCode(CSMERR_BB_CMD_ERROR);
        ctx->SetErrorMessage("Message: " + std::to_string(bb_cmd->bb_cmd_int) + " - " + cmd_out);
    }

    // Clean up the struct.
    csm_free_struct_ptr(csmi_bb_cmd_payload_t, bb_cmd);

    LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Exit";

    return success;
}

void BBCMDAgentState::HandleError(
    csm::daemon::EventContextHandlerState_sptr ctx,
    const csm::daemon::CoreEvent &aEvent,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    bool byAggregator )
{
    LOG( csmapi, trace ) << STATE_NAME ":HandleError: Enter";

    // Append the hostname to the start of the error message.
    // FIXME Workaround for Beta 1
    // ============================================================================
    ctx->SetErrorMessage(csm::daemon::Configuration::Instance()->GetHostname() + "; " + ctx->GetErrorMessage());
    // ============================================================================

    LOG( csmapi, error ) << STATE_NAME " Error Message: " << ctx->GetErrorCode() << " " <<ctx->GetErrorMessage();


    // FIXME Temporary fix for beta 1!
    // If this was a CSMERR_BAD_ADDR_TYPE don't talk through the aggregator.
    // Otherwise return the error through the aggregator.
    if ( ctx->GetErrorCode() == CSMERR_BAD_ADDR_TYPE )
    {
        CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, false );
    }
    else
    {
        CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, true );
    }

    LOG( csmapi, trace ) << STATE_NAME ":HandleError: Exit";
}

