/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMICGROUPLogin.cc
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMICGROUPLogin.h"
#include "helpers/cgroup.h"
#include "helpers/csm_handler_exception.h"

#define INPUT_STRUCT csm_cgroup_login_input_t

int migrate_pid( pid_t pid, int64_t allocationId )
{
    int retCode = CSMI_SUCCESS;

    // Attempt to migrate the PID.    
    try
    {
        csm::daemon::helper::CGroup cgroup( allocationId );
        cgroup.MigratePid( pid );
    }
    catch(const csm::daemon::helper::CSMHandlerException& e)
    {
        retCode=CSMERR_CGROUP_FAIL;
    }
    catch(const std::exception& e)
    {
        retCode=CSMERR_CGROUP_FAIL;

    }

    return retCode;
}

bool CGLoginInitState::HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi, trace) << "Enter CGLoginInitState";
    bool success = true;                             ///< Exit status of the state.
    int  errCode = CSMERR_CGROUP_FAIL;
    INPUT_STRUCT *state_args; ///< User arguments.

    // EARLY RETURN
    if( csm_deserialize_struct( INPUT_STRUCT, &state_args,
            content._Msg.GetData().c_str(), content._Msg.GetDataLen()) != 0 )
    {
        // Set the param
        ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage(std::string("Deserialization Failed"));
        return false;
    }

    // 0. Get the user from the active list.
    std::ifstream activelistStream("/etc/pam.d/csm/activelist");
    if( activelistStream.is_open() )
    {
        std::string line;
        while ( errCode != CSMI_SUCCESS && std::getline( activelistStream, line ) )
        {
            int delim = line.find(";");
            if( line.compare(0, delim, state_args->user_name) == 0 )
            {
                errCode = CSMI_SUCCESS;

                if(state_args->migrate_pid)
                {
                    std::string alloc = line.substr(delim +1);
                    errCode=migrate_pid(state_args->pid, strtoll( alloc.c_str(), nullptr, 10 ));
                }
                
                if ( errCode == CSMI_SUCCESS )
                    break; // once user has been located in active list, we are done.
            }
        }
    }
    
    csm_free_struct_ptr( INPUT_STRUCT, state_args );

    // Determine if the user is allowed.
    success = errCode == CSMI_SUCCESS;
    if (success)
    {
        // Reply to the user with an empty payload.
        this->PushReply( nullptr, 0, ctx, postEventList);
    }
    else
    {
        ctx->SetErrorCode(errCode);
        ctx->SetErrorMessage("User not authorized.");
    }

    LOG(csmapi, trace) << "Exit CGLoginInitState";
    return success;
}

