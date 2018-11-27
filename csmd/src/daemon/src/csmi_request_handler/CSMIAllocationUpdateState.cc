/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationUpdateState.cc
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIAllocationUpdateState.h"
#include "CSMIAllocationCreate.h"
#include "CSMIAllocationDelete.h"

#include "csmi/include/csm_api.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include "include/csm_event_type_definitions.h"

#include "csmi_stateful_db/CSMIStatefulDBRecvTerminal.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

#include "CSMIAllocationAgentUpdateState.h"
#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"
#include "csmi_mcast/CSMIMcastTerminal.h"

#define STATE_NAME "CSMIAllocationUpdateState:"
// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_update_state_input_t
#define MCAST_STRUCT   csmi_allocation_mcast_context_t
#define OUTPUT_STRUCT 

#define CMD_ID CSM_CMD_allocation_update_state

#define MCAST_PROPS_PAYLOAD CSMIMcastAllocation
#define EXTRA_STATES 7 

    const int NUM_SPAWN_PAYLOAD_FIELDS=13;

    CSMIAllocationUpdateState::CSMIAllocationUpdateState(csm::daemon::HandlerOptions& options) :
        CSMIStatefulDB(CMD_ID, options, STATEFUL_DB_DONE + EXTRA_STATES)
    {
        ///< State wherein the handler spawns a multicast event.
        const int MCAST_SPAWN   = STATEFUL_DB_RECV_DB;              // 2 Creates the multicast message.

        ///< A State for processing a staging in->running multicast.
        const int MCAST_RESPONSE_CRE = STATEFUL_DB_RECV_DB + 1;     // 3
        //< State which reverts the Multicast.      
        const int UNDO_MCAST_RES_CRE = STATEFUL_DB_RECV_DB + 2;     // 4
        ///< State which removes the allocation.
        const int REMOVE_ALLOCATION  = STATEFUL_DB_RECV_DB + 3;     // 5 Terminal point

        ///< A State for processing a running->staging out multicast.
        const int MCAST_RESPONSE_DEL = STATEFUL_DB_RECV_DB + 4;     // 6

        ///< State which processes a successful multicast response. Same state as delete.
        const int TERMINAL_STATE     = STATEFUL_DB_RECV_DB + 5;     // 7 NOTE: This is recv_db 
        const int UPDATE_STATS_CRE   = STATEFUL_DB_RECV_DB + 6;     // 8 Terminal point.
        const int UPDATE_STATS_DEL   = STATEFUL_DB_RECV_DB + 7;     // 9 NOTE: This is recv_db

        const int FINAL          = STATEFUL_DB_DONE + EXTRA_STATES;
        const int TO_FINAL       = STATEFUL_DB_DONE + EXTRA_STATES + 1; // 10 NOTE: This is for a timeout.
        
        const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

        // Primary path : Staging In -> Running
        // Alternate Path : Running -> Staging Out
        // Path determinate : If the current state != to next state and is a valid transition. 
        SetState( MCAST_SPAWN,
            new McastSpawner< 
                MCAST_PROPS_PAYLOAD,
                ParseInfoQuery, 
                NULLDBResp,
                CreateByteArray >(
            MCAST_RESPONSE_CRE,             // Success State 
            TERMINAL_STATE,                 // Failure State 
            FINAL,                          // Final State   
            csm::daemon::helper::BAD_STATE, // Timeout State 
            MASTER_TIMEOUT,                 // Timeout Time  
            MCAST_RESPONSE_DEL));           // Alternate State
        
        // ===============================================================
        // Create States
        // ===============================================================
        // XXX This might "smell bad", but the point is to use the existing create functionality, where possible.
        //
        SetState( MCAST_RESPONSE_CRE,
            new McastResponder< 
                MCAST_PROPS_PAYLOAD,
                PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                CSMIAllocationCreate_Master::InsertStatsStatement, // Called when all responses are successful.
                MCASTDBReqSpawn,                                   // Called for failure.
                csm::mcast::allocation::ParseResponseCreate,       // Performs a parse of the responses.
                false >(                                            // Specifies that a second multicast should be attempted in a failure. 
            UPDATE_STATS_CRE,           // Success State 
            TERMINAL_STATE,             // Failure State
            FINAL,                      // Final State
            TERMINAL_STATE,             // Timeout State
            MASTER_TIMEOUT));           // Timeout Time
        
        // Processes the statistics update results.
        SetState( UPDATE_STATS_CRE,
            new McastTerminal< 
                MCAST_PROPS_PAYLOAD, 
                CSMIAllocationCreate_Master::ParseStatsQuery,
                CreateByteArray,
                CSMIAllocationCreate_Master::UndoAllocationDB >(
            FINAL,                          // Success State
            UNDO_MCAST_RES_CRE,             // Failure State
            FINAL,                          // Final State 
            UNDO_MCAST_RES_CRE,             // Timeout State
            MASTER_TIMEOUT));               // Timeout Time

        // Failure Path - Uses the CSMIAllocationCreate_Master behavior.
        // --------------------------------------
        // Revert the Allocations on the individual nodes.
        SetState ( UNDO_MCAST_RES_CRE,
            new McastResponder< MCAST_PROPS_PAYLOAD,
                                PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                                CSMIAllocationCreate_Master::UndoAllocationDB, // Called when all responses are successful.                               
                                CSMIAllocationCreate_Master::UndoAllocationDB, // Called for failure.                                                 
                                csm::mcast::allocation::ParseResponseRecover,  // Performs a parse of the responses.                                  
                                false >(                                       // Specifies that a second multicast shouldn't be attempted in a failure. 
                REMOVE_ALLOCATION,          // Success State  
                REMOVE_ALLOCATION,          // Failure State  
                FINAL,                      // Final State    
                REMOVE_ALLOCATION,          // Timeout State  
                MASTER_TIMEOUT));           // Timeout Time

        // TODO Should this be a custom solution? 
        // Process the Removal of the allocation.
        SetState ( REMOVE_ALLOCATION,
            new StatefulDBRecvTerminal<CSMIAllocationCreate_Master::UndoTerminal>(
                FINAL,                      // Success State  
                FINAL,                      // Failure State
                FINAL ));                   // Final State
        // --------------------------------------
        

        // ===============================================================
        // Delete States
        // ===============================================================
        // TODO Should timeouts do something special?
        SetState( MCAST_RESPONSE_DEL,
            new McastResponder< 
                MCAST_PROPS_PAYLOAD,
                PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                CSMIAllocationUpdateState::InsertStatsStatement, // Called when all responses are successful. 
                MCASTDBReqSpawn,                                 // Called for failure.
                csm::mcast::allocation::ParseResponseDelete,     // Performs a parse of the responses. 
                false >(                                         // Specifies that a second multicast shouldn't be attempted in a failure. 
            UPDATE_STATS_DEL,    // Success State 
            TERMINAL_STATE,      // Failure State
            FINAL,               // Final State  
        TO_FINAL,            // Timeout State
        MASTER_TIMEOUT));    // Timeout Time

    SetState ( TERMINAL_STATE,
        new StatefulDBRecvTerminal<CSMIAllocationUpdateState::UpdateTerminal>(
            FINAL,                      // Success State  
            FINAL,                      // Failure State
            FINAL ));                   // Final State
}

