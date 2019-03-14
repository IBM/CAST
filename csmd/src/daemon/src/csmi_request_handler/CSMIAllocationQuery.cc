/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationQuery.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIAllocationQuery.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

#define STATE_NAME "CSMIAllocationQuery:"
// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_query_input_t
#define OUTPUT_STRUCT csm_allocation_query_output_t
#define ALLOC_STRUCT csmi_allocation_t
#define EXTRA_STATES 1
CSMIAllocationQuery::CSMIAllocationQuery(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CSM_CMD_allocation_query, options, STATEFUL_DB_DONE + EXTRA_STATES)
{
    const uint32_t final_state = STATEFUL_DB_DONE + EXTRA_STATES;
    uint32_t current_state = STATEFUL_DB_RECV_DB;
    uint32_t next_state = current_state + 1;

    SetState( current_state++,
        new StatefulDBRecvSend<CreateResponsePayload>(
            next_state++,
            final_state,
            final_state ) );
}
#undef EXTRA_STATES

bool CSMIAllocationQuery::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        int paramCount = 0;
        // First define the parameter's string, 
        // If an allocation was supplied use it, otherwise use primary and secondary job ids.
        std::string paramStmt = "";
        if ( input->allocation_id > 0 )
        {
            paramStmt.append("a.allocation_id= $1::bigint ");
            paramCount++;
        }
        else 
        {
            if( input->primary_job_id > 0 ) 
            {
                paramStmt.append("a.primary_job_id = $1::bigint ");
                paramCount++;
            }

            // Secondary job_id can be 0.
            if ( input->secondary_job_id >= 0 && paramCount > 0)
                paramStmt.append("AND a.secondary_job_id = $").append(
                    std::to_string(++paramCount)).append("::integer ");
        }

        // Early return if the query was malformed.
        if ( paramCount == 0 )
        {
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Incomplete values supplied";
            ctx->SetErrorCode(CSMERR_MISSING_PARAM);
            ctx->SetErrorMessage("Unable to build query, verify that an allocation id greater than 0 or a primary job id greater than zero has been supplied");
            csm_free_struct_ptr(INPUT_STRUCT, input);

            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
            return false;
        }

        // Build the SQL statement.
        #define SELECT_BODY \
            "a.allocation_id, a.primary_job_id, a.secondary_job_id, "\
            "a.ssd_file_system_name, a.launch_node_name, a.user_flags, "\
            "a.system_flags, a.ssd_min, a.ssd_max, a.num_nodes, "\
            "a.num_processors, a.num_gpus, a.projected_memory, "\
            "a.state, a.type, a.job_type, "\
            "a.user_name, a.user_id, a.user_group_id, "\
            "a.user_script, a.begin_time, a.account, "\
            "a.comment,  a.job_name, "\
            "a.job_submit_time, "\
            "a.queue, a.requeue, a.time_limit, "\
            "a.wc_key, a.isolated_cores, a.smt_mode "

        std::string stmt = "SELECT "
            SELECT_BODY
            ",NULL as end_time, NULL as exit_status, "
            "NULL as archive_history_time "
            "FROM csm_allocation a "
            "WHERE ";
        stmt.append(paramStmt).append(
        "UNION "
            "SELECT "
            SELECT_BODY
            ",a.end_time, a.exit_status, "
            "a.archive_history_time "
            "FROM csm_allocation_history a " 
            "WHERE ").append(paramStmt);
        #undef SELECT_BODY
        
        //==========================================================================

        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );

        // If the allocation was supplied only add it, otherwise add the job ids.
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

        LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
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
bool CSMIAllocationQuery::CreateResponsePayload(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
    bool success = true;
    uint32_t numRecords = tuples.size();

    if ( numRecords < 1 )
    {
        ctx->SetErrorCode( CSMI_NO_RESULTS );
        return false;
    }
    ALLOC_STRUCT* allocation = NULL;
    
    // Create the struct for most of the allocation.
    if ( CreateOutputStruct( tuples[0], &(allocation)) )
    {
        // We really only care about some of the details on the other nodes.
        if ( numRecords > 1 )
        {
            allocation->num_allocations = numRecords-1;
            allocation->allocations = (ALLOC_STRUCT**) calloc( numRecords-1,sizeof(ALLOC_STRUCT*));
            
            for ( uint32_t i = 1; i < numRecords; ++i )
            {
                CreateOutputStruct( tuples[i], &(allocation->allocations[i-1]));
            }
        }

        ctx->SetDataDestructor(
            [](void* data){
                free_csmi_allocation_t(
                    (csmi_allocation_t*) data);
                free(data);
                data=nullptr;
            }
        );

        ctx->SetUserData(allocation);

        std::string stmt = "SELECT DISTINCT ON(node_name) node_name FROM ";
        stmt.append(allocation->history ?  "csm_allocation_node_history " : "csm_allocation_node " );
        stmt.append("WHERE allocation_id=$1::bigint");

        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(allocation->allocation_id);
        *dbPayload = dbReq;


    }
    else
    {
        LOG( csmapi, trace  ) << STATE_NAME ":CreateByteArray: Invalid number of fields retrieved.";
        
        ctx->SetErrorCode( CSMERR_DB_ERROR );
        ctx->SetErrorMessage( "Query results could not be processed.");
        success = false;
    }

    return success;
}

