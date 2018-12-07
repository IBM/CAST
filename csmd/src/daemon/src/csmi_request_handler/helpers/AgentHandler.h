/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/AgentHandler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#ifndef _AGENT_HANDLER_H_
#define _AGENT_HANDLER_H_
#include "Agent.h"
#include "../csmi_handler_state.h" // TODO Fix this?

#define CSM_P_PROLOG "/opt/ibm/csm/prologs/privileged_prolog"
#define CSM_P_EPILOG "/opt/ibm/csm/prologs/privileged_epilog"
#define CSM_P_TYPE   "--type"
#define CSM_P_ALLOCATION  "allocation"
#define CSM_P_STEP  "step"

#define CSM_P_USER_FLAG "--user_flags"
#define CSM_P_SYSTEM_FLAG "--sys_flags"

namespace csm {
namespace daemon {
namespace helper {

int ScanForPrivleged(bool isProlog, int64_t allocationId=0, bool isShared=false);

/**
 * @brief Executes either a privileged prolog or epilog as either a step or allocation.
 *
 * @param[in] userFlags The user flags to supply to the privilege script.
 * @param[in] systemFlags The system flags to supply to the privilege script.
 * @param[in,out] ctx The context of the invoking handler.
 * @param[in] isProlog Flag determines whether this executes the prolog or epilog.
 * @param[in] isStep Flag determines whether this should execute as a step or allocation.
 *
 * @return True: The Privileged script executed properly. False: The Privileged script failed to execute.
 */
inline bool ExecutePrivileged( 
    char *userFlags,
    char *systemFlags,
    csm::daemon::EventContextHandlerState_sptr& ctx,
    bool isProlog,
    bool isStep)
{
    LOG( csmapi, trace ) << "ExecutePrivileged: Enter";
    
    char * scriptArgs[]= { (char*)(isProlog ? CSM_P_PROLOG : CSM_P_EPILOG),
                            (char*)CSM_P_USER_FLAG  , userFlags,
                            (char*)CSM_P_SYSTEM_FLAG, systemFlags,
                            (char*)CSM_P_TYPE, (char*)(isStep ? CSM_P_STEP : CSM_P_ALLOCATION),
                            NULL };
    int errCode = 0;

    // TODO Check for prolog/epilog.
    if ( !isStep )
    {
        errCode = ScanForPrivleged( isProlog );
        
        if (errCode != 0)
        {
            LOG( csmapi, error ) << "Privileged script execution failed. Another script was running.";
            ctx->SetErrorCode( errCode );
            return false;
        }
    }

    errCode = ForkAndExec( scriptArgs );

    // Report any failure.
    if ( errCode== 255 )
    {
        LOG( csmapi, error ) << "Privileged script execution failed. Invalid allocation flags.";
        ctx->SetErrorCode( CSMERR_ALLOC_BAD_FLAGS );

        std::string err = "Privileged script execution failure detected. Invalid allocation flags.";
        ctx->SetErrorMessage( err );

        return false;
    }
    else if ( errCode != 0 )
    {
        LOG( csmapi, error ) << "Privileged script execution failed. Error Code: " << errCode;
        ctx->SetErrorCode( CSMERR_SCRIPT_FAILURE );

        std::string err = "Privileged script execution failure detected. Error code received: ";
        err.append( std::to_string( errCode ) );
        ctx->SetErrorMessage( err );
        return false;
    }

    LOG( csmapi, trace ) << "ExecutePrivileged: Exit";
    return true;
}



} // End namespace helpers
} // End namespace daemon
} // End namespace csm

#endif
