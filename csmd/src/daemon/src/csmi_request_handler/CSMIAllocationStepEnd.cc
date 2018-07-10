/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepEnd.cc

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

#include "CSMIAllocationStepEnd.h"
#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"
#include "csmi_stateful_forward/CSMIStatefulForwardResponse.h"
#include "CSMIStepUpdateAgent.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include "csmi/src/wm/include/csmi_wm_internal.h"
#include "csmi/src/common/include/csmi_json.h"

#define STATE_NAME "CSMIAllocationStepEnd:"
#define CMD_ID CSM_CMD_allocation_step_end

#define MCAST_PROPS_PAYLOAD CSMIStepMcast
#define EXTRA_STATES 2

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_step_end_input_t
#define MCAST_STRUCT csmi_allocation_step_mcast_context_t
#define OUPUT_STRUCT 

#define DATA_STRING "status,history{exit_status,error_message,cpu_stats,total_u_time,total_s_time,omp_thread_limit"\
    "gpu_stats,memory_stats,max_memory,io_stats}"

const int NUM_STEP_END_FIELDS=4;

CSMIAllocationStepEnd::CSMIAllocationStepEnd( csm::daemon::HandlerOptions& options ):
        CSMIStatefulDB( CMD_ID, options, STATEFUL_DB_DONE + EXTRA_STATES )
{
    const int MCAST_SPAWN     = STATEFUL_DB_RECV_DB;     // 2
    const int MCAST_RESP      = STATEFUL_DB_RECV_DB + 1; // 3
    
    const int FINAL       =  STATEFUL_DB_DONE + EXTRA_STATES;

    const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

    SetState( MCAST_SPAWN,
        new McastSpawner<MCAST_PROPS_PAYLOAD,
                            ParseInfoQuery,
                            BadQuery,
                            CreateByteArray>(
            MCAST_RESP,
            FINAL,
            FINAL,
            csm::daemon::helper::BAD_STATE,
            MASTER_TIMEOUT ));

    SetState( MCAST_RESP,
        new McastResponder<MCAST_PROPS_PAYLOAD,
                            TerminalByte,
                            CreateByteArray,
                            BadQuery,
                            csm::mcast::step::ParseResponse,
                            false>(
            FINAL,
            FINAL,
            FINAL));
}

bool CSMIAllocationStepEnd::RetrieveDataForPrivateCheck(   
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
        const int paramCount = 1;
        std::string paramStmt = "SELECT user_id "
                "FROM csm_allocation "
                "WHERE allocation_id= $1::bigint";

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

    return true;
}        

bool CSMIAllocationStepEnd::CompareDataForPrivateCheck(
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
        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->AppendErrorMessage("Database check failed for user id " + std::to_string(msg.GetUserID()    ) + ";" );
    }

    LOG( csmapi, trace ) << STATE_NAME ":CompareDataForPrivateCheck: Exit";
    return success;
}

