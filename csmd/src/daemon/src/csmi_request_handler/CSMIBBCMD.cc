/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBCMD.cc
    
    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIBBCMD.h"
#include "CSMIBBCMDAgentState.h"

#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"

#include "csmi/include/csm_api_workload_manager.h"
#include "include/csm_event_type_definitions.h"

#include "csmi_stateful_db/CSMIStatefulDBInit.h"

#define STATE_NAME "CSMIBBCMD"

// Use this to make changing struct names easier.
#define INPUT_STRUCT   csm_bb_cmd_input_t
#define MCAST_STRUCT   csmi_bb_cmd_context_t
#define OUTPUT_STRUCT  csm_bb_cmd_output_t
#define CMD_ID CSM_CMD_bb_cmd

#define MCAST_PROPS_PAYLOAD CSMIBBCMD

CSMIBBCMD_Master::CSMIBBCMD_Master(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CMD_ID, options, 0)
{
    SetInitialState(BB_CMD_INIT);
    ResizeStates(BB_CMD_FINAL);

    const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

    SetState( STATEFUL_DB_INIT,
        new StatefulDBInit(
            this,
            BB_CMD_MCAST_SPAWN,
            BB_CMD_FINAL,
            BB_CMD_FINAL,
            csm::daemon::helper::BAD_STATE,
            csm::daemon::helper::BAD_TIMEOUT_LEN,
            BB_CMD_MCAST_SPAWN)
    );
    
    SetState( BB_CMD_MCAST_SPAWN,
        new McastSpawner< MCAST_PROPS_PAYLOAD,
                          ParseAuthQuery,
                          BadQuery,
                          CreateByteArray >(
            BB_CMD_MCAST_RESPONSE,             // Success State 
            BB_CMD_FINAL,                      // Failure State
            BB_CMD_FINAL,                      // Final State
            csm::daemon::helper::BAD_STATE,    // Timeout State 
            MASTER_TIMEOUT));                  // Timeout Time

    SetState( BB_CMD_MCAST_RESPONSE,
        new McastResponder< MCAST_PROPS_PAYLOAD,
                            TerminalByte,
                            CreateByteArray,
                            BadQuery,
                            csm::mcast::bb::ParseCMDResponse,
                            false>(                                     
           BB_CMD_FINAL,               // Success State 
           BB_CMD_FINAL,               // Failure State
           BB_CMD_FINAL,               // Final State
           BB_CMD_MCAST_RESPONSE));           // Timeout State, hack to work around timeout state check.
}                           
                            
bool CSMIBBCMD_Master::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
    return  RetrieveDataForPrivateCheck( arguments, len, dbPayload, ctx);
}

bool CSMIBBCMD_Master::RetrieveDataForPrivateCheck(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx )
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
        // FIXME can this segfault?
        mcast->user_id              = msg._Msg.GetUserID();
        mcast->num_nodes            = input->node_names_count;
        mcast->command_arguments    = input->command_arguments;
        mcast->compute_nodes        = input->node_names;

        // Input struct is no longer relevant.
        input->command_arguments = nullptr;
        input->node_names        = nullptr;
        csm_free_struct_ptr( INPUT_STRUCT, input);

        MCAST_PROPS_PAYLOAD* mcast_props = new MCAST_PROPS_PAYLOAD( CMD_ID, mcast );
        ctx->SetDataDestructor( []( void* data ){ delete (MCAST_PROPS_PAYLOAD*)data;});
        ctx->SetUserData( mcast_props );
        
        // Generate the auth query.
        std::string stmt = "SELECT COUNT( DISTINCT an.node_name ) "
            "FROM csm_allocation as a "
            "LEFT JOIN csm_allocation_node as an "
            "ON a.allocation_id=an.allocation_id AND "
                "a.user_id=$1::integer AND an.node_name= ANY($2::text[])";

        const int paramCount = 2;
        *dbPayload = new csm::db::DBReqContent( stmt, paramCount );
        (*dbPayload)->AddNumericParam<uint32_t>(mcast->user_id);
        (*dbPayload)->AddTextArrayParam(mcast->compute_nodes, mcast->num_nodes); 
        success = true;
        
        LOG(csmapi,info) << ctx <<  mcast_props->GenerateIdentifierString()
            << "; Message: Verifying user has access to nodes in database";
    }

    return success;
}

bool CSMIBBCMD_Master::CompareDataForPrivateCheck(
    const std::vector<csm::db::DBTuple *>& tuples,
    const csm::network::Message &msg,
    csm::daemon::EventContextHandlerState_sptr& ctx)
{
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);
    
    return ParseAuthQuery(ctx, tuples, mcastProps);
}

bool CSMIBBCMD_Master::ParseAuthQuery( 
    csm::daemon::EventContextHandlerState_sptr& ctx,
    const std::vector<csm::db::DBTuple *>& tuples, 
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":ParseAuthQuery: Enter";
    MCAST_STRUCT* mcast_ctx = mcastProps->GetData();

    // EARLY RETURN 
    // Authorized uses get cart blanche.
    if ( ctx->GetHasPrivateAccess() )
    {
        LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
            << "; Message: User has authorization to execute burst buffer command unilaterally";
        return true;
    }

    
    // EARLY RETURN
    // First, verify that the tuple set has any usable data.
    if ( tuples.size() != 1 )
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Auth query was unable to execute";

        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        
        return false;
    }

    csm::db::DBTuple* fields = tuples[0];
    if (!fields  || fields->nfields != 1)
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Auth query returned unexpected values";

        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        
        return false;
    }

    // Test the node count.
    bool success = true;
    uint32_t node_match_count = strtoul(fields->data[0],nullptr, 10);
    if (mcast_ctx->num_nodes != node_match_count )
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Only " + std::to_string(node_match_count) + " of the " +
            std::to_string(mcast_ctx->num_nodes) + " specified nodes were usable by the user;";
        
        ctx->SetErrorCode(CSMERR_PERM);
        ctx->SetErrorMessage(error);
        success = false;
    }
    
    // Log the burst buffer information.
    LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
        << "; Message: User was successfully authorized to execute burst buffer command";

    LOG(csmapi,trace) <<  STATE_NAME ":ParseAuthQuery: Exit";

    return success;
}
    
bool CSMIBBCMD_Master::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    return CreateByteArray(buf, bufLen, ctx);
}

bool CSMIBBCMD_Master::CreateByteArray(
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr& ctx )
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
        if (mcastProps && mcastProps->GetData())
        {
            csm_bb_cmd_output_t* output;
            csm_init_struct_ptr(csm_bb_cmd_output_t, output);
            
            // TODO fix this
            output->command_output = strdup(mcastProps->GetData()->cmd_output.c_str());

            csm_serialize_struct( csm_bb_cmd_output_t, output, buf, &bufLen );
            csm_free_struct_ptr(csm_bb_cmd_output_t, output);

            LOG(csmapi,info) << ctx->GetCommandName() << ctx 
                << mcastProps->GenerateIdentifierString() 
                << "; Message: Burst Buffer Command completed;";
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

CSMIBBCMD_Agent::CSMIBBCMD_Agent( csm::daemon::HandlerOptions& options ) : 
    CSMIStateful( CMD_ID, options )
{
    // Set the start state for the machine.
    SetInitialState( 0 );

    // Add the states and their Transitions.
    ResizeStates( 1 );

    SetState( 0,
        new BBCMDAgentState(
            1,
            1,
            1));
}

