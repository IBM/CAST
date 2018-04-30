/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBLVQuery.cc

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
#include "CSMIBBLVQuery.h"

#define STATE_NAME "CSMIBBLVQuery:"

#define INPUT_STRUCT csm_bb_lv_query_input_t
#define OUTPUT_STRUCT csm_bb_lv_query_output_t

bool CSMIBBLVQuery::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT *input = nullptr;
    
    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        int paramCount = 0; 

        std::string stmt = "SELECT "
                "logical_volume_name, node_name, allocation_id, "
                "vg_name, state, current_size, " 
                "max_size, begin_time, updated_time, "
                "file_system_mount, file_system_type "
            "FROM csm_lv " 
            "WHERE ";

        add_param_sql( stmt, input->allocation_ids_count > 0, ++paramCount,
            "allocation_id = ANY( $","::bigint[] ) AND ");
		add_param_sql( stmt, input->logical_volume_names_count > 0, ++paramCount,
            "logical_volume_name = ANY( $","::text[] ) AND ");
		add_param_sql( stmt, input->node_names_count > 0, ++paramCount,
            "node_name = ANY( $","::text[] ) AND ");
			
		if( paramCount > 0 )
        {
            // Replace the last 4 characters if any parameters were found.
            // Remove the AND.
            int len = stmt.length() - 1;
            for( int i = len - 3; i < len; ++i)
                stmt[i] = ' ';
		}
                
        stmt.append( "ORDER BY logical_volume_name, node_name, allocation_id ASC NULLS LAST " );

        if( paramCount == 0 )
        {
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: No values supplied";
            ctx->SetErrorCode(CSMERR_MISSING_PARAM);
            ctx->SetErrorMessage("Unable to build query, no values supplied");
            csm_free_struct_ptr(INPUT_STRUCT, input);

            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
            return false;
        }

        add_param_sql( stmt, input->limit > 0, ++paramCount, "LIMIT $", "::int ")
		add_param_sql( stmt, input->offset > 0, ++paramCount, "OFFSET $", "::int ")
        
        // =====================================================================
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount ); 
		if(input->allocation_ids_count > 0) dbReq->AddNumericArrayParam<int64_t>(input->allocation_ids, input->allocation_ids_count );
		if(input->logical_volume_names_count > 0) dbReq->AddTextArrayParam(input->logical_volume_names, input->logical_volume_names_count );
        if(input->node_names_count > 0) dbReq->AddTextArrayParam(input->node_names, input->node_names_count );

        if(input->limit > 0) dbReq->AddNumericParam<int>(input->limit);
        if(input->offset > 0) dbReq->AddNumericParam<int>(input->offset);
        
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

bool CSMIBBLVQuery::CreateByteArray(
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
        OUTPUT_STRUCT *output = nullptr;
        csm_init_struct_ptr( OUTPUT_STRUCT, output );

        output->results_count = numRecords;

        output->results = (csmi_lv_record_t**)malloc( numRecords * sizeof(csmi_lv_record_t*) );

        for ( uint32_t i = 0; i < numRecords; ++i )
        {
            CreateOutputStruct( tuples[i], &(output->results[i]) );
        }

        csm_serialize_struct( OUTPUT_STRUCT, output, buf, &bufLen );
        csm_free_struct_ptr( OUTPUT_STRUCT, output );
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    return true;
}
	
void CSMIBBLVQuery::CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        csmi_lv_record_t **output )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";

    if ( fields->nfields != 11 ) 
    {
        *output = nullptr;
        return;
    }

    csmi_lv_record_t  *o = nullptr;
    csm_init_struct_ptr(csmi_lv_record_t , o);

	o->logical_volume_name = strdup(fields->data[0]);
	o->node_name           = strdup(fields->data[1]);
    o->allocation_id       = strtoll(fields->data[2], nullptr, 10);
	o->vg_name             = strdup(fields->data[3]);
	o->state               = fields->data[4][0];
    o->current_size        = strtoll(fields->data[5], nullptr, 10);
    o->max_size            = strtoll(fields->data[6], nullptr, 10);
	o->begin_time          = strdup(fields->data[7]);
	o->updated_time        = strdup(fields->data[8]);
	o->file_system_mount   = strdup(fields->data[9]);
	o->file_system_type    = strdup(fields->data[10]);

	*output = o;
	
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}