bool CSMIAllocationUpdateState::RetrieveDataForPrivateCheck(
        const std::string& arguments, 
        const uint32_t len, 
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":RetrieveDataForPrivateCheck: Enter";
	
	// Unpack the buffer.
	INPUT_STRUCT* input = nullptr;

    if( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        int paramCount = 0;
        std::string paramStmt = "SELECT user_id "
                "FROM csm_allocation "
                "WHERE ";

        // Determine how to set the allocation id.
        if ( input->allocation_id > 0 )
        {
            ctx->SetErrorMessage("Allocation ID: " +  std::to_string(input->allocation_id) + ";");
            paramStmt.append("allocation_id= $1::bigint ");
            paramCount++;
        }

        if ( paramCount == 0 )
        {
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Incomplete values supplied";
            ctx->SetErrorCode(CSMERR_MISSING_PARAM);
            ctx->SetErrorMessage("Unable to build query, verify that an allocation id greater than 0 or a primary job id greater than zero has not been supplied");
            csm_free_struct_ptr(INPUT_STRUCT, input);
        
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
            return false;
        }

        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( paramStmt, paramCount );
        dbReq->AddNumericParam<int64_t>(input->allocation_id);
        *dbPayload = dbReq;

        csm_free_struct_ptr(INPUT_STRUCT, input);
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":RetrieveDataForPrivateCheck: Deserialization failed";
        LOG( csmapi, trace  ) << STATE_NAME ":RetrieveDataForPrivateCheck: Exit";
        
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the private check query, "
            "struct could not be deserialized");
        return false;
    }

    LOG( csmapi, trace ) << STATE_NAME ":RetrieveDataForPrivateCheck: Exit";

    return true;
}

