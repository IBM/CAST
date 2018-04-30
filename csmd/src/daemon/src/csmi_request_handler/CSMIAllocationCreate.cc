/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationCreate.cc
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIAllocationCreate.h"

#include "CSMIAllocationAgentUpdateState.h"
#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"
#include "csmi_mcast/CSMIMcastTerminal.h"

#include "csmi_stateful_db/CSMIStatefulDBRecvTerminal.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

#include "csmi/include/csm_api.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include "csmi/src/common/include/csmi_api_internal.h"
#include "include/csm_event_type_definitions.h"

#define STATE_NAME "CSMIAllocationCreate:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT   csm_allocation_create_input_t
#define MCAST_STRUCT   csmi_allocation_mcast_context_t
#define OUTPUT_STRUCT 
#define CMD_ID CSM_CMD_allocation_create

#define MCAST_PROPS_PAYLOAD CSMIMcastAllocation
#define EXTRA_STATES 6
#define SPAWN_STATE STATEFUL_DB_RECV_DB + 1

CSMIAllocationCreate_Master::CSMIAllocationCreate_Master(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CMD_ID, options, STATEFUL_DB_DONE + EXTRA_STATES)
{
    // State id for the multicast spawner.    
    const int RESERVE_NODES  = STATEFUL_DB_RECV_DB;             // 2 - Reserves nodes in the database.
    const int MCAST_SPAWN    = SPAWN_STATE;                     // 3 - Spawns a multicast message.
    const int MCAST_RESPONSE = STATEFUL_DB_RECV_DB + 2;         // 4 - Handles a multicast response, continuing on receiving all the events or a timeout.

    const int UNDO_MCAST_RESPONSE = STATEFUL_DB_RECV_DB + 3;    // 5 - Handles a recovery multicast responses.
    const int UNDO_INSERT    = STATEFUL_DB_RECV_DB + 4;         // 6 - Reverts the Allocation in the database. TERMINAL STATE

    const int UPDATE_STATS   = STATEFUL_DB_RECV_DB + 5;         // 7 - Updates The aggregated statistics.
    const int FINAL          = STATEFUL_DB_DONE + EXTRA_STATES + 1; // Beyond the final state of the state machine, a context placed in this state is done.
    
    const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

    LOG(csmapi,error) << STATE_NAME ":Timeout TIME "<<  MASTER_TIMEOUT;

    // State for reservation of the nodes.
    SetState( RESERVE_NODES, 
        new StatefulDBRecvSend<ReserveNodes>(
            MCAST_SPAWN,                    // Success State
            FINAL,                          // Failure State
            FINAL));                        // Final State


    // State for spawning the allocation.
    SetState( MCAST_SPAWN, 
        new McastSpawner<MCAST_PROPS_PAYLOAD,
                         PerformMulticast,
                         UndoAllocationDB,
                         CreateByteArray>(
            MCAST_RESPONSE,                 // Success State 
            UNDO_INSERT,                    // Failure State 
            FINAL,                          // Final State   
            csm::daemon::helper::BAD_STATE, // Timeout State 
            MASTER_TIMEOUT));               // Timeout Time  

    // State for processing the multicast.
    SetState( MCAST_RESPONSE,
        new McastResponder<MCAST_PROPS_PAYLOAD,
                           PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                           InsertStatsStatement,                        
                           UndoAllocationDB,                            
                           csm::mcast::allocation::ParseResponseCreate,
                           true>(                                      
           UPDATE_STATS,                    // Success State 
           UNDO_MCAST_RESPONSE,             // Failure State
           FINAL,                           // Final State
           UNDO_MCAST_RESPONSE,             // Timeout State
           MASTER_TIMEOUT));                // Timeout Time

    // State for processing the recovery multicast.
    SetState( UNDO_MCAST_RESPONSE,
        new McastResponder<MCAST_PROPS_PAYLOAD,
                           PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                           UndoAllocationDB,                          
                           UndoAllocationDB,                          
                           csm::mcast::allocation::ParseResponseRecover,
                           false>(                                     
           UNDO_INSERT,                     // Success State 
           UNDO_INSERT,                     // Failure State
           FINAL,                           // Final State
           UNDO_INSERT,                     // Timeout State
           MASTER_TIMEOUT));                // Timeout Time

    // Processes the Undo SQL Statement.
    SetState( UNDO_INSERT,
        new StatefulDBRecvTerminal<UndoTerminal>(
            FINAL,                          // Success State
            FINAL,                          // Failure State
            FINAL));                        // Final State

    // Processes the statistics update results.
    SetState( UPDATE_STATS,
        new McastTerminal< MCAST_PROPS_PAYLOAD, 
                           ParseStatsQuery,
                           CreateByteArray,
                           UndoAllocationDB >(
            FINAL,                          // Success State
            UNDO_MCAST_RESPONSE,            // Failure State
            FINAL,                          // Final State 
            UNDO_MCAST_RESPONSE,            // Timeout State
            MASTER_TIMEOUT));               // Timeout Time
}    

