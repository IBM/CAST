/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationDelete.cc
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIAllocationDelete.h"

#include "CSMIAllocationAgentUpdateState.h"
#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"

#include "csmi/include/csm_api.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include "include/csm_event_type_definitions.h"
#include "csmi/src/wm/include/csmi_wm_internal.h"
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_json.h"

#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

#define STATE_NAME "CSMIAllocationDelete"

// Use this to make changing struct names easier.
#define INPUT_STRUCT   csm_allocation_delete_input_t
#define MCAST_STRUCT   csmi_allocation_mcast_context_t
#define OUTPUT_STRUCT 
#define CMD_ID CSM_CMD_allocation_delete

#define MCAST_PROPS_PAYLOAD CSMIMcastAllocation
#define EXTRA_STATES 2
CSMIAllocationDelete_Master::CSMIAllocationDelete_Master(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CMD_ID, options, STATEFUL_DB_DONE + EXTRA_STATES)
{
    // State id for the multicast spawner.    
    const int MCAST_SPAWN    = STATEFUL_DB_RECV_DB;     // 2
    const int MCAST_RESPONSE = STATEFUL_DB_RECV_DB + 1; // 3
    const int REMOVE_ROWS    = STATEFUL_DB_RECV_DB + 2; // 4  NOTE: This is recv_db

    const int FINAL          = STATEFUL_DB_DONE + EXTRA_STATES;

    const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

    SetState( MCAST_SPAWN, 
        new McastSpawner< MCAST_PROPS_PAYLOAD,
                          ParseInfoQuery,
                          DeleteRowStatement,
                          CreateByteArray >(
            MCAST_RESPONSE,                    // Success State 
            REMOVE_ROWS,                       // Failure State
            FINAL,                             // Final State
            csm::daemon::helper::BAD_STATE,    // Timeout State 
            MASTER_TIMEOUT));                  // Timeout Time

    SetState( MCAST_RESPONSE,
        new McastResponder< MCAST_PROPS_PAYLOAD,
                            PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                            DeleteRowStatement,                         // Called when all responses are successful.                            
                            DeleteRowStatement,                         // Called for failure.                                                 
                            csm::mcast::allocation::ParseResponseDelete,// Performs a parse of the responses.                                  
                           false>(                                      // Specifies that a second multicast shouldn't be attempted in a failure. 
           REMOVE_ROWS,               // Success State 
           REMOVE_ROWS,               // Failure State
           FINAL,                     // Final State
           REMOVE_ROWS,               // Timeout State
           MASTER_TIMEOUT));          // Timeout Time
}                           

bool CSMIAllocationDelete_Master::RetrieveDataForPrivateCheck(
        const std::string& arguments, 
        const uint32_t len, 
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx )
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
        else if( input->primary_job_id > 0 )
        {
            std::string error = "Primary Job ID: " +  std::to_string(input->primary_job_id);
            ctx->SetErrorMessage("Primary Job ID: " +  std::to_string(input->allocation_id));
            paramStmt.append("primary_job_id = $1::bigint ");
            paramCount++;
        
            // Secondary job_id can be 0.
            if ( input->secondary_job_id >= 0 )
            {
                error.append("Secondary Job ID: ").append(std::to_string(input->secondary_job_id));
                paramStmt.append("AND secondary_job_id = $").append(
                    std::to_string(++paramCount)).append("::integer ");
            }
            else
            {
                error.append("Secondary Job ID: 0");
                paramStmt.append("AND secondary_job_id=0 ");
            }

            ctx->SetErrorMessage(error + ";");
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
        if ( input->allocation_id > 0 )
            dbReq->AddNumericParam<int64_t>(input->allocation_id);
        else
        {
            dbReq->AddNumericParam<int64_t>(input->primary_job_id);
        
            if ( input->secondary_job_id >= 0 )
                dbReq->AddNumericParam<int32_t>(input->secondary_job_id);
        }
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

bool CSMIAllocationDelete_Master::CompareDataForPrivateCheck(
        const std::vector<csm::db::DBTuple *>& tuples,
        const csm::network::Message &msg,
        csm::daemon::EventContextHandlerState_sptr ctx)
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
            error.append(std::to_string(msg.GetUserID())).append(" does not have permission to delete;");

            ctx->AppendErrorMessage(error);
        }
    }
    else
    {
        ctx->SetErrorCode(CSMERR_ALLOC_MISSING);
        ctx->AppendErrorMessage("Database check failed for user id " + std::to_string(msg.GetUserID()) + ";" );
    }

    LOG( csmapi, trace ) << STATE_NAME ":CompareDataForPrivateCheck: Exit";
	return success;
}
                            
