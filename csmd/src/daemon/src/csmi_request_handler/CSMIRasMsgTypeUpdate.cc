/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasMsgTypeUpdate.cc

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
#include "CSMIRasMsgTypeUpdate.h"
/* ## DEFINES ## */
//Used for debug prints
#define STATE_NAME "CSMIRasMsgTypeUpdate:"
// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_update_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_update_output_t

bool CSMIRasMsgTypeUpdate::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent** dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	/*Constants*/
	const char* RAS_MESSAGE_TYPE_TABLE_NAME = "csm_ras_type";
	
	/*HELPER VARIABLES*/
	/*Create a temporary string to use for the SQL statement for the database.*/
	std::string stmt;
	int SQLparameterCount = 0;
	bool SQL_parameter_override = false;
	//int somethingB4 = 0;
	
	/*Example sql statement*/
	/*sstm << "INSERT INTO " << TABLE_NAME << " VALUES ('n10', 2, 'my value');";*/
	
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
	
	/*Open "std::string stmt"*/
	stmt.append("WITH updated AS ("
		"UPDATE ");
			stmt.append(RAS_MESSAGE_TYPE_TABLE_NAME); 
		stmt.append(" SET");
			if(input->control_action[0])        stmt.append( " control_action = $").append(std::to_string(++SQLparameterCount)).append("::text,");
			if(input->description[0])           stmt.append( " description = $").append(std::to_string(++SQLparameterCount)).append("::text,");
			if(input->enabled < CSM_UNDEF_BOOL)          stmt.append( " enabled = $").append(std::to_string(++SQLparameterCount)).append("::boolean,");
			if(input->message[0])               stmt.append( " message = $").append(std::to_string(++SQLparameterCount)).append("::text,");
			//if(input->set_not_ready < CSM_UNDEF_BOOL)    stmt.append( " set_not_ready = $").append(std::to_string(++SQLparameterCount)).append("::boolean,");
			//if(input->set_ready < CSM_UNDEF_BOOL)        stmt.append( " set_ready = $").append(std::to_string(++SQLparameterCount)).append("::boolean,");
			if(input->severity > CSM_RAS_NO_SEV && input->severity < csm_enum_max(csmi_ras_severity_t))
			{
				stmt.append( " severity = $").append(std::to_string(++SQLparameterCount)).append("::ras_event_severity,");
			}				
			if(input->set_state > CSM_NODE_NO_DEF && input->set_state < csm_enum_max(csmi_node_state_t))
			{
				if(input->set_state == CSM_NODE_DATABASE_NULL){
					stmt.append( " set_state = NULL::compute_node_states,");
					SQL_parameter_override = true;
				}else{
					stmt.append( " set_state = $").append(std::to_string(++SQLparameterCount)).append("::compute_node_states,");
				}
			}
			
			if(input->threshold_count >= 0)     stmt.append( " threshold_count = $").append(std::to_string(++SQLparameterCount)).append("::int,");
			if(input->threshold_period >= 0)    stmt.append( " threshold_period = $").append(std::to_string(++SQLparameterCount)).append("::int,");
			if(input->visible_to_users < CSM_UNDEF_BOOL) stmt.append( " visible_to_users = $").append(std::to_string(++SQLparameterCount)).append("::boolean,");
			
			// Verify the payload.
			if(SQLparameterCount == 0 && SQL_parameter_override == false)
			{
				LOG(csmapi, error) << STATE_NAME ":CreatePayload: No values supplied";
				ctx->SetErrorCode(CSMERR_MISSING_PARAM);
				ctx->SetErrorMessage("Unable to build update query, no values supplied");
				csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

				LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
				return false;
			}
			
			// Remove the last comma.
			stmt.back()= ' ';

			stmt.append(" WHERE msg_id = $").append(std::to_string(++SQLparameterCount)).append(
				"::text RETURNING msg_id) "
				"SELECT * FROM updated ORDER BY msg_id ASC;"); 
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	if(input->control_action[0])        dbReq->AddTextParam(input->control_action);
	if(input->description[0])           dbReq->AddTextParam(input->description);
	if(input->enabled < CSM_UNDEF_BOOL)          dbReq->AddCharacterParam(input->enabled );
	if(input->message[0])               dbReq->AddTextParam(input->message);
	//if(input->set_not_ready < CSM_UNDEF_BOOL)    dbReq->AddCharacterParam(input->set_not_ready );
	//if(input->set_ready < CSM_UNDEF_BOOL)        dbReq->AddCharacterParam(input->set_ready );
	if(input->severity > CSM_RAS_NO_SEV && input->severity < csm_enum_max(csmi_ras_severity_t) )
	{
		dbReq->AddTextParam(csm_get_string_from_enum(csmi_ras_severity_t, input->severity) );
	}		
	if(input->set_state > CSM_NODE_NO_DEF && input->set_state < csm_enum_max(csmi_node_state_t) )
	{
		if(input->set_state == CSM_NODE_DATABASE_NULL){
			//we already added NULL above in the SQL 
		}else{
			dbReq->AddTextParam( csm_get_string_from_enum(csmi_node_state_t, input->set_state));
		}
	}
	if(input->threshold_count >= 0)     dbReq->AddNumericParam(input->threshold_count);
	if(input->threshold_period >= 0)    dbReq->AddNumericParam(input->threshold_period);
	if(input->visible_to_users < CSM_UNDEF_BOOL) dbReq->AddCharacterParam(input->visible_to_users );
	
	
	dbReq->AddTextParam(input->msg_id);

	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
	
	return true;
}

bool CSMIRasMsgTypeUpdate::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char** stringBuffer,
	uint32_t &bufferLength,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: ENTER";
	
    *stringBuffer = NULL;
    bufferLength = 0;

    uint32_t numberOfInsertedRecords = tuples.size();

    if(numberOfInsertedRecords == 0)
    {
        API_PARAMETER_OUTPUT_TYPE *output = nullptr;
        csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		/*The insert was not successful. */
        output->update_successful = CSM_FALSE;
		/* Copy the struct data onto the string buffer*/
        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		/* Free the struct*/
        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
    }else if(numberOfInsertedRecords == 1){
		API_PARAMETER_OUTPUT_TYPE *output = nullptr;
        csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		/*The insert was not successful. */
        output->update_successful = CSM_TRUE;
		LOG(csmapi, debug) << STATE_NAME ":CreateByteArray: tuples[0]->data[0]: " << tuples[0]->data[0];
		output->msg_id = strdup(tuples[0]->data[0]);
		/* Copy the struct data onto the string buffer*/
        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		/* Free the struct*/
        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
	}else{
		fprintf(stderr, "CSMIRasMsgTypeUpdate: CreateByteArray: ERROR: Problem? we encountered %u records. Expected either 0 or 1.", numberOfInsertedRecords);
		LOG(csmapi, error) << STATE_NAME ":CreateByteArray: ERROR: Problem? we encountered " << numberOfInsertedRecords << " records. Expected either 0 or 1.";
		ctx->SetErrorCode(CSMERR_DB_ERROR);
		ctx->AppendErrorMessage("CSMIRasMsgTypeUpdate: CreateByteArray: ERROR: Problem? we encountered a bad number of records? Expected either 0 or 1. See master daemon error log for more info.");
		return false;
	}
	
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
