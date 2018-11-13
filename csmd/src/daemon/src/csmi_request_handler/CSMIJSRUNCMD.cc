/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIJSRUNCMD.cc
    
    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIJSRUNCMD.h"
#include "CSMIJSRUNCMDAgentState.h"

#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"

#include "csmi/include/csm_api_workload_manager.h"
#include "include/csm_event_type_definitions.h"

#include "csmi_stateful_db/CSMIStatefulDBInit.h"

#define STATE_NAME "CSMIJSRUNCMD"

// Use this to make changing struct names easier.
#define INPUT_STRUCT   csm_jsrun_cmd_input_t
#define MCAST_STRUCT   csmi_jsrun_cmd_context_t
#define OUTPUT_STRUCT  
#define CMD_ID CSM_CMD_jsrun_cmd

#define MCAST_PROPS_PAYLOAD CSMIJSRUNCMD

CSMIJSRUNCMD_Master::CSMIJSRUNCMD_Master(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CMD_ID, options, 0)
{
    SetInitialState(JSRUN_CMD_INIT);
    ResizeStates(JSRUN_CMD_FINAL);

    const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

    SetState( STATEFUL_DB_INIT,
        new StatefulDBInit(
            this,
            JSRUN_CMD_MCAST_SPAWN,
            JSRUN_CMD_FINAL,
            JSRUN_CMD_FINAL,
            csm::daemon::helper::BAD_STATE,
            csm::daemon::helper::BAD_TIMEOUT_LEN,
            JSRUN_CMD_MCAST_SPAWN)
    );
    
    SetState( JSRUN_CMD_MCAST_SPAWN,
        new McastSpawner< MCAST_PROPS_PAYLOAD,
                          ParseAuthQuery,
                          BadQuery,
                          CreateByteArray >(
            JSRUN_CMD_MCAST_RESPONSE,             // Success State 
            JSRUN_CMD_FINAL,                      // Failure State
            JSRUN_CMD_FINAL,                      // Final State
            csm::daemon::helper::BAD_STATE,    // Timeout State 
            MASTER_TIMEOUT));                  // Timeout Time

    SetState( JSRUN_CMD_MCAST_RESPONSE,
        new McastResponder< MCAST_PROPS_PAYLOAD,
                            TerminalByte,
                            CreateByteArray,
                            BadQuery,
                            csm::mcast::wm::ParseCMDResponse,
                            false>(                                     
           JSRUN_CMD_FINAL,               // Success State 
           JSRUN_CMD_FINAL,               // Failure State
           JSRUN_CMD_FINAL,               // Final State
           JSRUN_CMD_MCAST_RESPONSE));           // Timeout State, hack to work around timeout state check.
}                           
                            
bool CSMIJSRUNCMD_Master::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    return  RetrieveDataForPrivateCheck( arguments, len, dbPayload, ctx);
}

bool CSMIJSRUNCMD_Master::RetrieveDataForPrivateCheck(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":RetrieveDataForPrivateCheck: Enter";

    bool success = false;
    INPUT_STRUCT* input = nullptr;

    if( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        csm::network::MessageAndAddress msg = 
            dynamic_cast<const csm::daemon::NetworkEvent *>( ctx->GetReqEvent() )->GetContent();

        // Generate multicast context object.
        MCAST_STRUCT* mcast = new MCAST_STRUCT();
        mcast->user_id              = msg._Msg.GetUserID();
        mcast->allocation_id        = input->allocation_id;
        mcast->jsm_path             = input->jsm_path;
        input->jsm_path             = nullptr;
        mcast->kv_pairs             = input->kv_pairs;
        input->kv_pairs             = nullptr;

        csm_free_struct_ptr( INPUT_STRUCT, input);

        MCAST_PROPS_PAYLOAD* mcast_props = new MCAST_PROPS_PAYLOAD( CMD_ID, mcast );
        ctx->SetDataDestructor( []( void* data ){ delete (MCAST_PROPS_PAYLOAD*)data;});
        ctx->SetUserData( mcast_props );
        
        // Generate the auth query.
        std::string stmt = 
            "SELECT a.user_id, a.num_nodes, array_agg(an.node_name) "
                "FROM csm_allocation as a "
                "LEFT JOIN csm_allocation_node as an "
                "ON a.allocation_id=an.allocation_id "
                "WHERE a.allocation_id=$1::bigint "
                    "AND a.state='";
        stmt.append(csm_get_string_from_enum(csmi_state_t,CSM_RUNNING));
        stmt.append("' GROUP BY a.user_id, a.num_nodes");    

        const int paramCount = 1;
        *dbPayload = new csm::db::DBReqContent( stmt, paramCount );
        (*dbPayload)->AddNumericParam<int64_t>(mcast->allocation_id);
        success = true;
        
        LOG(csmapi,info) << ctx <<  mcast_props->GenerateIdentifierString()
            << "; Message: Verifying user has access to nodes in database";
    }

    return success;
}