bool CSMIAllocationUpdateState::CompareDataForPrivateCheck(
        const std::vector<csm::db::DBTuple *>& tuples,
        const csm::network::Message &msg,
        csm::daemon::EventContextHandlerState_sptr& ctx)
{
    LOG( csmapi, trace ) << STATE_NAME ":CompareDataForPrivateCheck: Enter";
    bool success = false;

    if ( tuples.size() == 1 )
    {
		uint32_t userID = strtoul(tuples[0]->data[0], nullptr, 10);
        success = (userID == msg.GetUserID());

        // If the success failed report it.
        if (success)
        {
            ctx->SetErrorMessage("");
        }
        else
        {
            std::string error = "Allocation is owned by user id ";
            error.append(std::to_string(userID)).append(", user id ");
            error.append(std::to_string(msg.GetUserID())).append(" does not have permission to updare the state;");

            ctx->AppendErrorMessage(error);
        }
    }
    else
    {
        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->AppendErrorMessage("Database check failed for user id " + std::to_string(msg.GetUserID()) + ";" );
    }

    LOG( csmapi, trace ) << STATE_NAME ":CompareDataForPrivateCheck: Exit";
	return success;
}

bool CSMIAllocationUpdateState::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Enter"; 

    bool success = true;

    // Unpack the buffer.
    INPUT_STRUCT *stateArgs = nullptr;

    if ( csm_deserialize_struct( INPUT_STRUCT, &stateArgs, arguments.c_str(), len ) == 0 )
    {
        // TODO This might be excessive?
        if( stateArgs->new_state >= CSM_RUNNING || stateArgs->new_state <= CSM_STAGING_OUT )
        {
            MCAST_STRUCT* allocation = nullptr;
            csm_init_struct_ptr(MCAST_STRUCT, allocation);
            allocation->allocation_id = stateArgs->allocation_id;
            allocation->state = stateArgs->new_state;

            // Create a mcast property object and set _EEReply to true (meaning early exit just exits).
            MCAST_PROPS_PAYLOAD* payload = new MCAST_PROPS_PAYLOAD( 
                CMD_ID, allocation, stateArgs->new_state == CSM_RUNNING, true, 
                CSM_RAS_MSG_ID_ALLOCATION_TIMEOUT );
            ctx->SetDataDestructor( []( void* data ){ delete (MCAST_PROPS_PAYLOAD*)data;});
            ctx->SetUserData( payload );

            // Build the stmt.
            // XXX * is used for convenience, if the function changes this needs to changed.
            std::string stmt = "SELECT * FROM fn_csm_allocation_update_state($1::bigint, $2::text)";
            
            // Build the request.
            const int paramCount = 2;
            csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );

            dbReq->AddNumericParam<int64_t>( allocation->allocation_id );
            dbReq->AddTextParam( csm_get_string_from_enum(csmi_state_t, (allocation->state - 1) ) );
            *dbPayload = dbReq;


            LOG(csmapi,info) << payload->GenerateIdentifierString()
                << "; Message: Requesting information about the Allocation;";
        }
        else
        {
            std::string error = "; Message: Invalid state supplied: "  + 
                std::to_string(stateArgs->new_state);

            ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR );
            ctx->SetErrorMessage(error);
            success=false;
        }

        csm_free_struct_ptr( csm_allocation_update_state_input_t, stateArgs);
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
       
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR );
        ctx->SetErrorMessage("; Message: Unable to build the query, struct could not be deserialized");
        success=false;
    }

    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Exit"; 
    return success;
}

