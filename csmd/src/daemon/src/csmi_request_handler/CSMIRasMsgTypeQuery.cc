/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasMsgTypeQuery.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
 * Author: Nick Buonarota
 * Email:  nbuonar@us.ibm.com
 */

/* ## INCLUDES ## */
/* Header for this file. */
#include "CSMIRasMsgTypeQuery.h"
/* ## DEFINES ## */
//Used for debug prints
#define STATE_NAME "CSMIRasMsgTypeQuery:"
// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_query_output_t
#define DB_RECORD_STRUCT csmi_ras_type_record_t

bool CSMIRasMsgTypeQuery::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent** dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx ) 
{
	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	/*Unpack the buffer.*/
	API_PARAMETER_INPUT_TYPE* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0 )
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength
                            << " stringBuffer = " << stringBuffer.c_str();

		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed");
		return false;
	}

    // =====================================================================
    std::string stmtParams = "";
	int SQLparameterCount = 0;

    add_param_sql( stmtParams, input->control_action[0] != '\0', ++SQLparameterCount, 
        "control_action = $","::text AND ")
    add_param_sql( stmtParams, input->msg_id[0] != '\0', ++SQLparameterCount, 
        "msg_id LIKE $","::text AND ")
    add_param_sql( stmtParams, input->message[0] != '\0', ++SQLparameterCount, 
        "message = $","::text AND ")
    add_param_sql( stmtParams, input->severity > CSM_RAS_NO_SEV && input->severity < csm_enum_max(csmi_ras_severity_t), ++SQLparameterCount, 
        "severity = $","::ras_event_severity AND ")
    
    // TODO should this fail if the parameter count is zero?
    // Replace the last 4 characters if any parameters were found.
    if ( SQLparameterCount  > 0)
    {
        int len = stmtParams.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams[i] = ' ';
    }
	// =====================================================================

	/*Open "std::string stmt"*/
	std::string stmt = 
		"SELECT "
			"msg_id, "
			"control_action, "
			"description, "
            "enabled, "
			"message, "
			"set_state, "
			"severity, "
			"threshold_count, "
			"threshold_period, "
			"visible_to_users "
		"FROM "
			"csm_ras_type "
		"WHERE (";
	stmt.append( stmtParams );
    stmt.append(") "
		"ORDER BY "
			"msg_id "
			"ASC NULLS LAST ");
    add_param_sql( stmt, input->limit > 0, ++SQLparameterCount,
        "LIMIT $", "::int ")
    add_param_sql( stmt, input->offset > 0, ++SQLparameterCount,
        "OFFSET $", "::int ")
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	if(input->control_action[0] != '\0') dbReq->AddTextParam(input->control_action);
	if(input->msg_id[0]         != '\0') dbReq->AddTextParam(input->msg_id);
	if(input->message[0]        != '\0') dbReq->AddTextParam(input->message);
	if(input->severity > CSM_RAS_NO_SEV && input->severity < csm_enum_max(csmi_ras_severity_t) ) dbReq->AddTextParam(csm_get_string_from_enum(csmi_ras_severity_t, input->severity) );
	if(input->limit > 0)                 dbReq->AddNumericParam<int>(input->limit);
	if(input->offset > 0)                dbReq->AddNumericParam<int>(input->offset);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

void CSMIRasMsgTypeQuery::CreateOutputStruct(
	csm::db::DBTuple * const & fields,
    DB_RECORD_STRUCT **output)
{
	LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";

	// Error check
	if(fields->nfields != 10){
		*output = nullptr;
		return;
	}
	
	/*Helper Variables*/
	/*For strtol and strtoll conversion*/
    char* pEnd;
	int d = 0;
	
	// convert from DB tuple results to c data structure and add node list

	DB_RECORD_STRUCT *o = nullptr;
	/* CSM API initalize and malloc function*/
    csm_init_struct_ptr(DB_RECORD_STRUCT, o);
	
	o->msg_id          = strdup(fields->data[d]);                                                            d++;
	o->control_action  = strdup(fields->data[d]);                                                            d++;
	o->description     = strdup(fields->data[d]);                                                            d++;
	o->enabled         = csm_convert_psql_bool(fields->data[d][0]);                                          d++;
	o->message         = strdup(fields->data[d]);                                                            d++;
	o->set_state       = (csmi_node_state_t)csm_get_enum_from_string(csmi_node_state_t,fields->data[d]);     d++;
	o->severity        = (csmi_ras_severity_t)csm_get_enum_from_string(csmi_ras_severity_t,fields->data[d]); d++;
	//copy record
	o->threshold_count = strtol(fields->data[d], &pEnd, 10);                                                 d++;
	//use pointer to ensure not null.
	if(pEnd == fields->data[d])
	{
		//if null then set to -1.0
		
		/* We don't want to confuse a user between an actual 0.0 value with a could not convert value (blank value in database), which defaults to 0.0 
		* This could be a major issue potentially.
		* Use strtol and pointer to have more control.
		*
		* Repeat process below
		*/
		o->threshold_count = -1.0;
	}
	o->threshold_period = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) {o->threshold_period = -1.0;} d++;
	o->visible_to_users = csm_convert_psql_bool(fields->data[d][0]);                                                    d++;
	
	*output = o;
	
	LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
	
	return;
}


bool CSMIRasMsgTypeQuery::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char** stringBuffer,
	uint32_t &bufferLength,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Enter";
	
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
		
		//Everything went well. SUCCESS!
    }	

	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";
	return true;
}
