/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationQueryDetails.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIAllocationQueryDetails.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

#define STATE_NAME "CSMIAllocationQueryDetails:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_query_details_input_t
#define OUTPUT_STRUCT csm_allocation_query_details_output_t 

// There's one additional state that needs to be registered.
#define EXTRA_STATES 2
CSMIAllocationQueryDetails::CSMIAllocationQueryDetails(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CSM_CMD_allocation_query_details, options, STATEFUL_DB_DONE + EXTRA_STATES)
{
    const uint32_t final_state = STATEFUL_DB_DONE + EXTRA_STATES;
    uint32_t current_state = STATEFUL_DB_RECV_DB;
    uint32_t next_state = current_state + 1;

    SetState( current_state++,
        new StatefulDBRecvSend<CreateResponsePayload>(
            next_state++,
            final_state,
            final_state ) );


    SetState( current_state,
        new StatefulDBRecvSend<CreateStepPayload>(
            next_state,
            final_state,
            final_state ) );
}
#undef EXTRA_STATES

bool CSMIAllocationQueryDetails::RetrieveDataForPrivateCheck(
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
        std::string stmt = "SELECT user_id "
                "FROM csm_allocation "
                "WHERE allocation_id = $1::bigint "
            "UNION "
                "SELECT user_id "
                "FROM csm_allocation_history "
                "WHERE allocation_id = $1::bigint";

        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( input->allocation_id );
        *dbPayload = dbReq;

        ctx->SetErrorMessage("Allocation ID: " +  std::to_string(input->allocation_id) + ";");
        csm_free_struct_ptr(INPUT_STRUCT, input);

        LOG( csmapi, trace ) << STATE_NAME ":RetrieveDataForPrivateCheck: Parameterized SQL: " 
            << stmt;
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

bool CSMIAllocationQueryDetails::CompareDataForPrivateCheck(
        const std::vector<csm::db::DBTuple *>& tuples,
        const csm::network::Message &msg,
        csm::daemon::EventContextHandlerState_sptr& ctx)
{
    LOG( csmapi, trace ) << STATE_NAME ":CompareDataForPrivateCheck: Enter";
	
    bool success = false;

    if ( tuples.size() == 1 )
    {
		uint32_t userID = strtoul(tuples[0]->data[0], nullptr, 10);
        LOG( csmapi, trace ) << STATE_NAME << " Found User: " << userID << " Message User: " <<
                msg.GetUserID();
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
            error.append(std::to_string(msg.GetUserID())).append(" does not have permission to "
                "access allocation details;");

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

bool CSMIAllocationQueryDetails::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    bool success = true;;
	
    // Unpack the buffer.
    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 ) 
    {
        // Build the SQL statement.
        #define SELECT_BODY \
            "a.allocation_id, a.primary_job_id, a.secondary_job_id, "\
            "a.ssd_file_system_name, a.launch_node_name, a.user_flags, "\
            "a.system_flags, a.ssd_min, a.ssd_max, a.num_nodes, "\
            "a.num_processors, a.num_gpus, a.projected_memory, "\
            "a.state, a.type, a.job_type, "\
            "a.user_name, a.user_id, a.user_group_id, "\
            "a.user_script, a.begin_time, "\
            "a.account, a.comment, "\
            "a.job_name, "\
            "a.job_submit_time, a.queue, "\
            "a.requeue, a.time_limit, a.wc_key, "\
            "a.isolated_cores, a.smt_mode, a.core_blink " 

        std::string stmt = 
            "SELECT "
                "a.*,"
                "COUNT(sh) as sh_num_transitions,"
                "array_agg(sh.history_time) as a_sh_history_time,"
                "array_agg(sh.state) as a_sh_state "
            "FROM ( SELECT "
                SELECT_BODY
                    ",NULL as end_time, NULL as exit_status, "
                    "NULL as archive_history_time "
                "FROM csm_allocation a "
                "WHERE a.allocation_id = $1::bigint "
                "UNION SELECT "
                    SELECT_BODY
                    ",a.end_time, a.exit_status, "
                    "a.archive_history_time "
                "FROM csm_allocation_history a "
                "WHERE a.allocation_id = $1::bigint"
            ") as a "
            "LEFT JOIN ("
                "SELECT history_time,state,allocation_id "
                "FROM csm_allocation_state_history WHERE allocation_id = $1::bigint "
                "ORDER BY history_time desc) as sh "
            "ON sh.allocation_id = a.allocation_id "
            "GROUP BY " SELECT_BODY ",a.end_time,a.exit_status,a.archive_history_time";

        #undef SELECT_BODY
        // =============================================================================

        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        if( input->allocation_id > 0 ) 
            dbReq->AddNumericParam<int64_t>(input->allocation_id);
        else
        {
            LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";

            ctx->SetErrorCode( CSMERR_INVALID_PARAM );
            ctx->SetErrorMessage("Unable to build the query, Allocation ID was < 0 ");

            success = false;
        }

        *dbPayload = dbReq;
        csm_free_struct_ptr( INPUT_STRUCT, input );
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";

        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the query, struct could not be deserialized");
        success = false;
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return success;
}

bool CSMIAllocationQueryDetails::CreateResponsePayload(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreateResponsePayload: Enter";
    
    size_t numRecords = tuples.size();

    if (numRecords == 1 )
    {
        csm::db::DBTuple *fields = tuples[0];

        if ( !( fields->nfields == 37 ) ) 
        {
            ctx->SetErrorCode( CSMERR_DB_ERROR );
            ctx->SetErrorMessage("Incorrect number of fields received by query.");
        }

        OUTPUT_STRUCT *o = nullptr;
        csm_init_struct_ptr(OUTPUT_STRUCT, o);

        csmi_allocation_t *a = nullptr;
        csm_init_struct_ptr(csmi_allocation_t, a);

        csmi_allocation_details_t *ad = nullptr;
        csm_init_struct_ptr( csmi_allocation_details_t, ad );

        a->allocation_id        = strtoll(fields->data[0], nullptr, 10);
        a->primary_job_id       = strtoll(fields->data[1], nullptr, 10);
        a->secondary_job_id     = strtol(fields->data[2], nullptr, 10);
        a->ssd_file_system_name = strdup(fields->data[3]);
        a->launch_node_name     = strdup(fields->data[4]);
        a->user_flags           = strdup(fields->data[5]);
        a->system_flags         = strdup(fields->data[6]);
        a->ssd_min             = strtoll(fields->data[7], nullptr, 10);
        a->ssd_max             = strtoll(fields->data[8], nullptr, 10);
        a->num_nodes            = strtol(fields->data[9], nullptr, 10);
        a->num_processors       = strtol(fields->data[10], nullptr, 10);
        a->num_gpus             = strtol(fields->data[11], nullptr, 10);
        a->projected_memory     = strtol(fields->data[12], nullptr, 10);
        a->state                = (csmi_state_t)csm_get_enum_from_string(csmi_state_t, fields->data[13]);
        a->type                 = (csmi_allocation_type_t)csm_get_enum_from_string(csmi_allocation_type_t,fields->data[14]);
        a->job_type             = (csmi_job_type_t)csm_get_enum_from_string(csmi_job_type_t,fields->data[15]);
        a->user_name            = strdup(fields->data[16]);
        a->user_id              = strtol(fields->data[17], nullptr, 10);
        a->user_group_id        = strtol(fields->data[18], nullptr, 10);
        a->user_script          = strdup(fields->data[19]);
        a->begin_time           = strdup(fields->data[20]);
        a->account              = strdup(fields->data[21]);
        a->comment              = strdup(fields->data[22]);
        a->job_name             = strdup(fields->data[23]);
        a->job_submit_time      = strdup(fields->data[24]);
        a->queue                = strdup(fields->data[25]);
        a->requeue              = strdup(fields->data[26]);
        a->time_limit           = strtoll(fields->data[27], nullptr, 10);
        a->wc_key               = strdup(fields->data[28]);
        a->isolated_cores       = strtol(fields->data[29], nullptr, 10);
        a->smt_mode             = (int16_t)strtol(fields->data[30], nullptr, 10);
        a->core_blink           = strtol(fields->data[31], nullptr, 10) == CSM_TRUE; 

        // IFF a history time was found build the history.
        if ( fields->data[32][0] )
        {
            a->history = (csmi_allocation_history_t *)malloc(sizeof(csmi_allocation_history_t));

            a->history->end_time             = strdup(fields->data[32]);
            a->history->exit_status          = strtol(fields->data[33], nullptr, 10);
            a->history->archive_history_time = strdup(fields->data[34]);
        }

        // Transition table.
        ad->num_transitions = strtoll(fields->data[35], nullptr, 10); 
        
        if ( ad->num_transitions > 0 )
        {
            ad->state_transitions = (csmi_allocation_state_history_t**)malloc(
                sizeof(csmi_allocation_state_history_t*) * ad->num_transitions);

            // Malloc the transitions.
            for( uint32_t i = 0; i < ad->num_transitions; ++i )
            {
                ad->state_transitions[i] = (csmi_allocation_state_history_t*)
                    malloc(sizeof(csmi_allocation_state_history_t));
            }

            csm_prep_csv_to_struct();

            csm_parse_psql_array_to_struct(fields->data[35], ad->state_transitions,
                        ad->num_transitions, ->history_time,
                        strdup("N/A"), strdup);

            // No easy way to do this.
            i = 0;                                                  
            nodeStr = strtok_r(fields->data[36], ",\"{}", &saveptr);
            while ( nodeStr != NULL && i < ad->num_transitions )             
            {                                                       
                ad->state_transitions[i++]->state = (csmi_state_t)csm_get_enum_from_string(csmi_state_t, nodeStr); 
                nodeStr = strtok_r(NULL, ",\"{}", &saveptr);  
            }                                                       
            while ( i < ad->num_transitions )                                
            {                                                       
                ad->state_transitions[i++]->state = csmi_state_t_MAX;
            }

        }

        // Set the number of nodes for the details.
        ad->num_nodes = a->num_nodes;

        // Cache the allocation.
        o->allocation = a;
        o->allocation_details = ad;

        // Cache the results for later.
        ctx->SetDataDestructor(
            []( void* data ){
                
                free_csm_allocation_query_details_output_t(
                    (csm_allocation_query_details_output_t*) data );
                free(data);
                data = nullptr;
            }
        );
        ctx->SetUserData( o );
        
        // Get allocation node information.
        std::string stmt = "SELECT  array_agg(n.node_name) ";
        if ( a->history )
        {
            
            stmt.append(",array_agg(n.gpfs_read),"
                "array_agg(n.gpfs_write),"
                "array_agg(n.ib_tx),"
                "array_agg(n.ib_rx),"
                "array_agg(n.power_cap),"
                "array_agg(n.power_shifting_ratio) as a_psr,"
                "array_agg(n.power_cap_hit),"
                "array_agg(n.gpu_energy),"
                "array_agg(n.cpu_usage),"
                "array_agg(n.memory_usage_max),"
                "array_agg(n.energy),"
                "array_agg(n.gpu_usage) "
            "FROM ( SELECT DISTINCT ON (node_name) * FROM csm_allocation_node_history "
                "WHERE allocation_id = $1::bigint "
                "ORDER BY node_name, history_time desc ) as n");
        }
        else
        {
            stmt.append("FROM ( "
                "SELECT node_name FROM csm_allocation_node "
                "WHERE allocation_id = $1::bigint "
                "ORDER BY node_name ) as n");
        }

        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( a->allocation_id );
        *dbPayload = dbReq;
    }
    else 
    {
        LOG( csmapi, error ) << STATE_NAME ":CreateResponsePayload: Bad Record Count: " 
            << numRecords;
        LOG( csmapi, trace  ) << STATE_NAME ":CreateResponsePayload: Exit";
        
        if ( numRecords == 0 )
        {
            ctx->SetErrorCode(CSMI_NO_RESULTS);
            ctx->SetErrorMessage("Query of allocation tables had no results");
        }
        else
        {
            ctx->SetErrorCode( CSMERR_DB_ERROR );
            ctx->SetErrorMessage("Query of allocation tables had too many results");
        }

        return false;
    }
        
    LOG( csmapi, trace ) << STATE_NAME ":CreateResponsePayload: Exit";
    
    return true;
}

bool CSMIAllocationQueryDetails::CreateStepPayload(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreateResponsePayload: Enter";
    
    size_t numRecords = tuples.size();

    if (numRecords == 1 )
    {
        csm::db::DBTuple *fields = tuples[0];

        if ( !( fields->nfields == 1 || fields->nfields == 13 ) ) 
        {
            ctx->SetErrorCode( CSMERR_DB_ERROR );
            ctx->SetErrorMessage("Incorrect number of fields received by query.");
            return false;
        }
    
        // Get the output Struct first.
        OUTPUT_STRUCT *output= nullptr;
        std::unique_lock<std::mutex>dataLock = ctx->GetUserData<OUTPUT_STRUCT*>(&output);

        // Allocation details struct.
        csmi_allocation_t *a          = output->allocation;
        csmi_allocation_details_t *ad = output->allocation_details;

        // If more than one node was found scan the compute nodes.
        if ( a->num_nodes > 0 ) 
        {
            a->compute_nodes = (char **)malloc(sizeof(char *) * a->num_nodes);
            csm_prep_csv_to_struct();

            // Parse the compute nodes.
            //========================================================================
            csm_parse_psql_array_to_struct(fields->data[0], a->compute_nodes, 
                                    a->num_nodes, CSM_NO_MEMBER, 
                                    strdup("N/A"), strdup);
            //========================================================================
           ad->node_accounting = (csmi_allocation_accounting_t**)malloc(
               sizeof(csmi_allocation_accounting_t*) * a->num_nodes);

           // Calloc the node accounting.
           for( uint32_t i = 0; i < a->num_nodes; ++i )
           {
               ad->node_accounting[i] = (csmi_allocation_accounting_t*)
                    calloc(1,sizeof(csmi_allocation_accounting_t));
           }
            
            if ( fields->nfields > 1 )
            {
                // Parse the csv for the accounting data.
                //========================================================================
                csm_parse_psql_array_to_struct(fields->data[1], ad->node_accounting, 
                            ad->num_nodes, ->gpfs_read, 
                            0, csm_to_int64);
                csm_parse_psql_array_to_struct(fields->data[2], ad->node_accounting, 
                            ad->num_nodes, ->gpfs_write, 
                            0, csm_to_int64);
                csm_parse_psql_array_to_struct(fields->data[3], ad->node_accounting, 
                            ad->num_nodes, ->ib_tx, 
                            0, csm_to_int64);
                csm_parse_psql_array_to_struct(fields->data[4], ad->node_accounting, 
                            ad->num_nodes, ->ib_rx, 
                            0, csm_to_int64);
                //========================================================================
                
                // Power details.
                csm_parse_psql_array_to_struct(fields->data[5], ad->node_accounting, 
                            ad->num_nodes, ->power_cap, 
                            -1, csm_to_int32);

                csm_parse_psql_array_to_struct(fields->data[6], ad->node_accounting, 
                            ad->num_nodes, ->power_shifting_ratio, 
                            -1, csm_to_int32);

                csm_parse_psql_array_to_struct(fields->data[7], ad->node_accounting,
                            ad->num_nodes, ->power_cap_hit,
                            -1, csm_to_int64);

                csm_parse_psql_array_to_struct(fields->data[8], ad->node_accounting,
                            ad->num_nodes, ->gpu_energy,
                            -1, csm_to_int64);

                csm_parse_psql_array_to_struct(fields->data[9], ad->node_accounting,
                            ad->num_nodes, ->cpu_usage,
                            -1, csm_to_int64);

                csm_parse_psql_array_to_struct(fields->data[10], ad->node_accounting,
                            ad->num_nodes, ->memory_usage_max,
                            -1, csm_to_int64);

                csm_parse_psql_array_to_struct(fields->data[11], ad->node_accounting,
                            ad->num_nodes, ->energy_consumed,
                            -1, csm_to_int64);

                csm_parse_psql_array_to_struct(fields->data[12], ad->node_accounting,
                            ad->num_nodes, ->gpu_usage,
                            -1, csm_to_int64);
            }
        }
        


        // Build the payload, gets both the active and history steps.
        std::string stmt =
            "SELECT "
                "s.step_id,s.num_nodes,CAST(NULL AS DATE) as end_time, "
                "array_to_string(array_agg(sn.node_name),',') AS nodelist "
            "FROM csm_step s "
            "JOIN csm_step_node sn "
                " ON s.allocation_id=sn.allocation_id AND s.step_id=sn.step_id "
            "WHERE s.allocation_id = $1::bigint "
            "GROUP BY s.allocation_id, s.step_id, s.num_nodes, end_time "
            "UNION ALL "
            "SELECT "
                "s.step_id,s.num_nodes,s.end_time,"
                "array_to_string(array_agg(sn.node_name),',') AS nodelist "
            "FROM csm_step_history s "
            "JOIN csm_step_node_history sn "
                " ON s.allocation_id=sn.allocation_id AND s.step_id=sn.step_id "
            "WHERE s.allocation_id = $1::bigint "
            "GROUP BY s.allocation_id, s.step_id, s.num_nodes, s.end_time  "
            "ORDER BY  end_time DESC, step_id ASC" ;
            
        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(a->allocation_id);

        *dbPayload = dbReq;
    }
    else 
    {
        LOG( csmapi, error ) << STATE_NAME ":CreateResponsePayload: Bad Record Count: " 
            << numRecords;
        LOG( csmapi, trace  ) << STATE_NAME ":CreateResponsePayload: Exit";
        
        if ( numRecords == 0 )
        {
            ctx->SetErrorCode(CSMI_NO_RESULTS);
            ctx->SetErrorMessage("Query of allocation tables had no results");
        }
        else
        {
            ctx->SetErrorCode( CSMERR_DB_ERROR );
            ctx->SetErrorMessage("Query of allocation tables had too many results");
        }

        return false;
    }
    
    LOG( csmapi, trace ) << STATE_NAME ":CreateResponsePayload: Exit";

    return true;
}

bool CSMIAllocationQueryDetails::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
	LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    bool success = true;
    *buf = nullptr;
    bufLen = 0;

    // Get the output Struct first.
    OUTPUT_STRUCT *output= nullptr;
    std::unique_lock<std::mutex>dataLock = ctx->GetUserData<OUTPUT_STRUCT*>(&output);

    // Allocation details struct.
    if ( output )
    {
        csmi_allocation_details_t *ad = output->allocation_details;
        ad->num_steps = tuples.size();

        if ( ad->num_steps )
        {
            ad->steps = (csmi_allocation_step_list_t **)
                malloc( ad->num_steps * sizeof(csmi_allocation_step_list_t *));

            for ( uint32_t i = 0; i < ad->num_steps; ++i )
            {
                CreateOutputStruct( tuples[i], &(ad->steps[i]) );
            }
        }

        csm_serialize_struct( OUTPUT_STRUCT, output, buf, &bufLen );
    }
    else
    {
        if( ctx->GetErrorCode() == CSMI_SUCCESS)
        {
            ctx->SetErrorCode( CSMERR_CONTEXT_LOST );
            ctx->SetErrorMessage( "Context of query was corrupted or not set." );
        }
        success = false;
    }

    // Unlock the user data then release it.
    dataLock.unlock();
    ctx->SetUserData(nullptr);

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    return success;
}

void CSMIAllocationQueryDetails::CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        csmi_allocation_step_list_t  **output )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateOutputStruct: Enter";

    if ( !( fields->nfields == 4 ) )
    {
        *output = nullptr;
        return;
    }

    csmi_allocation_step_list_t *o = nullptr;
    csm_init_struct_ptr(csmi_allocation_step_list_t , o);

    o->step_id       = strtoll(fields->data[0], nullptr, 10);
    o->num_nodes     = strtol(fields->data[1], nullptr, 10);
    o->end_time      = strdup(fields->data[2]);
    o->compute_nodes = strdup(fields->data[3]);

    *output = o;

    LOG( csmapi, trace ) << STATE_NAME ":CreateOutputStruct: Exit";
}

