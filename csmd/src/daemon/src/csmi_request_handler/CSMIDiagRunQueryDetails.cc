/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIDiagRunQueryDetails.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota
* Email: nbuonar@us.ibm.com
*/

#include "CSMIDiagRunQueryDetails.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

#define STATE_NAME "CSMIDiagRunQueryDetails:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT  csm_diag_run_query_details_input_t
#define OUTPUT_STRUCT  csm_diag_run_query_details_output_t

// There's one additional state that needs to be registered.
#define EXTRA_STATES 1
CSMIDiagRunQueryDetails::CSMIDiagRunQueryDetails(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CSM_CMD_diag_run_query_details, options, STATEFUL_DB_DONE + EXTRA_STATES)
{
    const uint32_t final_state = STATEFUL_DB_DONE + EXTRA_STATES;
    uint32_t current_state = STATEFUL_DB_RECV_DB;
    uint32_t next_state = current_state + 1;

    SetState( current_state,
        new StatefulDBRecvSend<CreateResponsePayload>(
            next_state,
            final_state,
            final_state ) );
}
#undef EXTRA_STATES

bool CSMIDiagRunQueryDetails::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt = 
            "SELECT "
                "NULL as history_time, run_id, allocation_id, "
                "begin_time, NULL as end_time, status, "
                "inserted_ras, log_dir, cmd_line "
            "FROM csm_diag_run "
            "WHERE run_id=$1::bigint "
            "UNION ALL "
            "SELECT "
                "history_time, run_id, allocation_id, "
                "begin_time, end_time, status, "
                "inserted_ras, log_dir, cmd_line "
            "FROM csm_diag_run_history " 
            "WHERE run_id=$1::bigint "
            "ORDER BY run_id, history_time";

        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(input->run_id);

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

bool CSMIDiagRunQueryDetails::CreateResponsePayload(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreateResponsePayload: Enter";
    
    size_t numRecords = tuples.size();
    bool success = true;

    if (numRecords == 1 )
    {
        csm::db::DBTuple *fields = tuples[0];

        // EARLY RETURN !
        if ( fields->nfields != 9 )
        {
            ctx->SetErrorCode( CSMERR_DB_ERROR );
            ctx->SetErrorMessage("Incorrect number of fields received by query.");

            return false;
        }

        // Build the output struct from the database results.
        OUTPUT_STRUCT* o = nullptr;
        csm_init_struct_ptr(OUTPUT_STRUCT, o );

        csmi_diag_run_t* r = nullptr; 
        csm_init_struct_ptr(csmi_diag_run_t, r);

        r->history_time    = strdup(fields->data[0]);
        r->run_id          = strtoll(fields->data[1], nullptr, 10);
        r->allocation_id   = strtoll(fields->data[2], nullptr, 10);
        r->begin_time      = strdup(fields->data[3]);
        r->end_time        = strdup(fields->data[4]);
        r->inserted_ras    = fields->data[6][0];
        r->log_dir         = strdup(fields->data[7]);
        r->cmd_line        = strdup(fields->data[8]);
         
        strncpy(r->diag_status, fields->data[5], CSM_STATUS_MAX);
        r->diag_status[CSM_STATUS_MAX-1] = 0;

        o->run_data = r;

        // Cache the results for later.
        ctx->SetDataDestructor(
            []( void* data ){
                
                free_csm_diag_run_query_details_output_t(
                    (csm_diag_run_query_details_output_t*) data);
                free(data);
                data = nullptr;
            }
        );
        ctx->SetUserData( o );

        // Build the payload.
        std::string stmt = 
            "SELECT "
                "NULL as history_time, run_id, test_name, "
                "node_name, serial_number, begin_time, "
                "end_time, status, log_file "
            "FROM csm_diag_result "
            "WHERE run_id = $1::bigint "
            "UNION ALL "
            "SELECT "
                "history_time, run_id, test_name, "
                "node_name, serial_number, begin_time, "
                "end_time, status, log_file "
            "FROM csm_diag_result_history "
            "WHERE run_id = $1::bigint "
            "ORDER BY run_id, history_time";
            
        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(r->run_id);

        *dbPayload = dbReq;
        LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
    }
    else if ( numRecords == 0 )
    {
        ctx->SetErrorCode(CSMI_NO_RESULTS);
        success = false;
    }
    else
    {
        // Build the verbose error.
        std::string error = "The supplied run id had ";
        error.append(std::to_string(numRecords));
        error.append(" records in the csm_diag_run and csm_diag_run_history tables");

        LOG( csmapi, error ) << STATE_NAME ":CreateResponsePayload: " << error;

        ctx->SetErrorCode( CSMERR_DB_ERROR );
        ctx->SetErrorMessage( error );
        success = false;
    }
    
    LOG( csmapi, trace ) << STATE_NAME ":CreateResponsePayload: Exit";

    return success;
}

bool CSMIDiagRunQueryDetails::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf = nullptr;
    bufLen = 0;
    
    // Get the output struct first.
    OUTPUT_STRUCT *output= nullptr;
    std::unique_lock<std::mutex>dataLock = ctx->GetUserData<OUTPUT_STRUCT*>(&output);

    uint32_t numRecords = tuples.size();

    if ( numRecords > 0 )
    {
        output->num_details = numRecords;

        output->details = (csmi_diag_run_query_details_result_t**) malloc( 
            numRecords * sizeof(csmi_diag_run_query_details_result_t*));

        for ( uint32_t i = 0; i < numRecords; ++i )
        {
            CreateOutputStruct( tuples[i], &(output->details[i]) );
        }
    }

    // Serialize the struct.
    csm_serialize_struct( OUTPUT_STRUCT, output, buf, &bufLen);
    
    // Unlock the user data and then release it.
    dataLock.unlock();
    ctx->SetUserData(nullptr);

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    return true;
}

void CSMIDiagRunQueryDetails::CreateOutputStruct(
    csm::db::DBTuple * const & fields,
    csmi_diag_run_query_details_result_t **output )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";

    if ( fields->nfields != 9 ) 
    {
        *output = nullptr;
        return;
    }

    csmi_diag_run_query_details_result_t* o = nullptr;
    csm_init_struct_ptr(csmi_diag_run_query_details_result_t, o);

	o->history_time     = strdup (fields->data[0]);
    o->run_id           = strtoll(fields->data[1], nullptr, 10);
    o->test_name        = strdup (fields->data[2]); 
    o->node_name        = strdup (fields->data[3]); 
    o->serial_number    = strdup (fields->data[4]); 
    o->begin_time       = strdup (fields->data[5]); 
    o->end_time         = strdup (fields->data[6]); 
    o->log_file         = strdup (fields->data[8]); 

	strncpy(o->status, fields->data[7], CSM_STATUS_MAX);
	o->status[CSM_STATUS_MAX-1] = 0;

	*output = o;
	
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}