bool CSMIAllocationQuery::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
	LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    bool success = true;
    *buf = nullptr;
    bufLen = 0;

    OUTPUT_STRUCT output;
    csm_init_struct(OUTPUT_STRUCT,output);

    // Get the allocation.
    std::unique_lock<std::mutex>dataLock = ctx->GetUserData<ALLOC_STRUCT*>(&output.allocation);

    if (output.allocation)
    {
        // Prep the num records.
        uint32_t numRecords = tuples.size();
        output.allocation->num_nodes = numRecords;
        if ( numRecords > 0 )
        {
            output.allocation->compute_nodes = (char **)malloc(sizeof(char *) * numRecords);
        }

        for(uint32_t i = 0; i < numRecords; ++i)
        {
            if ( tuples[i] && tuples[i]->data)
            {
                output.allocation->compute_nodes[i] = strdup(tuples[i]->data[0]);
            }
        }

        // Serialize and free.
        csm_serialize_struct( OUTPUT_STRUCT, &output, buf, &bufLen);
        output.allocation = nullptr;
    }
    else
    {
        if( ctx->GetErrorCode() == CSMI_SUCCESS)
        {
            ctx->SetErrorCode( CSMERR_CONTEXT_LOST );
            ctx->SetErrorMessage( "Context of query was corrupted or not set." );
        }
        success=false;
    }

    dataLock.unlock();
    ctx->SetUserData(nullptr);

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    return success;
}

bool CSMIAllocationQuery::CreateOutputStruct(
    csm::db::DBTuple * const & fields,
    ALLOC_STRUCT **output )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";
	
    if ( !( fields->nfields == 31 || fields->nfields == 34 ) ) 
    {
        *output = nullptr;
        return false;
    }

    ALLOC_STRUCT *o = nullptr;
    csm_init_struct_ptr(ALLOC_STRUCT, o);

    o->allocation_id        = strtoll(fields->data[0], nullptr, 10);
    o->primary_job_id       = strtoll(fields->data[1], nullptr, 10);
    o->secondary_job_id     = strtol(fields->data[2], nullptr, 10);
    o->ssd_file_system_name = strdup(fields->data[3]);
    o->launch_node_name     = strdup(fields->data[4]);
    o->user_flags           = strdup(fields->data[5]);
    o->system_flags         = strdup(fields->data[6]);
    o->ssd_max              = strtoll(fields->data[7], nullptr, 10);
    o->ssd_min              = strtoll(fields->data[8], nullptr, 10);
    o->num_nodes            = strtol(fields->data[9], nullptr, 10);
    o->num_processors       = strtol(fields->data[10], nullptr, 10);
    o->num_gpus             = strtol(fields->data[11], nullptr, 10);
    o->projected_memory     = strtol(fields->data[12], nullptr, 10);
    o->state                = (csmi_state_t)csm_get_enum_from_string(csmi_state_t, fields->data[13]);
    o->type                 = (csmi_allocation_type_t)csm_get_enum_from_string(csmi_allocation_type_t, fields->data[14]);
    o->job_type             = (csmi_job_type_t)csm_get_enum_from_string(csmi_job_type_t, fields->data[15]);
    o->user_name            = strdup(fields->data[16]);
    o->user_id              = strtol(fields->data[17], nullptr, 10);
    o->user_group_id        = strtol(fields->data[18], nullptr, 10);
    o->user_script          = strdup(fields->data[19]);
    o->begin_time           = strdup(fields->data[20]);
    o->account              = strdup(fields->data[21]);
    o->comment              = strdup(fields->data[22]);
    o->job_name             = strdup(fields->data[23]);
    o->job_submit_time      = strdup(fields->data[24]);
    o->queue                = strdup(fields->data[25]);
    o->requeue              = strdup(fields->data[26]);
    o->time_limit           = strtoll(fields->data[27], nullptr, 10);
    o->wc_key               = strdup(fields->data[28]);
    o->isolated_cores       = strtol(fields->data[29], nullptr, 10);
    o->smt_mode             = (int16_t) strtol(fields->data[30], nullptr, 10);
                                                  
    
    // IFF a history time was found build the history.
    if ( fields->data[31][0] )
    {
        o->history = (csmi_allocation_history_t *)malloc(sizeof(csmi_allocation_history_t));

        o->history->end_time             = strdup(fields->data[31]);
        o->history->exit_status          = strtol(fields->data[32], nullptr, 10);
        o->history->archive_history_time = strdup(fields->data[33]);
    }

    *output = o;
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
    
    return true;
}