bool CSMIAllocationDelete_Master::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Enter"; 

    // Unpack the user data.
    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct( INPUT_STRUCT, &input, arguments.c_str(), len ) == 0 )
    {
        // Allocation id clears primary jobid.
        if ( input->allocation_id > 0 )
            input->primary_job_id = 0;

        // Make sure the secondary_job_id is clamped.
        if ( input->secondary_job_id < 0 )
            input->secondary_job_id = 0;

        const int paramCount = 4;
        std::string paramStmt = "SELECT * FROM fn_csm_allocation_delete_start( "
            "$1::bigint, $2::bigint, $3::integer, $4::integer)";

        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( paramStmt, paramCount );
        dbReq->AddNumericParam<int64_t>(input->allocation_id);
        dbReq->AddNumericParam<int64_t>(input->primary_job_id);
        dbReq->AddNumericParam<int32_t>(input->secondary_job_id);
        dbReq->AddNumericParam<int32_t>(csm_get_timeout(CMD_ID) / 1000 ); // Convert to seconds.
        *dbPayload = dbReq;
        
        // Set the context object for later use.
        MCAST_STRUCT* mcast = nullptr;
        csm_init_struct_ptr(MCAST_STRUCT, mcast);
        mcast->allocation_id = input->allocation_id;
        mcast->primary_job_id = input->primary_job_id;
        mcast->secondary_job_id = input->secondary_job_id;

        MCAST_PROPS_PAYLOAD* payload = new MCAST_PROPS_PAYLOAD( CMD_ID, mcast, false, false, 
            CSM_RAS_MSG_ID_ALLOCATION_TIMEOUT);
        ctx->SetDataDestructor( []( void* data ){ delete (MCAST_PROPS_PAYLOAD*)data;});
        ctx->SetUserData( payload );
        
        LOG(csmapi,info) << ctx << payload->GenerateIdentifierString()
            << "; Message: Requesting details about Allocation;";

        // Free the input struct.
        csm_free_struct_ptr(INPUT_STRUCT, input);
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";
       
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR );
        ctx->SetErrorMessage("; Message: Unable to build initial query, delete failed.");
        return false;
    }

    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Exit"; 

    return true;
}