bool CSMIAllocationCreate_Master::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Enter"; 

    // Unpack the user data.
    INPUT_STRUCT* allocation = nullptr;

    if ( csm_deserialize_struct( INPUT_STRUCT, &allocation, arguments.c_str(), len ) == 0 )
    {
        // Copy the meaningful values. 
        // // TODO write something that copys the intersection of the structs?
        // ========================================================================================
        MCAST_STRUCT *mcastProps = nullptr; 
        csm_init_struct_ptr(MCAST_STRUCT, mcastProps);

        // Job ids.
        //mcastProps->allocation_id    = 0;
        mcastProps->primary_job_id   = allocation->primary_job_id;
        mcastProps->secondary_job_id = allocation->secondary_job_id;
        
        // Job details.
        mcastProps->isolated_cores   = allocation->isolated_cores;
        mcastProps->shared           = allocation->shared;
        mcastProps->user_flags       = allocation->user_flags;
        mcastProps->system_flags     = allocation->system_flags;  
        mcastProps->type             = allocation->type;
        mcastProps->state            = allocation->state;
        mcastProps->user_name        = allocation->user_name;

        // Node details.
       mcastProps->shared            = allocation->shared; 
       mcastProps->num_processors    = allocation->num_processors;
       mcastProps->num_gpus          = allocation->num_gpus; 
       mcastProps->projected_memory  = allocation->projected_memory;

        // Initialize arrays
        mcastProps->num_nodes        = allocation->num_nodes;
        mcastProps->compute_nodes    = allocation->compute_nodes;

        // Create shouldn't save config errors:
        mcastProps->save_allocation  = 0;

        // Null the allocation values cached to save the trouble of a strdup and free.
        allocation->user_name         = nullptr;
        allocation->user_flags        = nullptr;  
        allocation->system_flags      = nullptr;
        allocation->compute_nodes     = nullptr; 
        // ========================================================================================
        // Lambda expression for destructor.
        MCAST_PROPS_PAYLOAD* payload = new MCAST_PROPS_PAYLOAD( CMD_ID, mcastProps, true, true, 
            CSM_RAS_MSG_ID_ALLOCATION_TIMEOUT);
        ctx->SetDataDestructor( []( void* data ){ delete static_cast<MCAST_PROPS_PAYLOAD*>(data);});
        ctx->SetUserData( payload );

        // Compute what state the create should start in.
        csmi_state_t creating_state = allocation->state == CSM_RUNNING ? CSM_TO_RUNNING : allocation->state;
       
        // --------------------------------------------------------------------------
        // Build the insert allocation statement.
        // --------------------------------------------------------------------------
        std::string stmt = "INSERT INTO csm_allocation ("
                "allocation_id,        begin_time,           primary_job_id,  secondary_job_id,"
                "ssd_file_system_name, launch_node_name,     user_flags,      system_flags,"
                "ssd_min,              ssd_max,              num_nodes,       num_processors,"
                "num_gpus,             projected_memory,     state,           type,"
                "job_type,             user_name,            user_id,         user_group_id,"
                "user_script,          account,              comment,         job_name,"
                "job_submit_time,      queue,                requeue,         time_limit,"
                "wc_key,               isolated_cores"
            ") VALUES ("
                "default,        'now',        $1::bigint,   $2::integer,"
                "$3::text,       $4::text,     $5::text,     $6::text,"
                "$7::bigint,     $8::bigint,   $9::integer,  $10::integer,"  
                "$11::integer,   $12::integer, $13::text,    $14::text," 
                "$15::text,      $16::text,    $17::integer, $18::integer, "
                "$19::text,      $20::text,    $21::text,    $22::text,"
                "$23::timestamp, $24::text,    $25::text,    $26::bigint,"
                "$27::text,      $28::integer"
            ") returning allocation_id";

        const int paramCount = 28;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(allocation->primary_job_id);                // $1 - bigint
        dbReq->AddNumericParam<int32_t>(allocation->secondary_job_id);              // $2 - integer

        dbReq->AddTextParam(allocation->ssd_file_system_name);                      // $3 - text
        dbReq->AddTextParam(allocation->launch_node_name);                          // $4 - text
        dbReq->AddTextParam(mcastProps->user_flags);                                // $5 - text
        dbReq->AddTextParam(mcastProps->system_flags);                              // $6 - text 

        dbReq->AddNumericParam<int64_t>(allocation->ssd_min);                       // $7 - bigint
        dbReq->AddNumericParam<int64_t>(allocation->ssd_min);                       // $8 - bigint
        dbReq->AddNumericParam<int32_t>(allocation->num_nodes);                     // $9 - integer
        dbReq->AddNumericParam<int32_t>(allocation->num_processors);                // $10 - integer

        dbReq->AddNumericParam<int32_t>(allocation->num_gpus);                      // $11 - integer
        dbReq->AddNumericParam<int32_t>(allocation->projected_memory);              // $12 - integer 
        dbReq->AddTextParam(csm_get_string_from_enum(csmi_state_t, ( creating_state ))); // $13 - text
        dbReq->AddTextParam(csm_get_string_from_enum(csmi_allocation_type_t,mcastProps->type)); // $14 - text

        dbReq->AddTextParam(csm_get_string_from_enum(csmi_job_type_t,allocation->job_type));  // $15 - text
        dbReq->AddTextParam(mcastProps->user_name);                                       // $16 - text
        dbReq->AddNumericParam<int32_t>(allocation->user_id);                             // $17 - integer
        dbReq->AddNumericParam<int32_t>(allocation->user_group_id);                       // $18 - integer

        dbReq->AddTextParam(allocation->user_script);   // $19 - text
        dbReq->AddTextParam(allocation->account);       // $20 - text
        dbReq->AddTextParam(allocation->comment);       // $21 - text
        dbReq->AddTextParam(allocation->job_name);      // $22 - text

        dbReq->AddTextParam(allocation->job_submit_time); // $23 - text
        dbReq->AddTextParam(allocation->queue);           // $24 - text
        dbReq->AddTextParam(allocation->requeue);         // $25 - text
        dbReq->AddNumericParam<int64_t>(allocation->time_limit); // $26 - text
        
        dbReq->AddTextParam(allocation->wc_key); // $27 - text
        dbReq->AddNumericParam<int32_t>(allocation->isolated_cores); // $28 - text
        // --------------------------------------------------------------------------
        
        csm_free_struct_ptr(INPUT_STRUCT, allocation );

        *dbPayload = dbReq;
        
        // Log that the request is happening. 
        LOG(csmapi,info) << ctx << payload->GenerateIdentifierString() 
            << "; Message: Requesting allocation in database; ";
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";
       
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR );
        ctx->SetErrorMessage("; Message: Unable to build initial query, create failed.");
        return false;
    }

    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Exit"; 

    return true;
}

