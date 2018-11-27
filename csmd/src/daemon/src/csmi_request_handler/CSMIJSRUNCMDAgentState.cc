/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIJSRUNCMDAgentState.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "CSMIJSRUNCMDAgentState.h"
#include "helpers/csm_handler_exception.h"
#include "helpers/AgentHandler.h"

#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include "csmi/include/csm_api_consts.h"
#include "csmi/include/csm_api.h"
#include <syslog.h>

#define STATE_NAME "JSRUNCMDAgentState:"

bool JSRUNCMDAgentState::HandleNetworkMessage(
    const csm::network::MessageAndAddress content,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
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
    csmi_jsrun_cmd_payload_t *jsrun_cmd = nullptr;
    
    // EARLY RETURN
    if( csm_deserialize_struct( csmi_jsrun_cmd_payload_t, &jsrun_cmd,
            content._Msg.GetData().c_str(), content._Msg.GetDataLen() ) != 0 )
    {
        ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Message: Unable to JSRUN Command struct");
        return false;
    }

    if ( jsrun_cmd->hostname )
        free(jsrun_cmd->hostname);
    
    // Clone the hostname for RAS reporting.
    jsrun_cmd->hostname = 
        strdup( csm::daemon::Configuration::Instance()->GetHostname().c_str() );
    
    // If the string was empty free it and set it to NULL.
    if ( strcmp ( jsrun_cmd->jsm_path, "" ) == 0  )  
    {
        free(jsrun_cmd->jsm_path);
        jsrun_cmd->jsm_path = nullptr;
    }

    int error_code = CSMERR_JSRUN_CMD_ERROR;
    try{
        // Execute the JSRUN Command.
        error_code = csm::daemon::helper::ExecuteJSRUN(jsrun_cmd->jsm_path, jsrun_cmd->allocation_id, 
            jsrun_cmd->user_id, jsrun_cmd->kv_pairs);
    }
    catch(const csm::daemon::helper::CSMHandlerException& e)
    {
        std::string error = "Message: ";
        error.append(e.what());
        ctx->SetErrorMessage(error);
        ctx->SetErrorCode(CSMERR_CGROUP_FAIL);
        error_code = CSMERR_CGROUP_FAIL;
    }
    catch(const std::exception& e)
    {
        std::string error = "Message: ";
        error.append(e.what());
        ctx->SetErrorMessage(error);
        ctx->SetErrorCode(CSMERR_CGROUP_FAIL);
        error_code = CSMERR_CGROUP_FAIL;
    }
    

    // If the initialization or reversion was successful reply to the master daemon.
    if ( error_code  == 0 )
    {
        // Return the results to the Master via the Aggregator.
        char *buffer          = nullptr;
        uint32_t bufferLength = 0;

        csm_serialize_struct( csmi_jsrun_cmd_payload_t, jsrun_cmd, &buffer, &bufferLength );

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
        ctx->SetErrorCode(CSMERR_JSRUN_CMD_ERROR);
        if ( error_code != INT_MAX ) 
        {
            ctx->SetErrorMessage("JSRUN command error code: " + std::to_string(error_code));
        }
        else
        {
            std::string error = "Message: jsm_path not legal/found ";
            error.append(jsrun_cmd->jsm_path);
            ctx->SetErrorMessage(error);
        }
    }

    // Clean up the struct.
    csm_free_struct_ptr(csmi_jsrun_cmd_payload_t, jsrun_cmd);

    LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Exit";

    return success;
}

void JSRUNCMDAgentState::HandleError(
    csm::daemon::EventContextHandlerState_sptr& ctx,
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

    LOG( csmapi, error ) << STATE_NAME " Error Code: " << ctx->GetErrorCode() << " ;Error Message: " <<ctx->GetErrorMessage() << ";";


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

