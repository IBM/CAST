/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepBegin.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: John Dunham, Nick Buonarota
* Email: jdunham@us.ibm.com, nbuonar@us.ibm.com
*/

#include "CSMIAllocationStepBegin.h"
#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvTerminal.h"
#include "csmi_stateful_forward/CSMIStatefulForwardResponse.h"
#include "CSMIStepUpdateAgent.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include "csmi/src/wm/include/csmi_wm_internal.h"
#include "csmi/src/common/include/csmi_json.h"

#define STATE_NAME "CSMIAllocationStepBegin:"
#define CMD_ID CSM_CMD_allocation_step_begin

#define MCAST_PROPS_PAYLOAD CSMIStepMcast
#define EXTRA_STATES 4

#define DATA_STRING "allocation_id,step_id,num_nodes,compute_nodes,num_processors,num_gpus,projected_memory,"\
    "num_tasks,status,executable,working_directory,user_flags,argument,environment_variable,begin_time}"

CSMIAllocationStepBegin::CSMIAllocationStepBegin( csm::daemon::HandlerOptions& options ):
        CSMIStatefulDB( CMD_ID, options, STATEFUL_DB_DONE + EXTRA_STATES )
{
    const int MCAST_SPAWN     = STATEFUL_DB_RECV_DB;     // 2
    const int MCAST_RESP      = STATEFUL_DB_RECV_DB + 1; // 3
    const int UNDO_MCAST_RESP = STATEFUL_DB_RECV_DB + 2; // 4
    const int REVERT_STEP     = STATEFUL_DB_RECV_DB + 3; // 5
    
    const int FINAL       =  STATEFUL_DB_DONE + EXTRA_STATES;

    const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

    // Add the states and their Transitions.
    SetState( MCAST_SPAWN,
        new McastSpawner<MCAST_PROPS_PAYLOAD, 
                            ParseInfoQuery, 
                            BadQuery,
                            CreateByteArray>(
            MCAST_RESP,                     // Success State
            REVERT_STEP,                    // Failure State
            FINAL,                          // Final State
            csm::daemon::helper::BAD_STATE, // Timeout State
            MASTER_TIMEOUT));               // Timeout Time

    SetState( MCAST_RESP,
        new McastResponder< MCAST_PROPS_PAYLOAD,
                            TerminalByte,
                            CreateByteArray,   
                            UndoStepDB,
                            csm::mcast::step::ParseResponse,
                            true>(
            MCAST_RESP,                     // Success State
            UNDO_MCAST_RESP,                // Failure State
            FINAL,                          // Final State
            UNDO_MCAST_RESP,                // Timeout State
            MASTER_TIMEOUT));               // Timeout Time

    SetState( UNDO_MCAST_RESP,
        new McastResponder< MCAST_PROPS_PAYLOAD,
                            PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                            UndoStepDB,
                            UndoStepDB,
                            csm::mcast::step::ParseResponse,
                            false>(
            REVERT_STEP,                    // Success State
            REVERT_STEP,                    // Failure State
            FINAL,                          // Final State
            REVERT_STEP,                    // Timeout State
            MASTER_TIMEOUT));               // Timeout Time

    SetState( REVERT_STEP,
        new StatefulDBRecvTerminal<UndoTerminal>(
            FINAL,                          // Success State
            FINAL,                          // Failure State
            FINAL));                        // Final State
}