bool CSMIAllocationCreate_Master::ReserveNodes(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx)
{
    LOG(csmapi,trace) << STATE_NAME ":ReserveNodes: Enter";
    
    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    // Cache the allocation for readability.
    MCAST_STRUCT* allocation = mcastProps->GetData();

    if( allocation )
    {
        // EARLY RETURN
        if ( tuples.size() == 0 )
        {
            std::string error = mcastProps->GenerateIdentifierString() + 
                "; Message: Could not create the allocation in the database;";

            ctx->SetErrorCode(CSMERR_DB_ERROR);
            ctx->SetErrorMessage(error);
            return false;
        }

        // TODO Perform an arg check?
        allocation->allocation_id = strtoll( *tuples[0]->data, nullptr, 10 );
        //
        // --------------------------------------------------------------------------
        // INSERT into allocation nodes.
        // --------------------------------------------------------------------------
        std::string stmt = "SELECT fn_csm_allocation_node_sharing_status( "
            "$1::bigint, $2::text, $3::text, $4::boolean, $5::text[] )";

        const int paramCount = 5;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );

        dbReq->AddNumericParam<int64_t>(allocation->allocation_id);
        dbReq->AddTextParam(csm_get_string_from_enum(csmi_allocation_type_t,allocation->type));
        dbReq->AddTextParam(csm_get_string_from_enum(csmi_state_t, allocation->state));
        dbReq->AddCharacterParam(allocation->shared == CSM_TRUE); // TODO should this just be the contents?
        dbReq->AddTextArrayParam(allocation->compute_nodes, allocation->num_nodes);

        // --------------------------------------------------------------------------
        
        LOG(csmapi,info) << ctx <<  mcastProps->GenerateIdentifierString() 
            << "; Message: Reserving nodes in database; ";

        *dbPayload = dbReq;
    }
    else
    {
        // TODO might need more.
        std::string error = mcastProps->GenerateIdentifierString() + 
            "; Message: The cached allocation settings could not be found;";
        int errorCode     = CSMERR_INVALID_PARAM;
        
        // This is a larger error.
        if(tuples.size() == 0)
        {   
            error.append(" Allocation could not be created in the database;");
            errorCode = CSMERR_DB_ERROR;
        }

        ctx->SetErrorCode(errorCode);
        ctx->SetErrorMessage(error);
    }

    dataLock.unlock();

    LOG(csmapi,trace) << STATE_NAME ":ReserveNodes: Exit";
    return dbPayload != nullptr; // The status of the payload is all that matters.
}