bool CSMIAllocationDelete_Master::ParseInfoQuery( 
    csm::daemon::EventContextHandlerState_sptr ctx,
    const std::vector<csm::db::DBTuple *>& tuples, 
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":ParseInfoQuery: Enter";
    MCAST_STRUCT* a = mcastProps->GetData();
    
    // EARLY RETURN
    // First, verify that the tuple set has any usable data.
    if ( tuples.size() != 1 )
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Allocation";

        if ( tuples.size() == 0)
            error.append(" was not present in the database, exiting;");
        else
            error.append(" too many results were found, exiting;");

        a->primary_job_id = -1;
        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        
        return false;
    }

    csm::db::DBTuple* fields = tuples[0];
    
    // EARLY RETURN
    // If the number of fields is invalid exit.
    if ( fields->nfields != 11 )
    {
        std::string error = mcastProps->GenerateIdentifierString() +
            "; Message: Allocation query returned an incorrect number of fields;";

        a->primary_job_id = -1;
        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        
        return false;
    }

    // TODO remove some of the extras here.
    // Then extract the allocation struct from the database result.
    a->allocation_id        = strtoll(fields->data[0], nullptr, 10);
    a->primary_job_id       = strtoll(fields->data[1], nullptr, 10);
    a->secondary_job_id     = strtol(fields->data[2], nullptr, 10);
    
    a->user_flags           = strdup(fields->data[3]);
    a->system_flags         = strdup(fields->data[4]);
    a->num_nodes            = strtol(fields->data[5], nullptr, 10);

    a->state                = (csmi_state_t)csm_get_enum_from_string(csmi_state_t, fields->data[6]);
    a->type                 = (csmi_allocation_type_t)csm_get_enum_from_string(csmi_allocation_type_t, fields->data[7]);
    a->isolated_cores       = strtol(fields->data[8], nullptr, 10);
    a->user_name            = strdup(fields->data[9]);

    a->compute_nodes        = nullptr;

    if (a->num_nodes > 0) 
    {
        a->compute_nodes = (char **)malloc(sizeof(char *) * a->num_nodes);
        uint32_t i = 0;
        char* saveptr;
        char* nodeStr = strtok_r(fields->data[10], ",", &saveptr);

        while (nodeStr != NULL && i < a->num_nodes)
        {
            a->compute_nodes[i++] = strdup(nodeStr);
            nodeStr = strtok_r(NULL, ",", &saveptr);
        }

        // Make sure the compute nodes are nulled out.
        while( i< a->num_nodes)
        {
            a->compute_nodes[i++] = nullptr;
        }
    }    

    // Log the Allocation information.
    LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
        << "; Message: Successfully retrieved information for Allocation, performing multicast;";

    LOG(csmapi,trace) <<  STATE_NAME ":ParseInfoQuery: Exit";
    // DELETING will always MCAST to be sure, because typically a collision occurs when deleting from running.
    // Return true if the node needs to multicast (diag or not running).
    return a->type == CSM_DIAGNOSTICS || 
        (a->state >= CSM_TO_RUNNING     &&  a->state <= CSM_TO_STAGING_OUT ) || 
        (a->state >= CSM_DELETING_MCAST &&  a->state <= CSM_STAGING_OUT_FAILED );
}
    
csm::db::DBReqContent* CSMIAllocationDelete_Master::DeleteRowStatement( 
    csm::daemon::EventContextHandlerState_sptr ctx,
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":DeleteRowStatement: Enter";

    csm::db::DBReqContent *dbReq = nullptr;

    MCAST_STRUCT* allocation = mcastProps->GetData();

    // Early return if a transitory state was found.

    #define INVALID_STATE 1
    #define INVALID_ALLOCATION 2
    switch ( ctx->GetDBErrorCode() )
    {
        case INVALID_STATE:
            ctx->SetErrorCode(CSMERR_DELETE_STATE_BAD);
            break;
        case INVALID_ALLOCATION: 
            ctx->SetErrorCode(CSMERR_ALLOC_MISSING);
            break;
        default:
            break;
    }
    

    if (allocation && allocation->primary_job_id > 0)
    {
        const int paramCount = 13;
        std::string stmt = "SELECT fn_csm_allocation_history_dump( "
            "$1::bigint,'now','0',$2::text,'t',$3::text[],$4::bigint[],"
            "$5::bigint[],$6::bigint[],$7::bigint[],$8::bigint[],"
            "$9::bigint[],$10::bigint[],$11::bigint[],$12::bigint[],$13::bigint[]);";

        dbReq = new csm::db::DBReqContent( stmt, paramCount );

        csmi_state_t error_code = (csmi_cmd_err_t) ctx->GetErrorCode() == CSMI_SUCCESS ? 
            CSM_COMPLETE : CSM_FAILED;
        
        dbReq->AddNumericParam<int64_t>(allocation->allocation_id);
        dbReq->AddTextParam(csm_get_string_from_enum(csmi_state_t, error_code));
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

        // TODO Replace this with mechanism to send data to BDS via file beats
        LOG(csmapi,info) << "NAB2: allocation->gpu_metrics=" << allocation->gpu_metrics;
        if ( allocation->gpu_metrics )
        {
            for ( uint32_t nodeIdx = 0; nodeIdx < allocation->num_nodes; nodeIdx++ )
            {
                LOG(csmapi,info) << "NAB2: allocation->gpu_metrics[nodeIdx]=" << allocation->gpu_metrics[nodeIdx];
                for ( uint32_t gpuIdx = 0; allocation->gpu_metrics[nodeIdx] && gpuIdx < allocation->gpu_metrics[nodeIdx]->num_gpus; gpuIdx++ )
                {
                    LOG(csmapi,info) << "NAB2: InsertStatsStatement() gpu_metrics: node=" << allocation->compute_nodes[nodeIdx]
                                     << " gpu_id=" << allocation->gpu_metrics[nodeIdx]->gpu_id[gpuIdx]
                                     << " gpu_usage=" << allocation->gpu_metrics[nodeIdx]->gpu_usage[gpuIdx]
                                     << " max_gpu_memory=" << allocation->gpu_metrics[nodeIdx]->max_gpu_memory[gpuIdx];
                }
            }
        }
        // End TODO

        LOG(csmapi,info) << ctx <<  mcastProps->GenerateIdentifierString()
            << "; Message: Recording Allocation statistics and removing allocation from database;";

    }

    LOG(csmapi,trace) <<  STATE_NAME ":DeleteRowStatement: Exit";
    return dbReq;
}