bool CSMIAllocationStepEnd::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct( INPUT_STRUCT, &input, arguments.c_str(), len ) == 0  &&
            input->history)
    {
        // Cache the useful data from the payload.
        MCAST_STRUCT* mcastContext = nullptr;
        csm_init_struct_ptr(MCAST_STRUCT, mcastContext);

        // Build the payload object that's preserved across states.
        mcastContext->step_id       = input->step_id;
        mcastContext->allocation_id = input->allocation_id;

        printf("%p\n",input->history);
        // Generate the baseline step json transaction.
        std::string json = "";
        csmiGenerateJSON(json, DATA_STRING, input, CSM_STRUCT_MAP(csm_allocation_step_end_input_t));
        mcastContext->json_str = strdup(json.c_str());
            
        MCAST_PROPS_PAYLOAD *payload = new MCAST_PROPS_PAYLOAD( CMD_ID, mcastContext, false, true );
        ctx->SetDataDestructor( []( void* data ){ delete (MCAST_PROPS_PAYLOAD*)data;});
        ctx->SetUserData( payload ); // Begin and early exit.
        
        // ------------------------------------------------------------------------------

        std::string stmt = "SELECT * FROM fn_csm_step_end( $1::bigint, $2::bigint, $3::int, "
            "$4::text, $5::text, $6::double precision, $7::double precision, "
            "$8::text, $9::text, $10::text, $11::bigint, $12::text )";

        const int paramCount = 12;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( input->step_id );       // $1 bigint
        dbReq->AddNumericParam<int64_t>( input->allocation_id ); // $2 bigint
        
        // Cache history for readability.
        csmi_allocation_step_history_t *history = input->history;
        dbReq->AddNumericParam<int32_t>( history->exit_status );// $3 integer
        dbReq->AddTextParam( history->error_message );               // $4 text   
        dbReq->AddTextParam( history->cpu_stats );              // $5 text
        dbReq->AddNumericParam<double>( history->total_u_time );// $6 double.
        dbReq->AddNumericParam<double>( history->total_s_time );// $7 double.

        dbReq->AddTextParam( history->omp_thread_limit );       // $8 text
        dbReq->AddTextParam( history->gpu_stats );              // $9 text
        dbReq->AddTextParam( history->memory_stats );           // $10 text
        dbReq->AddNumericParam<int64_t>( history->max_memory ); // $11 bigint 
        dbReq->AddTextParam( history->io_stats );               // $12 text

        *dbPayload = dbReq;
        
        // ------------------------------------------------------------------------------

        LOG(csmapi,info) << ctx << payload->GenerateIdentifierString()
            << "; Message: Step end in database";

        LOG(csmapi,trace) << ctx << "; step_id: " << input->step_id << " allocation_id: " <<
            input->allocation_id << " exit_status: '" << history->exit_status << "' error_message: '"   << 
            history->error_message <<  "' cpu_stats: '" << history->cpu_stats << "' total_u_time: "     << 
            history->total_u_time << " total_s_time" << history->total_s_time << " omp_thread_limit: '" << 
            history->omp_thread_limit << "' gpu_stats: '" << history->gpu_stats << "' memory_stats: '"  <<
            history->memory_stats << "' max_memory: " << history->max_memory << " io_stats: '" <<
            history->io_stats << "'";
        csm_free_struct_ptr( INPUT_STRUCT, input );

        LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";
        
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the query, struct could not be deserialized");
        return false;
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMIAllocationStepEnd::ParseInfoQuery(
    csm::daemon::EventContextHandlerState_sptr ctx,
    const std::vector<csm::db::DBTuple *>& tuples,
    CSMIStepMcast* mcastProps)
{
    LOG( csmapi, trace ) << STATE_NAME ":ParseInfoQuery: Enter";

    // EARLY RETURN
    // If the tuple is empty, then the update failed, exit.
    // IT IS NOT EXPECTED TO REACH THIS CASE!
    if ( tuples.size() != 1 )
    {
        std::string error = "; Message: ";

        if ( tuples.size() == 0)
            error.append("Didn't find any results, exiting.");
        else
            error.append("Found too many results, exiting.");

        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
        return false;
    }

    csm::db::DBTuple* fields = tuples[0];
    bool success = false;

    // If the correct number of fields are present parse the results.
    if( fields->nfields == NUM_STEP_END_FIELDS )
    {
        MCAST_STRUCT* step = mcastProps ? mcastProps->GetData() : nullptr;

        if( step )
        {
            /// The step is assumed to have not been populated before this.
            step->user_flags = strdup(fields->data[0]);
            int32_t node_count = strtol(fields->data[1], nullptr, 10);

            std::string timestamp = fields->data[3];
            TRANSACTION("allocation-step", ctx->GetRunID(),
                "\""<< step->allocation_id << "-" << step->step_id << "\"", step->json_str);
            TRANSACTION("allocation-step", ctx->GetRunID(),
                "\"" << step->allocation_id << "-" << step->step_id << "\"",
                "{\"history\":{\"end_time\":\"" << timestamp << "\"}}" );


            // TODO should this be a function?
            if ( node_count > 0 )
            {
                step->num_nodes = (uint32_t) node_count;
                step->compute_nodes = (char**)malloc( sizeof(char*) * step->num_nodes);

                uint32_t i = 0;
                char *saveptr;
                char *nodeStr = strtok_r(fields->data[2], ",", &saveptr);

                while (nodeStr != NULL && i < step->num_nodes)
                {
                    step->compute_nodes[i++] = strdup(nodeStr);
                    nodeStr = strtok_r(NULL, ",", &saveptr);
                }

                // EARLY RETURN
                // If the nodes are mismatched this is a failure.
                success = (i == step->num_nodes);
                if (!success)
                {
                    std::string error =  mcastProps->GenerateIdentifierString();
                    error.append("; Message: Node count mismatch detected, can't build multicast;");

                    ctx->SetErrorCode(CSMERR_DB_ERROR);
                    ctx->SetErrorMessage(error);

                    while ( i < step->num_nodes )
                        step->compute_nodes[i++] = nullptr;

                    LOG( csmapi, trace ) << STATE_NAME ":ParseInfoQuery: Exit";
                    return false;
                }
            }
            
            // This means the userflags were set and there were enough user flags.
            success &= step->user_flags[0] != 0;

            if ( success )
            {
                LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString() <<
                    "; User Flags: \"" << step->user_flags << 
                    "\"; Message: Beginning step end multicast (no user flags);";
            }
            else if ( step )
            {
                LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString() <<
                    "; User Flags: \"" << step->user_flags << 
                    "\"; Message: Skipping step end multicast (no user flags);";
            }
            else
            {
                std::string error = "; Message: step end lost step context information;";
                ctx->SetErrorCode(CSMERR_GENERIC);
                ctx->SetErrorMessage(error);
            }

        }
        else
        {
            std::string error = "; Message: step end lost context information;";
            ctx->SetErrorCode(CSMERR_GENERIC);
            ctx->SetErrorMessage(error);
        }
    }
    else
    {
        std::string error = "Query returned the incorrect number of fields; ";
        error.append("Expected: ").append(std::to_string(NUM_STEP_END_FIELDS));
        error.append(" Received: ").append( std::to_string(fields->nfields));
        LOG( csmapi, error ) << STATE_NAME" " << error;

        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage(error);
    }

    LOG( csmapi, trace ) << STATE_NAME ":ParseInfoQuery: Exit";

    return success;
}

bool CSMIAllocationStepEnd::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    return CreateByteArray(buf, bufLen, ctx);
}

bool CSMIAllocationStepEnd::CreateByteArray(
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
        "; Message: Step End completed";
    dataLock.unlock();

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true; 
}

CSMIAllocationStepEnd_Agent::CSMIAllocationStepEnd_Agent( 
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