csm::db::DBReqContent* CSMIAllocationCreate_Master::UndoAllocationDB(
    csm::daemon::EventContextHandlerState_sptr ctx,
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) << STATE_NAME ":UndoAllocationDB: Enter";
    csm::db::DBReqContent *dbReq = nullptr;
    std::string error = "";

    MCAST_STRUCT* allocation = mcastProps->GetData();
    if (allocation)
    {
        // The allocation should be reserved.
        bool reserve = allocation->save_allocation != 0;

        //error.append(mcastProps->GenerateIdentifierString())
        error.append("; Message: Allocation is being reverted;");

        if ( !reserve ) error.append(" Unable to reserve nodes;");

        std::string stmt = "";
        // If the allocation is reserved perform a dump.
        // Else simply delete.
        if ( reserve )
        {
            stmt = "SELECT fn_csm_allocation_history_dump( $1::bigint, 'now', -1, $2::text, 'f', "
                "'{}',  '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}' )";

            const int paramCount = 2;
            dbReq = new csm::db::DBReqContent( stmt, paramCount );
            dbReq->AddNumericParam<int64_t>(allocation->allocation_id);
            dbReq->AddTextParam(csm_get_string_from_enum(csmi_state_t, CSM_FAILED));
        }
        else
        {
            stmt = "SELECT fn_csm_allocation_revert($1::bigint)";
            const int paramCount = 1;
            dbReq = new csm::db::DBReqContent( stmt, paramCount );
            dbReq->AddNumericParam<int64_t>(allocation->allocation_id);
        }
    }
    else
    {
        error.append("; Message: Multicast context was lost, unable to give more verbose error;");
    }

    // Report a verbose error.
    ctx->AppendErrorMessage(error, ' ');

    LOG(csmapi,trace) << STATE_NAME ":UndoAllocationDB: Exit";
    return dbReq;
}