bool CSMIAllocationDelete_Master::CreateResponsePayload(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx)
{
    LOG(csmapi,trace) <<  STATE_NAME ":CreateResponsePayload: Enter";
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    *dbPayload = DeleteRowStatement(ctx, mcastProps);
    dataLock.unlock();

    LOG(csmapi,trace) <<  STATE_NAME ":CreateResponsePayload: Exit";
    return *dbPayload != nullptr;
}

bool CSMIAllocationDelete_Master::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{

    if( tuples.size() > 0 && tuples[0]->data && tuples[0]->nfields > 0)
    {
        MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock =
            ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);
        
        MCAST_STRUCT *allocation = mcastProps->GetData();

        // Do the transaction/TRANSACTION write.
        if (allocation )
        {
            std::string end_time_str(tuples[0]->data[0]);
            std::string state_str    = csm_get_string_from_enum(csmi_state_t,
                ctx->GetErrorCode() == CSMI_SUCCESS ? CSM_COMPLETE : CSM_FAILED );

            std::string json = "";
            json.append("{\"state\":\"").append(state_str).append("\",\"history\":{\"end_time\":\"")
                .append(end_time_str).append("\"}");
                
            if ( allocation->state == CSM_RUNNING )
            {
                // TODO make this cleaner.
                json.append(",\"running-end-timestamp\":\"")
                    .append(end_time_str).append("\"");
            }

            json.append("}");
                
            TRANSACTION("allocation", ctx->GetRunID(), allocation->allocation_id, json);
        }
        
        dataLock.unlock();
    }

    return CreateByteArray(buf, bufLen, ctx);
}

bool CSMIAllocationDelete_Master::CreateByteArray(
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
        bufLen= 0;
        if ( mcastProps )
            LOG(csmapi,info) << ctx->GetCommandName() << ctx 
                << mcastProps->GenerateIdentifierString() 
                << "; Message: Delete completed;";
    }
    else
    {
        if ( mcastProps )
        {
            ctx->PrependErrorMessage(mcastProps->GenerateIdentifierString(),';');
            ctx->AppendErrorMessage(mcastProps->GenerateErrorListing(), ' ');
        }

        ctx->AppendErrorMessage("; Message: The allocation was removed from the database;", ' ');
    }

    dataLock.unlock();

    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Exit";

    return success;
}

CSMIAllocationDelete_Agent::CSMIAllocationDelete_Agent( csm::daemon::HandlerOptions& options ) : 
    CSMIStateful( CMD_ID, options )
{
    // Set the start state for the machine.
    SetInitialState( 0 );

    // Add the states and their Transitions.
    ResizeStates( 1 );

    SetState( 0,
        new AllocationAgentUpdateState(
            1,
            1,
            1));
}