bool CSMIAllocationStepBegin::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

	csmi_allocation_step_t* step = nullptr;

	if( csm_deserialize_struct( csmi_allocation_step_t, &step, arguments.c_str(), len ) == 0 )
	{
        // Cache the useful data from the payload.
        csmi_allocation_step_mcast_context_t* mcastContext = nullptr;
        csm_init_struct_ptr(csmi_allocation_step_mcast_context_t, mcastContext);

        // Build the payload object that's preserved across states.
        mcastContext->step_id       = step->step_id;
        mcastContext->allocation_id = step->allocation_id;
        mcastContext->num_nodes     = step->num_nodes;

        // TODO profile this against strdup?
        // @note I alias here to reduce the number of mallocs to keep the non multicast case fast.
        mcastContext->compute_nodes = step->compute_nodes; 
        mcastContext->user_flags    = step->user_flags;
        
        // Generate the baseline step json transaction.
        std::string json = "";
        csmiGenerateJSON(json, DATA_STRING, step, CSM_STRUCT_MAP(csmi_allocation_step_t));
        mcastContext->json_str = strdup(json.c_str());

        MCAST_PROPS_PAYLOAD *payload = new MCAST_PROPS_PAYLOAD( CMD_ID, mcastContext, true, true );
        ctx->SetDataDestructor( []( void* data ){ delete (MCAST_PROPS_PAYLOAD*)data;});
        ctx->SetUserData( payload );

        // ------------------------------------------------------------------------------

        std::string stmt = "SELECT * FROM fn_csm_step_begin( $1::bigint, $2::bigint, "
            "$3::character, $4::text, $5::text, $6::text, $7::text, "
            "$8::integer, $9::integer, $10::integer, $11::integer, $12::integer, "
            "$13::text, $14::text[] )";

        const int paramCount = 14;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( step->step_id );        // $1 bigint
        dbReq->AddNumericParam<int64_t>( step->allocation_id );  // $2 bigint 

        dbReq->AddTextParam( csm_get_string_from_enum(csmi_step_status_t, step->status )); // $3 text
        dbReq->AddTextParam( step->executable );                 // $4 text
        dbReq->AddTextParam( step->working_directory );          // $5 text 
        dbReq->AddTextParam( step->argument );                   // $6 text
        dbReq->AddTextParam( step->environment_variable );       // $7 text
        
        dbReq->AddNumericParam<int32_t>( step->num_nodes );       // $8  integer 
        dbReq->AddNumericParam<int32_t>( step->num_processors );  // $9  integer
        dbReq->AddNumericParam<int32_t>( step->num_gpus );        // $10 integer
        dbReq->AddNumericParam<int32_t>( step->projected_memory );// $11 integer
        dbReq->AddNumericParam<int32_t>( step->num_tasks );       // $12 integer
                                                                      
        dbReq->AddTextParam( step->user_flags );                  // $13 text
        dbReq->AddTextArrayParam( step->compute_nodes, step->num_nodes );   // $14 text[]

        *dbPayload = dbReq;
        // ------------------------------------------------------------------------------
        

        // These have been aliased above so we don't allocate excess memory.
        // Prevent them from being freed.
        step->compute_nodes         = nullptr;
        step->user_flags            = nullptr;

        LOG(csmapi,info) << ctx << payload->GenerateIdentifierString()
            << "; Message: Step begin in database; ";

        csm_free_struct_ptr(csmi_allocation_step_t, step); // Free the struct, as the meaningful data was extracted.

        LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
	}
    else
    {
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";

        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage(" Message: Unable to build the query, struct could not be deserialized");
        return false;
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

	return true;
}

bool CSMIAllocationStepBegin::ParseInfoQuery(
    csm::daemon::EventContextHandlerState_sptr ctx,
    const std::vector<csm::db::DBTuple *>& tuples,
    CSMIStepMcast* mcastProps )
    {
        LOG( csmapi, trace ) << STATE_NAME ":ParseInfoQuery: Enter";
        bool success = false;
        
        // Get the timestamp from the 
        std::string timestamp = "";
        if(tuples.size() == 1 && tuples[0]->data && tuples[0]->nfields == 1 && tuples[0]->data[0])
        {
            timestamp = tuples[0]->data[0];
        }

        // If the user flags are null then multicasting is not needed.
        if( mcastProps )
        {
            csmi_allocation_step_mcast_context_t* step = mcastProps->GetData();
            success = step && ( step->user_flags[0] != 0 );

            if ( step )
            {
                TRANSACTION("allocation-step", ctx->GetRunID(), 
                    "\""<< step->allocation_id << "-" << step->step_id << "\"", step->json_str);
                TRANSACTION("allocation-step", ctx->GetRunID(), 
                    "\"" << step->allocation_id << "-" << step->step_id << "\"", 
                    "{\"begin_time\":\"" << timestamp << "\",\"history\":{\"end_time\":\"\"}}" );

                if ( success ) 
                {
                    LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString() <<
                        "; User Flags: \"" << step->user_flags << 
                        " \"; Message: Step begin multicast;";

                }
                else 
                {
                    LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString() <<
                        "; User Flags: \"" << step->user_flags << 
                        " \"; Message: Skipping step begin multicast (no user flags);";
                }
            }
            else
            {
                std::string error = " Message: step begin lost context information;";
                ctx->SetErrorCode(CSMERR_GENERIC);
                ctx->SetErrorMessage(error);
            }
        }
        else
        {
            std::string error = " Message: step begin lost context information;";
            ctx->SetErrorCode(CSMERR_GENERIC);
            ctx->SetErrorMessage(error);
        }
        
        LOG( csmapi, trace ) << STATE_NAME ":ParseInfoQuery: Exit";

        return success;
    }