csm::db::DBReqContent* CSMIAllocationCreate_Master::InsertStatsStatement( 
    csm::daemon::EventContextHandlerState_sptr ctx,
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) << STATE_NAME ":InsertStatsStatement: Enter";
    csm::db::DBReqContent *dbReq = nullptr;
    MCAST_STRUCT *allocation = mcastProps->GetData();

    if ( allocation )
    {
        std::string stmt = "SELECT fn_csm_allocation_create_data_aggregator( "
            "$1::bigint, $2::text, $3::text[], $4::bigint[], "
            "$5::bigint[], $6::bigint[], $7::bigint[], " 
            "$8::bigint[], $9::int[], $10::int[], "
            "$11::int[], $12::bigint[])";

        const int paramCount = 12;
        dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(allocation->allocation_id);
        dbReq->AddTextParam(csm_get_string_from_enum(csmi_state_t, allocation->state)); 
        dbReq->AddTextArrayParam(allocation->compute_nodes, allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->ib_rx,      allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->ib_tx,      allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->gpfs_read,  allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->gpfs_write, allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->energy,     allocation->num_nodes);
        dbReq->AddNumericArrayParam<int32_t>(allocation->power_cap,  allocation->num_nodes);
        dbReq->AddNumericArrayParam<int32_t>(allocation->ps_ratio,   allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->power_cap_hit, allocation->num_nodes);
        dbReq->AddNumericArrayParam<int64_t>(allocation->gpu_energy, allocation->num_nodes);


        LOG(csmapi,info) << ctx <<  mcastProps->GenerateIdentifierString() 
            << "; Message: Recording Allocation statistics to database; ";
    }

    LOG(csmapi,trace) << STATE_NAME ":InsertStatsStatement: Exit";
    return dbReq;
}

bool CSMIAllocationCreate_Master::UndoTerminal( 
    const std::vector<csm::db::DBTuple *>& tuples,
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    if ( mcastProps )
    {
        ctx->PrependErrorMessage(mcastProps->GenerateIdentifierString(),';');
        ctx->AppendErrorMessage(mcastProps->GenerateErrorListing(), ' ');
        ctx->AppendErrorMessage("; Message: Allocation was successfully reverted;", ' ');
    }

    dataLock.unlock();

    return false;
}

bool CSMIAllocationCreate_Master::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    return CreateByteArray(buf, bufLen, ctx);
}

bool CSMIAllocationCreate_Master::CreateByteArray(
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Enter";

    bool success = true;

    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    if( mcastProps )
    {
        MCAST_STRUCT* allocation = mcastProps->GetData();
        if( allocation ) 
        {
            csm_primative_serializer(allocation->allocation_id, *buf, bufLen);

            LOG(csmapi,info) << ctx->GetCommandName() << ctx <<
                mcastProps->GenerateIdentifierString() << "; Message: Create Completed; ";
        }
        else
            success = false;
    }
    else
    {
        success = false;
    }

    dataLock.unlock();

    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Exit";
    return success;
}

CSMIAllocationCreate_Agent::CSMIAllocationCreate_Agent( csm::daemon::HandlerOptions& options ) : 
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

