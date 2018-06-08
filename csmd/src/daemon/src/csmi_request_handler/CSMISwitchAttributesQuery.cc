/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMISwitchAttributesQuery.cc

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
#include "CSMISwitchAttributesQuery.h"

//Used for debug prints
#define STATE_NAME "CSMISwitchAttributesQuery:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_switch_attributes_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_switch_attributes_query_output_t
#define DB_RECORD_STRUCT csmi_switch_record_t 

bool CSMISwitchAttributesQuery::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
    // Unpack the buffer.
    API_PARAMETER_INPUT_TYPE* input = nullptr;
	
	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0)
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed...";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength << " stringBuffer = " 
            << stringBuffer.c_str();
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		//append to the err msg as to preserve other previous messages.
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		return false;
	}
	
	// =====================================================================
	std::string stmtParams = "";
	int SQLparameterCount = 0;
	
	//where statement parameters
	add_param_sql(stmtParams, input->switch_names_count > 0, ++SQLparameterCount, "switch_name = ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->state[0] != '\0', ++SQLparameterCount, "state = $","::text AND ")
	add_param_sql(stmtParams, input->serial_number[0] != '\0', ++SQLparameterCount, "serial_number = $","::text AND ")
	
    // Replace the last 4 characters if any parameters were found.
    if ( SQLparameterCount > 0)
    {
        int len = stmtParams.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams[i] = ' ';
    }
	
	std::string stmt = "SELECT "
			"switch_name, "
			"serial_number, "
			"discovery_time, "
			"collection_time, "
			"comment, "
			"description, "
			"fw_version, "
			"gu_id, "
			"has_ufm_agent, "
			"hw_version, "
			"ip, "
			"model, "
			"num_modules, "
			"physical_frame_location, " 
			"physical_u_location, "
			"ps_id, "
			"role, "
			"server_operation_mode, "
			"sm_mode, "
			"state, "
			"sw_version, "
			"system_guid, "
			"system_name, "
			"total_alarms, "
			"type, "
			"vendor "
		"FROM csm_switch ";
	if(SQLparameterCount > 0)
	{
		//Filters have been provided. 
		//Filter query pased off of input.
		stmt.append("WHERE (");
		stmt.append( stmtParams );
		stmt.append(") ");
	}
	stmt.append("ORDER BY ");
	switch (input->order_by)
	{
		case 'a':
			stmt.append("switch_name ASC NULLS LAST ");
			break;
		case 'b':
			stmt.append("switch_name DESC NULLS LAST ");
			break;
		default:
			stmt.append("switch_name ASC NULLS LAST ");
	}
	add_param_sql( stmt, input->limit > 0, ++SQLparameterCount, "LIMIT $", "::int ")
	add_param_sql( stmt, input->offset > 0, ++SQLparameterCount, "OFFSET $", "::int ")

	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, SQLparameterCount );
	if ( input->switch_names_count > 0 ) dbReq->AddTextArrayParam(input->switch_names, input->switch_names_count);
	if ( input->state[0] != '\0') dbReq->AddTextParam(input->state);
	if ( input->serial_number[0] != '\0') dbReq->AddTextParam(input->serial_number);
	if ( input->limit  > 0 ) dbReq->AddNumericParam<int>(input->limit); 
	if ( input->offset > 0 ) dbReq->AddNumericParam<int>(input->offset);

	*dbPayload = dbReq;
	csm_free_struct_ptr( API_PARAMETER_INPUT_TYPE, input );
	
	LOG( csmapi, debug ) << STATE_NAME ":SQL: " << stmt;
    
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMISwitchAttributesQuery::CreateByteArray(
		const std::vector<csm::db::DBTuple *>&tuples, 
		char **stringBuffer, 
		uint32_t &bufferLength, 
		csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    // Init the buffer
    *stringBuffer = nullptr;
    bufferLength = 0;

    /*If we want to return stuff*/
	/*Implement code here*/
	
	/*Helper Variables*/
	uint32_t numberOfRecords = tuples.size();

    if(numberOfRecords > 0){
		/*Our SQL query found at least one matching record.*/
		
		/* Prepare the data to be returned. */
        csm_switch_attributes_query_output_t* output = NULL;
        csm_init_struct_ptr(csm_switch_attributes_query_output_t, output);
        /* Say how many results there are. */
        output->results_count = numberOfRecords;
		/* Create space for each result. */
        output->results = (DB_RECORD_STRUCT**)calloc(output->results_count, sizeof(DB_RECORD_STRUCT*));
        
		/* Build the individual records for packing. */
        for(uint32_t i = 0; i < numberOfRecords; i++){
            CreateOutputStruct(tuples[i], &(output->results[i]));
        }
        
		// Pack the allocation up.
        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
        
		// Free struct we made.
        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
    }    

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    
    return true;
}

void CSMISwitchAttributesQuery::CreateOutputStruct(
    csm::db::DBTuple * const & fields, 
    DB_RECORD_STRUCT **output)
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateOutputStruct: Enter";
    
	// Error check
    if ( fields->nfields != 26 )
    {
        *output = nullptr;
        return;
    }
	
	// convert from DB tuple results to c data structure
	
	/*Helper Variables*/
    char* pEnd;
	int d = 0; //data counter tracker
	
	/*Set up data to call API*/
    DB_RECORD_STRUCT *o = nullptr;
	/* CSM API initialize and malloc function*/
    csm_init_struct_ptr(DB_RECORD_STRUCT, o);
	
	o->switch_name             = strdup(fields->data[d]);                                                                d++;
	o->serial_number           = strdup(fields->data[d]);                                                                d++;
	o->discovery_time          = strdup(fields->data[d]);                                                                d++;
	o->collection_time         = strdup(fields->data[d]);                                                                d++;
	o->comment                 = strdup(fields->data[d]);                                                                d++;
	o->description             = strdup(fields->data[d]);                                                                d++;
	o->fw_version              = strdup(fields->data[d]);                                                                d++;
	o->gu_id                   = strdup(fields->data[d]);                                                                d++;
	o->has_ufm_agent           = csm_convert_psql_bool(fields->data[d][0]);                                              d++;
	o->hw_version              = strdup(fields->data[d]);                                                                d++;
	o->ip                      = strdup(fields->data[d]);                                                                d++;
	o->model                   = strdup(fields->data[d]);                                                                d++;
	o->num_modules             = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->num_modules = -1.0;  d++;
	o->physical_frame_location = strdup(fields->data[d]);                                                                d++;
	o->physical_u_location     = strdup(fields->data[d]);                                                                d++;
	o->ps_id                   = strdup(fields->data[d]);                                                                d++;
	o->role                    = strdup(fields->data[d]);                                                                d++;
	o->server_operation_mode   = strdup(fields->data[d]);                                                                d++;
	o->sm_mode                 = strdup(fields->data[d]);                                                                d++;
	o->state                   = strdup(fields->data[d]);                                                                d++;
	o->sw_version              = strdup(fields->data[d]);                                                                d++;
	o->system_guid             = strdup(fields->data[d]);                                                                d++;
	o->system_name             = strdup(fields->data[d]);                                                                d++;
	o->total_alarms            = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->total_alarms = -1.0; d++;
	o->type                    = strdup(fields->data[d]);                                                                d++;
	o->vendor                  = strdup(fields->data[d]);                                                                d++;
	
	*output = o;

    LOG( csmapi, trace ) << STATE_NAME ":CreateOutputStruct: Exit";
}
