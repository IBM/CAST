/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationResourcesQuery.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: John Dunham
* Email: jdunham@us.ibm.com
*/

#include "CSMIAllocationResourcesQuery.h"

//Used for debug prints
#define STATE_NAME "CSMIAllcoationResourcesQuery:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT  csm_allocation_resources_query_input_t
#define RECORD_STRUCT csmi_allocation_resources_record_t
#define OUTPUT_STRUCT csm_allocation_resources_query_output_t

bool CSMIAllocationResourcesQuery::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	/*Unpack the buffer.*/
	INPUT_STRUCT* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if ( csm_deserialize_struct(INPUT_STRUCT, &input, stringBuffer.c_str(), bufferLength) == 0 )
    {
        // Initialize to 1 since allocation_id is static.
        int paramCount = 1;

        std::string stmt = "SELECT "
                "n.node_name,n.state "
            "FROM csm_node n "
            "LEFT JOIN csm_allocation_node a "
            "ON a.node_name = n.node_name "
            "WHERE a.allocation_id=$1::bigint "
            "ORDER BY n.node_name ";

        add_param_sql( stmt, input->limit > 0, ++paramCount, "LIMIT $", "::int ")
        add_param_sql( stmt, input->offset > 0, ++paramCount, "OFFSET $", "::int ")


        //==========================================================================
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>(input->allocation_id);
        if(input->limit > 0) dbReq->AddNumericParam<int>(input->limit);
        if(input->offset > 0) dbReq->AddNumericParam<int>(input->offset);

        *dbPayload = dbReq;
        csm_free_struct_ptr(INPUT_STRUCT, input);
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

bool CSMIAllocationResourcesQuery::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf = NULL;
    bufLen = 0;
	
	uint32_t numRecords = tuples.size();

    if(numRecords > 0)
    {
		OUTPUT_STRUCT* output = NULL;
		csm_init_struct_ptr(OUTPUT_STRUCT, output);
		
		output->results_count = numRecords;
	
		output->results = (RECORD_STRUCT **)malloc(numRecords * sizeof(RECORD_STRUCT*));
		
		for ( uint32_t i = 0; i < numRecords; i++ )
        {
			CreateOutputStruct(tuples[i], &(output->results[i]));
		}
		
		csm_serialize_struct(OUTPUT_STRUCT, output, buf, &bufLen);
		csm_free_struct_ptr(OUTPUT_STRUCT, output);
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

void CSMIAllocationResourcesQuery::CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        RECORD_STRUCT **output )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";

	// Error check
    if ( fields->nfields != 2 )
    {
        *output = nullptr;
        return;
    }
	
	RECORD_STRUCT* o = nullptr;
	csm_init_struct_ptr(RECORD_STRUCT, o);

	o->node_name = strdup(fields->data[0]);

    // Convert the state
    csmi_node_state_t state = (csmi_node_state_t)csm_get_enum_from_string( 
        csmi_node_state_t, fields->data[1]);
	o->ready     = state==CSM_NODE_IN_SERVICE; // FIXME This needs to be a function somewhere!

	*output = o;

    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}