bool CSMIJSRUNCMD_Master::CompareDataForPrivateCheck(
    const std::vector<csm::db::DBTuple *>& tuples,
    const csm::network::Message &msg,
    csm::daemon::EventContextHandlerState_sptr ctx)
{
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);
    
    return ParseAuthQuery(ctx, tuples, mcastProps);
}

bool CSMIJSRUNCMD_Master::ParseAuthQuery( 
    csm::daemon::EventContextHandlerState_sptr ctx,
    const std::vector<csm::db::DBTuple *>& tuples, 
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":ParseAuthQuery: Enter";
    MCAST_STRUCT* mcast_ctx = mcastProps->GetData();
    
    // EARLY RETURN
    // First, verify that the tuple set has any usable data.
    if ( tuples.size() != 1 )
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Auth query was unable to execute, invalid number of results (" +
            std::to_string(tuples.size()) + ")";

        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        
        return false;
    }

    bool success = true;
    csm::db::DBTuple* fields = tuples[0];

    if (fields  || fields->nfields == 3)
    {
        uint32_t user_id   = strtol(fields->data[0], nullptr, 10);
        uint32_t num_nodes =  strtol(fields->data[1], nullptr, 10);
        
        if ( num_nodes > 0 )
        {
            mcast_ctx->compute_nodes = (char **)malloc(sizeof(char *) * num_nodes);
            uint32_t i = 0;
            char *saveptr;
            char *nodeStr = strtok_r(fields->data[2], ",\"{}", &saveptr);
            
            while (nodeStr != NULL && i < num_nodes )
            {
                mcast_ctx->compute_nodes[i++] = strdup(nodeStr);
                nodeStr = strtok_r(NULL, ",\"{}", &saveptr);
            }
            mcast_ctx->num_nodes = i;
        }
        else
        {
            std::string error = mcastProps->GenerateIdentifierString() + 
                "; Message: No nodes were found for Allocation ID " + 
                std::to_string(mcast_ctx->allocation_id) + ";";
            ctx->SetErrorCode(CSMERR_DB_ERROR);
            ctx->SetErrorMessage(error);
            return false;
        }

        if ( user_id != mcast_ctx->user_id )
        {
            std::string error = mcastProps->GenerateIdentifierString() + 
                "; Message: User ID " + std::to_string(mcast_ctx->user_id) + " was not authorized "
                "for Allocation ID " + std::to_string(mcast_ctx->allocation_id) + ";";
            ctx->SetErrorCode(CSMERR_PERM);
            ctx->SetErrorMessage(error);
            success = false;
        }
        else
        {
            // Log the jsrun information.
            LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
                << "; Message: User was successfully authorized to execute jsrun command";
        }
    }
    else
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Auth query returned unexpected values";

        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        
        success= false;
    }

    LOG(csmapi,trace) <<  STATE_NAME ":ParseAuthQuery: Exit";

    return success;
}
    
bool CSMIJSRUNCMD_Master::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    return CreateByteArray(buf, bufLen, ctx);
}

bool CSMIJSRUNCMD_Master::CreateByteArray(
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Enter";

    bool success = ctx->GetErrorCode() == CSMI_SUCCESS;
    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    if (success)
    {
        *buf = nullptr;
        bufLen = 0;
        if (mcastProps )
        {
            LOG(csmapi,info) << ctx->GetCommandName() << ctx 
                << mcastProps->GenerateIdentifierString() 
                << "; Message: JSRUN Command completed;";
        }
    }
    else
    {
        if ( mcastProps )
        {
            ctx->PrependErrorMessage(mcastProps->GenerateIdentifierString(),';');
            ctx->SetNodeErrors(mcastProps->GenerateErrorListingVector());
            //ctx->AppendErrorMessage(mcastProps->GenerateErrorListing());
        }
    }

    dataLock.unlock();

    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Exit";

    return success;
}

CSMIJSRUNCMD_Agent::CSMIJSRUNCMD_Agent( csm::daemon::HandlerOptions& options ) : 
    CSMIStateful( CMD_ID, options )
{
    // Set the start state for the machine.
    SetInitialState( 0 );

    // Add the states and their Transitions.
    ResizeStates( 1 );

    SetState( 0,
        new JSRUNCMDAgentState(
            1,
            1,
            1));
}