csm::db::DBReqContent* CSMIAllocationUpdateState::InsertStatsStatement(
    csm::daemon::EventContextHandlerState_sptr& ctx,
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":InsertStatsStatement: Enter";

    csm::db::DBReqContent *dbReq = nullptr;
    MCAST_STRUCT *allocation = mcastProps->GetData();

    if ( allocation )
    {
        std::string stmt = "SELECT * FROM fn_csm_allocation_finish_data_stats( $1::bigint, $2::text, $3::text[],"
            "$4::bigint[],$5::bigint[],$6::bigint[],$7::bigint[],$8::bigint[],$9::bigint[], $10::bigint[],"
            "$11::bigint[], $12::bigint[], $13::bigint[])";
    
        const int paramCount = 13;
        dbReq = new csm::db::DBReqContent( stmt, paramCount );

        dbReq->AddNumericParam<int64_t>( allocation->allocation_id );
        dbReq->AddTextParam( csm_get_string_from_enum(csmi_state_t, allocation->state ) );
        dbReq->AddTextArrayParam(allocation->compute_nodes, allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->ib_rx,      allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->ib_tx,      allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->gpfs_read,  allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->gpfs_write, allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->energy,     allocation->num_nodes);

        dbReq->AddNumericArrayParam<int64_t>(allocation->power_cap_hit, allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->gpu_usage,     allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->cpu_usage,     allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->memory_max,    allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->gpu_energy,    allocation->num_nodes);


        LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
            << "; Message: Recording Allocation statistics to the database;";
    }

    LOG(csmapi,trace) <<  STATE_NAME ":InsertStatsStatement: Exit";
    return dbReq;
}

