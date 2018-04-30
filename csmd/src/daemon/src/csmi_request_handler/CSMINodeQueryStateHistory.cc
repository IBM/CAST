/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMINodeQueryStateHistory.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota, John Dunham
* Email: nbuonar@us.ibm.com, jdunham@us.ibm.com
*/

/* Header for this file. */
#include "CSMINodeQueryStateHistory.h"

//Used for debug prints
#define STATE_NAME "CSMINodeQueryStateHistory:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_node_query_state_history_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_query_state_history_output_t
#define DB_RECORD_STRUCT csmi_node_query_state_history_record_t 

bool CSMINodeQueryStateHistory::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    /*Unpack the buffer.*/
	API_PARAMETER_INPUT_TYPE* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0 )
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed...";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength << " stringBuffer = " << stringBuffer.c_str();
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		//append to the err msg as to preserve other previous messages.
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		return false;
	}
	
	int SQLparameterCount = 1;

	/*Open "std::string stmt"*/
	std::string stmt = 
		"SELECT "
			"n.node_name, n.history_time, n.state, r.rec_id, r.msg_id " 
		"FROM "
			"csm_node_state_history AS n "
		"FULL OUTER JOIN "
			"csm_ras_event_action AS r "
				"ON ( "
					"n.history_time = r.master_time_stamp AND "
					"n.node_name = r.location_name "
				") "
		"WHERE ("
			"n.node_name = $1::text" 
			")"
		"ORDER BY "
			"node_name, ";
			if(input->order_by == 'a'){
				stmt.append("history_time ASC NULLS LAST ");
			}else if(input->order_by == 'd'){
				stmt.append("history_time DESC NULLS LAST ");
			}else{
				//default case
				stmt.append("history_time ASC NULLS LAST ");
			}
		add_param_sql( stmt, input->limit > 0, ++SQLparameterCount,
            "LIMIT $", "::int ")

		add_param_sql( stmt, input->offset > 0, ++SQLparameterCount,
            "OFFSET $", "::int ")
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddTextParam(input->node_name);
	if(input->limit > 0) dbReq->AddNumericParam<int>(input->limit);
	if(input->offset > 0) dbReq->AddNumericParam<int>(input->offset);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMINodeQueryStateHistory::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

	*stringBuffer = NULL;
    bufferLength = 0;
	
	/*If we want to return stuff*/
	/*Implement code here*/

	/*Helper Variables*/
	uint32_t numberOfRecords = tuples.size();

	if(numberOfRecords > 0){
		/*Our SQL query found at least one matching record.*/
		
        /* Prepare the data to be returned. */
		API_PARAMETER_OUTPUT_TYPE* output = NULL;
		csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		/* Copy the node_name. */
		output->node_name = strdup(tuples[0]->data[0]);
		/* Say how many results there are. */
		output->results_count = numberOfRecords;
		/* Create space for each result. */
		output->results = (DB_RECORD_STRUCT**)calloc(output->results_count, sizeof(DB_RECORD_STRUCT*));
		
		/* Build the individual records for packing. */
		for (uint32_t i = 0; i < numberOfRecords; i++){
			CreateOutputStruct(tuples[i], &(output->results[i]));
		}
		
		// Pack the allocation up.
		csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		
		// Free struct we made.
		csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
    }
    
    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

void CSMINodeQueryStateHistory::CreateOutputStruct(
    csm::db::DBTuple * const & fields, 
    DB_RECORD_STRUCT **output )
{
    LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";
	
	// Error check
    if(fields->nfields != 5) 
    {
        *output = nullptr;
        return;
    }

    DB_RECORD_STRUCT *o = nullptr;
    csm_init_struct_ptr(DB_RECORD_STRUCT, o);
	
	int i = 1;
	
	o->history_time            = strdup(fields->data[i]);                                                        i++;
    o->state                   = (csmi_node_state_t)csm_get_enum_from_string(csmi_node_state_t,fields->data[i]); i++;
	
	if(o->state == CSM_NODE_SOFT_FAILURE){
		o->alteration = CSM_NODE_ALTERATION_RAS_EVENT;
		o->ras_rec_id = strdup(fields->data[i]); i++;
		o->ras_msg_id = strdup(fields->data[i]); i++;
	}else if(o->state == CSM_NODE_DISCOVERED){
		o->alteration = CSM_NODE_ALTERATION_CSM_INVENTORY;
	}else{
		o->alteration = CSM_NODE_ALTERATION_CSM_API;
	}
	
	*output = o;
	
    LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
}
