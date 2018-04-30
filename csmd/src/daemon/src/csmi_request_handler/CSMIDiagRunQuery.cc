/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIDiagRunQuery.cc

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

/* Header for this file. */
#include "CSMIDiagRunQuery.h"

#define STATE_NAME "CSMIDiagRunQuery:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_diag_run_query_input_t
#define OUTPUT_STRUCT csm_diag_run_query_output_t
#define RUN_STRUCT csmi_diag_run_t

bool CSMIDiagRunQuery::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        int paramCount = 0;

        // =====================================================================
            
        std::string  stmtParams = "";
        add_param_sql( stmtParams, input->allocation_ids_count > 0, ++paramCount, 
            "allocation_id = ANY($", "::bigint[]) AND " )

        add_param_sql( stmtParams, input->begin_time_search_begin[0], ++paramCount, 
            "begin_time >= $", "::timestamp AND " )

        add_param_sql( stmtParams, input->begin_time_search_end[0], ++paramCount, 
            "begin_time <= $", "::timestamp AND " )

        
        bool rasInsertedTrue =input->inserted_ras == 't' || 
            input->inserted_ras == 'T' || 
            input->inserted_ras == '1'; 

        bool rasInserted = rasInsertedTrue || 
            input->inserted_ras == 'F' || 
            input->inserted_ras == 'f' || 
            input->inserted_ras == '0';
        add_param_sql( stmtParams, rasInserted, ++paramCount, 
            "inserted_ras = $", "::boolean AND ")

        add_param_sql( stmtParams, input->run_ids_count > 0, ++paramCount, 
            "run_id = ANY($","::bigint[]) AND ")
        
        add_param_sql( stmtParams, input->status, ++paramCount,
            "status=ANY($","::text[]) AND ")
        // =====================================================================
        std::string stmt =  "";

        // If the param count is greater than zero, 
        // then this a query for the active table as well.
        if( paramCount > 0 )
        {
            // Replace the last 4 characters if any parameters were found.
            // Remove the AND.
            int len = stmtParams.length() - 1;
            for( int i = len - 3; i < len; ++i)
                stmtParams[i] = ' ';

            stmt.append(
                "SELECT "
                    "run_id, allocation_id, begin_time, "
                    "cmd_line, NULL as end_time, NULL as history_time, "
                    "inserted_ras, log_dir, status "
                "FROM csm_diag_run WHERE ");

            stmt.append(stmtParams);
            stmt.append(" UNION ALL ");
        }

        stmt.append( 
            "SELECT "
                "run_id, allocation_id, begin_time, "
                "cmd_line, end_time, history_time, "
                "inserted_ras, log_dir, status "
            "FROM csm_diag_run_history WHERE " );

        stmt.append(stmtParams);
        
        if ( input->end_time_search_begin[0] ) 
        { 
            if ( paramCount > 0 ) stmt.append("AND ");
            stmt.append("end_time >= $").append(
                std::to_string(++paramCount)).append("::timestamp "); 
        }

        if ( input->end_time_search_end[0] ) 
        { 
            if ( paramCount > 0 ) stmt.append("AND ");
            stmt.append("end_time <= $").append(
                std::to_string(++paramCount)).append("::timestamp "); 
        }

        if( paramCount == 0 )
        {
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: No values supplied";
            ctx->SetErrorCode(CSMERR_MISSING_PARAM);
            ctx->SetErrorMessage("Unable to build query, no values supplied");
            csm_free_struct_ptr(INPUT_STRUCT, input);

            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
            return false;
        }

        stmt.append("ORDER BY run_id ASC NULLS LAST ");

        add_param_sql( stmt, input->limit > 0, ++paramCount, "LIMIT $", "::int ")
        add_param_sql( stmt, input->offset > 0, ++paramCount, "OFFSET $", "::int ")

        // ========================================================================
        
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );

        if(input->allocation_ids_count > 0) 
            dbReq->AddNumericArrayParam<int64_t>( 
                input->allocation_ids, input->allocation_ids_count );

        if(input->begin_time_search_begin[0])
            dbReq->AddTextParam(input->begin_time_search_begin );

        if(input->begin_time_search_end[0]) 
            dbReq->AddTextParam(input->begin_time_search_end);

        if(rasInserted) dbReq->AddCharacterParam(rasInsertedTrue);

        if(input->run_ids_count > 0)
            dbReq->AddNumericArrayParam<int64_t>( 
                input->run_ids, input->run_ids_count );

        if(input->status)
        {
            // Read the status a a bit mask.
            char status = input->status;
            std::string stat = "";

            // For the specified bit count, check to see if a bit was set.
            for( uint32_t i = 0; i < csm_enum_bit_count(csmi_diag_run_status_t); ++i  )
            {
                if ( (status >> i) & 1 )
                    stat.append(", ").append(csmi_diag_run_status_t_strs[i+1]);
            }
            // Make it an array.
            stat[0] = '{';
            stat.append("}");

            dbReq->AddTextParam( stat.c_str() );
        }
        
        // This needs to be after the other params, because it's exclusively for the history.
        if(input->end_time_search_begin[0]) 
            dbReq->AddTextParam(input->end_time_search_begin);

        if(input->end_time_search_end[0]) 
            dbReq->AddTextParam(input->end_time_search_end);

        if(input->limit > 0) dbReq->AddNumericParam<int>(input->limit);

        if(input->offset > 0) dbReq->AddNumericParam<int>(input->offset);
        // =====================================================================

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

// will return csmi defined error code
bool CSMIDiagRunQuery::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf = nullptr;
    bufLen = 0;

    uint32_t numRecords = tuples.size();

    if ( numRecords > 0 )
    {
        OUTPUT_STRUCT output;
        csm_init_struct_versioning(&output);
        output.num_runs = numRecords;
        output.runs = (RUN_STRUCT**) malloc( numRecords * sizeof(RUN_STRUCT*));

        for ( uint32_t i = 0; i < numRecords; ++i )
        {
            CreateOutputStruct( tuples[i], &output.runs[i] );
        }

        csm_serialize_struct(OUTPUT_STRUCT, &output, buf, &bufLen);
        csm_free_struct(OUTPUT_STRUCT, output);
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    return true;
}

void CSMIDiagRunQuery::CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        RUN_STRUCT **output )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";

    if ( fields->nfields != 9 ) 
    {
        *output = nullptr;
        return;
    }

    RUN_STRUCT *o = nullptr;
    csm_init_struct_ptr(RUN_STRUCT, o);

    o->run_id           = strtoll(fields->data[0], nullptr, 10);
    o->allocation_id    = strtoll(fields->data[1], nullptr, 10);
	o->begin_time       = strdup (fields->data[2]);
	o->cmd_line         = strdup (fields->data[3]);
	o->end_time         = strdup (fields->data[4]);
	o->history_time     = strdup (fields->data[5]);
	o->inserted_ras     = fields->data[6][0];
	o->log_dir          = strdup (fields->data[7]);

	strncpy(o->diag_status, fields->data[8], CSM_STATUS_MAX);
	o->diag_status[CSM_STATUS_MAX-1] = 0;

	*output = o;
	
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}