bool CSMIAllocationUpdateState::ParseInfoQuery(
    csm::daemon::EventContextHandlerState_sptr& ctx,
    const std::vector<csm::db::DBTuple *>& tuples,
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":ParseInfoQuery: Enter";

    MCAST_STRUCT* a = mcastProps->GetData();

    // EARLY RETURN
    // If the tuple is empty, then the update failed, exit.
    // IT IS NOT EXPECTED TO REACH THIS CASE!
    if ( tuples.size() != 1 )
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Allocation ";
    
        if ( tuples.size() == 0)
            error.append(" was not present in the database, exiting.");
        else
            error.append(" too many results were found, exiting.");
    
        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        return false;
    }


    csm::db::DBTuple* fields = tuples[0];
    bool success  = true;

    // If the number of fields is invalid exit.
    if( fields->nfields == NUM_SPAWN_PAYLOAD_FIELDS )
    {
        a->primary_job_id       = strtoll(fields->data[0], nullptr, 10);
        a->secondary_job_id     = strtol(fields->data[1], nullptr, 10);
        a->user_flags           = strdup(fields->data[2]);

        a->system_flags         = strdup(fields->data[3]);
        a->num_nodes            = strtol(fields->data[4], nullptr, 10);
        a->isolated_cores       = strtol(fields->data[6], nullptr, 10);
        a->user_name            = strdup(fields->data[7]);
        a->shared               = csm_convert_psql_bool(fields->data[8][0]);
        a->num_gpus             = strtol(fields->data[9], nullptr, 10);
        a->num_processors       = strtol(fields->data[10], nullptr, 10);
        a->projected_memory     = strtol(fields->data[11], nullptr, 10);

        
        a->start_state = (csmi_state_t)csm_get_enum_from_string(csmi_state_t, fields->data[12]);
        if ( a->start_state == CSM_STAGING_IN && a->state == CSM_STAGING_OUT )
        {
            success=false;
            ctx->SetErrorCode(CSM_STATE_JUMPED);
            ctx->SetErrorMessage("State jumped, skipping multicast.");
            
            std::string state =  csm_get_string_from_enum(csmi_state_t, (a->state ) );
            std::string json  = "{\"state\":\"";
            json.append(state).append("\"}");

            TRANSACTION("allocation", ctx->GetRunID(), a->allocation_id, json);
        }
        else if ( a->start_state == a->state )
        {
            success=false;
            ctx->SetErrorCode(CSM_SAME_STATE_TRANSITION);
            ctx->SetErrorMessage("The state was unchanged, exiting early.");
        }

        if ( success && a->num_nodes > 0 )
        {
            a->compute_nodes = (char **)malloc(sizeof(char *) * a->num_nodes);
            uint32_t i = 0;
            char* saveptr;
            char* nodeStr = strtok_r(fields->data[5], ",", &saveptr);

            while (nodeStr != NULL && i < a->num_nodes)
            {
                a->compute_nodes[i++] = strdup(nodeStr);
                nodeStr = strtok_r(NULL, ",", &saveptr);
            }

            // Null out the compute nodes.
            while ( i < a->num_nodes )
            {
                a->compute_nodes[i++] = nullptr;
            }
        }

        LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
            << "; Message: Retrieved Allocation details, performing multicast;";
    }
    else
    {
        std::string error = mcastProps->GenerateIdentifierString();
        error.append("; Message: Allocation query returned an incorrect number of fields");
        error.append("; Expected: ").append(std::to_string(NUM_SPAWN_PAYLOAD_FIELDS));
        error.append("; Received: ").append( std::to_string(fields->nfields));

        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        success = false;
    }
     
    LOG(csmapi,trace) <<  STATE_NAME ":ParseInfoQuery: Exit";

    // If the allocation is designed for delete, set the alternate route.
    mcastProps->SetAlternate(!mcastProps->IsCreate());
    return success;

}

bool CSMIAllocationUpdateState::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    return CSMIAllocationUpdateState::UpdateTerminal( tuples, buf, bufLen, ctx );
}

bool CSMIAllocationUpdateState::CreateByteArray(
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Enter";

    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    bool success = ctx->GetErrorCode() == CSMI_SUCCESS;
    if (success)
    {
        *buf = nullptr;
        bufLen= 0;
        if ( mcastProps )
        {
            MCAST_STRUCT* allocation = mcastProps->GetData();
            if ( allocation ) 
            {
                std::string state =  csm_get_string_from_enum(csmi_state_t, (allocation->state ) );
                std::string time_str(allocation->timestamp ? allocation->timestamp : "");

                std::string json  = "{\"state\":\"";
                json.append(state);//.append("\"}");

                if ( allocation->timestamp &&
                        (allocation->state == CSM_RUNNING 
                            || allocation->start_state == CSM_RUNNING )) 
                {
                    std::string run_state = allocation->state == CSM_RUNNING ? "start" : "end";
                    json.append("\",\"running-").append(run_state)
                        .append("-timestamp\":\"").append(time_str).append("\"}");
                }
                else
                {
                    json.append("\"}");
                }

                TRANSACTION("allocation", ctx->GetRunID(), allocation->allocation_id, json);
            }

            LOG(csmapi, info) << ctx->GetCommandName() << ctx 
                << mcastProps->GenerateIdentifierString() 
                << "; Message: Update completed;";
        }
    }
    else
    {
        if ( mcastProps )
        {
            ctx->PrependErrorMessage(mcastProps->GenerateIdentifierString(),';');
            //ctx->AppendErrorMessage(mcastProps->GenerateErrorListing(), ' ');
            ctx->SetNodeErrors(mcastProps->GenerateErrorListingVector());
        }
    }

    dataLock.unlock();

    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Exit";
    return success;
}

