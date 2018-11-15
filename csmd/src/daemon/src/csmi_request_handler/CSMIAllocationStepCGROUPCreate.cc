/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepCGROUPCreate.cc
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIAllocationStepCGROUPCreate.h"
#include "helpers/cgroup.h"
#include "helpers/csm_handler_exception.h"

#define INPUT_STRUCT   csm_allocation_step_cgroup_create_input_t

bool AllocStepCGCreateInitState::HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{

    LOG(csmapi, trace) << "Enter AllocStepCGCreateInitState";
    bool success = true;                             ///< Exit status of the state.
    INPUT_STRUCT *state_args; ///< User arguments.

    // IF the buffer could be parsed, build the cgroup.
    // ELSE Set the error message.
    if( csm_deserialize_struct( INPUT_STRUCT, &state_args,
            content._Msg.GetData().c_str(), content._Msg.GetDataLen()) == 0 )
    {
        // Make the cgroup object.
        csm::daemon::helper::CGroup cgroup = 
            csm::daemon::helper::CGroup( state_args->allocation_id );

        try
        {
            // Create all of the cgroups.
            for( uint32_t i = 0; i < state_args->num_components; ++i )
            {
                cgroup.CreateCGroup( state_args->cgroup_name, 
                                        state_args->components[i],
                                        state_args->pid );
            }
        }
        catch(const csm::daemon::helper::CSMHandlerException& e)
        {
            ctx->SetErrorCode(e.type());
            ctx->SetErrorMessage(e.what());
            success=false; 
        }
        catch(const std::exception& e)
        {
            std::string error = "CGroup Failed: ";
            error.append(e.what());
            ctx->SetErrorMessage(error);
            ctx->SetErrorCode(CSMERR_CGROUP_FAIL);
            success=false;
        }

        csm_free_struct_ptr( INPUT_STRUCT, state_args );

        if (success)
        {
            // Reply to the user with an empty payload.
            this->PushReply(
                nullptr,
                0,
                ctx, 
                postEventList);
        }
    }
    else
    {
        LOG(csmapi,error) << "Deserialization failed.";

        // Set the param
        ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage(std::string("Deserialization Failed"));
        success = false;
    }


    LOG(csmapi, trace) << "Exit AllocStepCGCreateInitState";
    return success;
}

