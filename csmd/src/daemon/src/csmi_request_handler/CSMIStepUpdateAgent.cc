/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIStepUpdateAgent.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "CSMIStepUpdateAgent.h"
#include "helpers/csm_handler_exception.h"
#include "helpers/cgroup.h"
#include "helpers/AgentHandler.h"

#include "csmi/include/csm_api_consts.h"
#include "csmi/include/csm_api.h"
#include <syslog.h>

#define STATE_NAME "StepAgentUpdateState:"
#define SYSTEM_FLAGS ""

//static char* SYSTEM_FLAGS = strdup(""); ///< Constant for execute privileged

bool StepAgentUpdateState::HandleNetworkMessage(
    const csm::network::MessageAndAddress content,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Enter";

    // Return status of this function.
    bool success = false;
    
    // EARLY RETURN If peer to peer is detected, assume this was invoked on the agent and forward.
    if ( content.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP )
    {
        LOG(csmapi, trace) << STATE_NAME " Detected PTP connection, forwarding request to master.";
        return this->ForwardToMaster(content._Msg, ctx, postEventList); 
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
    csmi_allocation_step_mcast_payload_t *step;
    
    // EARLY RETURN
    if( csm_deserialize_struct( csmi_allocation_step_mcast_payload_t, &step, 
            content._Msg.GetData().c_str(), content._Msg.GetDataLen()) != 0 )
    {
        ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Message: Unable to unpack step struct");
        return false;
    }

    // TODO should the node stat collection happen here before going deeper?    
    
    // The create flag was set intialize the node.
    // Else, this is an instruction to revert the node.
    if ( step->begin )
        success = StepBegin( step, postEventList, ctx ); 
    else
        success = StepEnd( step, postEventList, ctx );
    
    // If the initialization or reversion was successful reply to the master daemon.
    if ( success )
    {
        if ( step->hostname )
            free(step->hostname);

        step->hostname = strdup( csm::daemon::Configuration::Instance()->GetHostname().c_str() );
    
        // Return the results to the Master via the Aggregator.
        char *buffer          = nullptr;
        uint32_t bufferLength = 0;
        csm_serialize_struct( csmi_allocation_step_mcast_payload_t, step, &buffer, &bufferLength );

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

            LOG(csmapi,info) << ctx->GetCommandName() << ctx <<
                "Allocation ID: " << step->allocation_id <<
                "; Step Id: "     << step->step_id <<
                "; Message: Agent completed successfully;";

        }
        else
        {
            ctx->SetErrorCode(CSMERR_MSG_PACK_ERROR);
            ctx->SetErrorMessage("Message: Unable pack response to the Master Daemon;");
            success = false; 
        }
    }

    // Clean up the struct.
    csm_free_struct_ptr( csmi_allocation_step_mcast_payload_t, step );

    LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Exit";

    return success;
}

bool StepAgentUpdateState::StepBegin(
    csmi_allocation_step_mcast_payload_t *step,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr ctx )
{ 
    LOG( csmapi, trace ) << STATE_NAME ":InitNode: Enter ";

    // Track the revert status.
    bool success = true;
    
    // Log the allocation.
    openlog("csmd", LOG_NDELAY , LOG_USER);
    syslog(LOG_INFO, "Allocation %lu Step %lu Begin",
           step->allocation_id, step->step_id);
    closelog();

    LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
        "; Step Id: " << step->step_id <<
        "; Message: Step begin;";
    
    // 0. Export Environment Variables.
    csm_export_env( 
        step->allocation_id, 
        -1,
        -1,
        "")//step->user_name)

    // Free this since it's all done.
    //free( step->user_name ); 
    //step->user_name = nullptr;
    
    // 1. Run the Prolog.
    // EARLY RETURN!

    LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
        "; Step Id: " << step->step_id <<
        "; User Flags: " << step->user_flags << 
        "; Message: Prolog start;";

    success = csm::daemon::helper::ExecutePrivileged(
            step->user_flags, (char*)SYSTEM_FLAGS, ctx, true, true );

    if ( success )
    {
        LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
            "; Step Id: " << step->step_id <<
            "; User Flags: " << step->user_flags << 
            "; Message: Prolog success;";
    }
    else
    {
        LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
            "; Step Id: " << step->step_id <<
            "; User Flags: " << step->user_flags << 
            "; Message: Prolog failure;";
    }

    // Set the Daemon Run Mode interaction.
    // FIXME do we need this?
    //postEventList.push_back( csm::daemon::helper::CreateJobStartSystemEvent(ctx) );

    LOG( csmapi, trace ) << STATE_NAME ":InitNode: Exit";
    
    return success;
}

bool StepAgentUpdateState::StepEnd( 
    csmi_allocation_step_mcast_payload_t *step,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":RevertNode: Enter";
    
    // Track the revert status.
    bool success = true;

    // Log the step end.
    openlog("csmd", LOG_NDELAY , LOG_USER);
    syslog(LOG_INFO, "Allocation %lu Step %lu End",
           step->allocation_id, step->step_id);
    closelog();

    LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
        "; Step Id: " << step->step_id <<
        "; Message: Step end;";

    // 0. Export Environment Variables.
    csm_export_env( 
        step->allocation_id, 
        -1,
        -1,
        "") //step->user_name ) 

    // Free this since it's all done.
    //free( step->user_name ); 
    //step->user_name = nullptr;
    
    // 1. Execute the Epilog.
    LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
        "; Step Id: " << step->step_id <<
        "; User Flags: " << step->user_flags << 
        "; Message: Epilog start;";

    success = csm::daemon::helper::ExecutePrivileged(
                step->user_flags, (char*)SYSTEM_FLAGS, ctx, false, true );

    if ( success )
    {
        LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
            "; Step Id: " << step->step_id <<
            "; User Flags: " << step->user_flags << 
            "; Message: Epilog success;";
    }
    else
    {
        LOG(csmapi, info) << ctx << "Allocation ID: " << step->allocation_id <<
            "; Step Id: " << step->step_id <<
            "; User Flags: " << step->user_flags << 
            "; Message: Epilog failure;";

    }

    LOG( csmapi, trace ) << STATE_NAME ":RevertNode: Exit";
    return success;
}

void StepAgentUpdateState::HandleError(
    csm::daemon::EventContextHandlerState_sptr ctx,
    const csm::daemon::CoreEvent &aEvent,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    bool byAggregator )
{ 
    // Append the hostname to the start of the error message.
    // FIXME Workaround for Beta 1
    // ============================================================================
    ctx->SetErrorMessage( csm::daemon::Configuration::Instance()->GetHostname() + "; " + ctx->GetErrorMessage());
    // ============================================================================ 

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
} 

