/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBVGQuery.cc

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
#include "CSMIBBVGQuery.h"

#define STATE_NAME "CSMIBBVGQuery:"
#define INPUT_STRUCT csm_bb_vg_query_input_t
#define OUTPUT_STRUCT csm_bb_vg_query_output_t
     
bool CSMIBBVGQuery::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT *input = nullptr;
    
    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0) 
    {
        int SQLparameterCount = 0; 
        
        // Looks like we 
        std::string stmt = 
			"SELECT "
                "vg_name, available_size, node_name, total_size, scheduler "
            "FROM csm_vg "
            "WHERE ";

        add_param_sql( stmt, input->vg_names_count > 0, ++SQLparameterCount,
            "vg_name = ANY( $","::text[] ) AND ");
			
		add_param_sql( stmt, input->node_names_count > 0, ++SQLparameterCount,
            "node_name = ANY( $","::text[] ) AND ");
			
		if( SQLparameterCount > 0 )
        {
            // Replace the last 4 characters if any parameters were found.
            // Remove the AND.
            int len = stmt.length() - 1;
            for( int i = len - 3; i < len; ++i)
                stmt[i] = ' ';
		}
                
        stmt.append( "ORDER BY vg_name, node_name ASC NULLS LAST " );

        if( SQLparameterCount == 0 )
        {
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: No values supplied";
            ctx->SetErrorCode(CSMERR_MISSING_PARAM);
            ctx->SetErrorMessage("Unable to build query, no values supplied");
            csm_free_struct_ptr(INPUT_STRUCT, input);

            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
            return false;
        }

        add_param_sql( stmt, input->limit > 0, ++SQLparameterCount, "LIMIT $", "::int " );
        add_param_sql( stmt, input->offset > 0, ++SQLparameterCount, "OFFSET $", "::int " );
        
        // =====================================================================
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, SQLparameterCount ); 
        if(input->vg_names_count > 0) dbReq->AddTextArrayParam(input->vg_names, input->vg_names_count );
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

bool CSMIBBVGQuery::CreateByteArray(
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

        output->results = (csmi_vg_record_t**)malloc( numRecords * sizeof(csmi_vg_record_t*) );

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

void CSMIBBVGQuery::CreateOutputStruct(
    csm::db::DBTuple * const & fields, 
    csmi_vg_record_t **output)
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";
	
    if ( fields->nfields != 5) 
    {
        *output = nullptr;
        return;
    }
	
	/*Helper Variables*/
	char* pEnd; // comparison pointer for data conversion check.
	int i = 0; //keep place in data counter

	csmi_vg_record_t *o = nullptr;
    csm_init_struct_ptr(csmi_vg_record_t, o);
	
	o->vg_name        = strdup (fields->data[i]);                                                                    i++;
	o->available_size = strtoll(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->available_size = -1.0;} i++;
	o->node_name      = strdup (fields->data[i]);                                                                    i++;
	o->total_size     = strtoll(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->total_size = -1.0;}     i++;
	o->scheduler      = csm_convert_psql_bool(fields->data[i][0]);                                                   i++;
	
	*output = o;

    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}