csm::db::DBReqContent* CSMIAllocationStepBegin::UndoStepDB(
    csm::daemon::EventContextHandlerState_sptr ctx,
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) << STATE_NAME ":UndoStepDB: Enter";

    csm::db::DBReqContent *dbReq = nullptr;
    std::string error = "";

    csmi_allocation_step_mcast_context_t* step = mcastProps->GetData();
    
    if ( step )
    {
        error.append(mcastProps->GenerateIdentifierString())
            .append("; Message: Step is being reverted;");

        std::string stmt = "SELECT * FROM fn_csm_step_end("
            "$1::bigint, $2::bigint, '', $3::int, $4::text,"
            "'', '', 0.0, 0.0, '',"
            "'', '', '', '', '' )";
        const int paramCount = 4;
        dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(step->step_id);        // $1::bigint - Step ID
        dbReq->AddNumericParam<int64_t>(step->allocation_id);  // $2::bigint - Allocation ID
        dbReq->AddNumericParam<int32_t>(ctx->GetErrorCode());  // $3::int - Error Code
        dbReq->AddTextParam(ctx->GetErrorMessage().c_str());   // $4::text - Error Message
    }
    else
    {
        error.append(" Message: Multicast context was lost in revert, unable to give more verbose error;");
    }

    ctx->AppendErrorMessage(error, ' ');

    LOG(csmapi,trace) << STATE_NAME ":UndoStepDB: Exit";
    return dbReq;
}

// will return csmi defined error code
bool CSMIAllocationStepBegin::UndoTerminal(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);
    
    std::string error = "";

    
    if ( mcastProps )
    {
        ctx->PrependErrorMessage(mcastProps->GenerateIdentifierString(), ';');
        ctx->AppendErrorMessage(mcastProps->GenerateErrorListing(), ' ');
        ctx->AppendErrorMessage("; Message: Step was successfully reverted;", ' ');

        csmi_allocation_step_mcast_context_t* step = mcastProps->GetData();
        if(step && tuples.size() == 1 && tuples[0]->data && 
            tuples[0]->nfields == 4 && tuples[0]->data[3])
        {
            std::string timestamp(tuples[0]->data[3]);

            TRANSACTION("allocation-step", ctx->GetRunID(), 
                "\"" << step->allocation_id << "-" << step->step_id << "\"", 
                "{\"history\":{\"end_time\":\"" << timestamp << "\"}}" );
        }
    }
    else
    {
        error.append(" Message : Multicast context was lost, unable to give more verbose error;");
    }

    ctx->AppendErrorMessage(error, ' ');

    dataLock.unlock();

    return false;
}

bool CSMIAllocationStepBegin::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    return CreateByteArray(buf, bufLen, ctx);
}

// TODO Inline?
bool CSMIAllocationStepBegin::CreateByteArray(
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf = nullptr;
    bufLen = 0;

    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    LOG(csmapi, info) << ctx->GetCommandName() << ctx << 
        mcastProps->GenerateIdentifierString() << 
        "; Message: Step Begin completed;";
    dataLock.unlock();
    
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

CSMIAllocationStepBegin_Agent::CSMIAllocationStepBegin_Agent( 
    csm::daemon::HandlerOptions& options ):
        CSMIStateful( CMD_ID, options )
{
    const int AGENT_INIT         = 0; // Multicast and request forwarder.
    const int AGENT_FORWARD_RESP = 1; // Forwards message to the client.
    const int AGENT_FINAL        = 2; // The terminal state.
    
    // Set the start state for the machine.
    SetInitialState( AGENT_INIT );

    // Add the states and their Transitions.
    ResizeStates( AGENT_FINAL );

    // Forwarder (if invoked on compute node)/primary state
    SetState( AGENT_INIT, 
        new StepAgentUpdateState(
            AGENT_FINAL,
            AGENT_FINAL,
            AGENT_FINAL,
            csm::daemon::helper::BAD_STATE,
            csm::daemon::helper::BAD_TIMEOUT_LEN,
            AGENT_FORWARD_RESP));
    
    // Forwarder responder.
    SetState( AGENT_FORWARD_RESP,
        new StatefulForwardResponse(
            AGENT_FINAL,
            AGENT_FINAL,
            AGENT_FINAL));
}