bool CSMIAllocationUpdateState::UpdateTerminal(
    const std::vector<csm::db::DBTuple *>& tuples,
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);
    
    if ( mcastProps )
    {
        MCAST_STRUCT* allocation = mcastProps->GetData();
        if ( allocation && tuples.size() == 1 && tuples[0]->data && tuples[0]->nfields > 0) 
        {
            if( allocation->timestamp != nullptr ) 
            { 
                free(allocation->timestamp); 
                allocation->timestamp = nullptr;
            }
            allocation->timestamp = strdup(tuples[0]->data[0]);

            // If the end time and state is returned, process it.
            // Otherwise just update the state.
            if ( tuples[0]->nfields == 2 )
            {
                allocation->state = 
                    (csmi_state_t)csm_get_enum_from_string(csmi_state_t,tuples[0]->data[1]);
            }
        }
    }

    dataLock.unlock();

    return CreateByteArray( buf, bufLen, ctx );
}

csm::db::DBReqContent* CSMIAllocationUpdateState::MCASTDBReqSpawn(
    csm::daemon::EventContextHandlerState_sptr& ctx,
    CSMIMcastAllocation* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":MCASTDBReqSpawn: Enter";
    
    // Setup the request.
    csm::db::DBReqContent* dbReq = nullptr;

    // Only continue if the state change was not a real error case.
    int errCode = ctx->GetErrorCode();
    if ( !(errCode == CSM_STATE_JUMPED || errCode == CSM_SAME_STATE_TRANSITION) )
    {
        // Set the error code so the caller knows something was bad.
        ctx->SetErrorCode(CSMERR_STATE_CHANGE_FAILED);

        // If the allocation was set and not in  one of the existing failed states transition to a failure state.
        MCAST_STRUCT* allocation = mcastProps->GetData();
        if (allocation && 
               !( allocation->state == CSM_RUNNING_FAILED || 
                    allocation->state == CSM_STAGING_OUT_FAILED ) )
        {
            std::string stmt = "UPDATE csm_allocation "
                "SET state = $1::text " 
                "WHERE allocation_id = $2::bigint";
            
            csmi_state_t failure_state = allocation->state == CSM_RUNNING ? 
                CSM_RUNNING_FAILED : CSM_STAGING_OUT_FAILED;
            
            const int paramCount = 2;
            dbReq = new csm::db::DBReqContent( stmt, paramCount );
            dbReq->AddTextParam( csm_get_string_from_enum(csmi_state_t, failure_state ) );
            dbReq->AddNumericParam<int64_t>( allocation->allocation_id );


            LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
                << "; Message: Setting the allocation " << allocation->allocation_id << " to state: "
                << (csm_get_string_from_enum(csmi_state_t, failure_state));
        }
    }

    LOG(csmapi,trace) <<  STATE_NAME ":MCASTDBReqSpawn: Exit";
    return dbReq;
}

CSMIAllocationUpdateState_Agent::CSMIAllocationUpdateState_Agent( 
    csm::daemon::HandlerOptions& options ) : 
        CSMIStateful( CMD_ID, options )
{
    // Set the start state for the machine.
    SetInitialState( 0 );

    // Add the states and their Transitions.
    ResizeStates( 1 );

    SetState( 0, new AllocationAgentUpdateState( 1, 1, 1));
}


