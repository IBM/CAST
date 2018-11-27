/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasMsgTypeCreate.cc

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

/* ## INCLUDES ## */
/* Header for this file. */
#include "CSMIRasMsgTypeCreate.h"
/* ## DEFINES ## */
//Used for debug prints
#define STATE_NAME "CSMIRasMsgTypeCreate:"
// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_create_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_create_output_t

#define RAS_MESSAGE_TYPE_TABLE_NAME  "csm_ras_type"

bool CSMIRasMsgTypeCreate::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent** dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
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
	
	int SQLparameterCount = 0;
	std::string stmt = "";
	
	if( input->set_state && input->set_state != CSM_NODE_NO_DEF && input->set_state < csm_enum_max(csmi_node_state_t) )
	{
		//there is a state to set 
		SQLparameterCount = 10;
		
		/*Open "std::string stmt"*/
		stmt = "WITH inserted AS ("
			"INSERT "
				"INTO " RAS_MESSAGE_TYPE_TABLE_NAME "( "
					"msg_id, "
					"severity, "
					"message, "
					"description, "
					"control_action, "
					"threshold_count, "
					"threshold_period, "
					"enabled, "
					"visible_to_users, "
					"set_state "
				") VALUES ("
					"$1::text, "
					"$2::ras_event_severity, "
					"$3::text, "
					"$4::text, "
					"$5::text, "
					"$6::int, "
					"$7::int, "
					"$8::boolean, "
					"$9::boolean, "
					"$10::compute_node_states "
				") RETURNING msg_id"
			") SELECT * FROM inserted ORDER BY msg_id ASC;";
		/*Close "std::string stmt"*/
		
	}else if (input->set_state == CSM_NODE_NO_DEF)
	{
		//if someone passed in NO_DEF then we want to set the db field to NULL
		SQLparameterCount = 9;
		
		/*Open "std::string stmt"*/
		stmt = "WITH inserted AS ("
			"INSERT "
				"INTO " RAS_MESSAGE_TYPE_TABLE_NAME "( "
					"msg_id, "
					"severity, "
					"message, "
					"description, "
					"control_action, "
					"threshold_count, "
					"threshold_period, "
					"enabled, "
					"visible_to_users "
				") VALUES ("
					"$1::text, "
					"$2::ras_event_severity, "
					"$3::text, "
					"$4::text, "
					"$5::text, "
					"$6::int, "
					"$7::int, "
					"$8::boolean, "
					"$9::boolean "
				") RETURNING msg_id"
			") SELECT * FROM inserted ORDER BY msg_id ASC;";
		/*Close "std::string stmt"*/

	}else{
		SQLparameterCount = 9;
		
		/*Open "std::string stmt"*/
		stmt = "WITH inserted AS ("
			"INSERT "
				"INTO " RAS_MESSAGE_TYPE_TABLE_NAME "( "
					"msg_id, "
					"severity, "
					"message, "
					"description, "
					"control_action, "
					"threshold_count, "
					"threshold_period, "
					"enabled, "
					"visible_to_users "
				") VALUES ("
					"$1::text, "
					"$2::ras_event_severity, "
					"$3::text, "
					"$4::text, "
					"$5::text, "
					"$6::int, "
					"$7::int, "
					"$8::boolean, "
					"$9::boolean "
				") RETURNING msg_id"
			") SELECT * FROM inserted ORDER BY msg_id ASC;";
		/*Close "std::string stmt"*/
	}
	
	
	
	// Build the parameterized list.
	
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddTextParam(input->msg_id);
	dbReq->AddTextParam(csm_get_string_from_enum(csmi_ras_severity_t, input->severity) );
	dbReq->AddTextParam(input->message);
	dbReq->AddTextParam(input->description);
	dbReq->AddTextParam(input->control_action);
	dbReq->AddNumericParam(input->threshold_count);
	dbReq->AddNumericParam(input->threshold_period);
	dbReq->AddCharacterParam(input->enabled == CSM_TRUE);
	dbReq->AddCharacterParam(input->visible_to_users == CSM_TRUE);
	if( input->set_state && input->set_state != CSM_NODE_NO_DEF && input->set_state < csm_enum_max(csmi_node_state_t) )
	{
		dbReq->AddTextParam( csm_get_string_from_enum(csmi_node_state_t, input->set_state));
	}else if (input->set_state == CSM_NODE_NO_DEF)
	{
		//if someone passed in NO_DEF then we want to set the db field to NULL
		// so don't add anything
	}else{
		//default to NULL
		// so don't add anything
	}
            

	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
	
	return true;
}

bool CSMIRasMsgTypeCreate::CreateByteArray(
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
        output->insert_successful = CSM_FALSE;
		/* Copy the struct data onto the string buffer*/
        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		/* Free the struct*/
        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
    }else if(numberOfInsertedRecords == 1){
		API_PARAMETER_OUTPUT_TYPE *output = nullptr;
        csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		/*The insert was not successful. */
        output->insert_successful = CSM_TRUE;
		LOG(csmapi, debug) << STATE_NAME ":CreateByteArray: tuples[0]->data[0]: " << tuples[0]->data[0];
		output->msg_id = strdup(tuples[0]->data[0]);
		/* Copy the struct data onto the string buffer*/
        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		/* Free the struct*/
        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
	}else{
		fprintf(stderr, "CSMIRasMsgTypeCreate: CreateByteArray: ERROR: Problem? we encountered %u records. Expected either 0 or 1.", numberOfInsertedRecords);
		LOG(csmapi, error) << STATE_NAME ":CreateByteArray: ERROR: Problem? we encountered " << numberOfInsertedRecords << " records. Expected either 0 or 1.";
		ctx->SetErrorCode(CSMERR_DB_ERROR);
		ctx->AppendErrorMessage("CSMIRasMsgTypeCreate: CreateByteArray: ERROR: Problem? we encountered a bad number of records? Expected either 0 or 1. See master daemon error log for more info.");
		return false;
	}
	
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
