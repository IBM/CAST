/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasMsgTypeDelete.cc

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
#include "CSMIRasMsgTypeDelete.h"
/* ## DEFINES ## */
//Used for debug prints
#define STATE_NAME "CSMIRasMsgTypeDelete:"
// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ras_msg_type_delete_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_msg_type_delete_output_t

bool CSMIRasMsgTypeDelete::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent** dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	/*Constants*/
	const char* RAS_MESSAGE_TYPE_TABLE_NAME = "csm_ras_type";
	
	/*HELPER VARIABLES*/
	
	/*Create a temporary string to use for the SQL statement for the database.*/
	std::string stmt;
	int SQLparameterCount = 0;
	//int somethingB4 = 0;
	
	/*Example sql statement*/
	/*sstm << "INSERT INTO " << TABLE_NAME << " VALUES ('n10', 2, 'my value');";*/
	
	if(bufferLength <= 0){
		/*Then we have a problem*/
		/*TODO: figure out better error handle.*/
		return false;
	}
	
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
	
	stmt.append("WITH deleted AS (DELETE FROM ");
	stmt.append(RAS_MESSAGE_TYPE_TABLE_NAME);
	stmt.append(" WHERE msg_id = ANY ($").append(std::to_string(++SQLparameterCount)).append("::text[])");
	stmt.append(" RETURNING msg_id) SELECT * FROM deleted ORDER BY msg_id ASC;");
	
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount);
	if(input->msg_ids) dbReq->AddTextArrayParam(input->msg_ids, input->msg_ids_count);
	
	*dbPayload = dbReq;
	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

	/*Print out SQL to be sent to the database.*/
	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
	
	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
	return true;
}

bool CSMIRasMsgTypeDelete::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer, 
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Enter";
    
	*stringBuffer = NULL;
    bufferLength = 0;
	
	/*If we want to return stuff*/
	/*Implement code here*/
	
	uint32_t numberOfDeletedRecords = tuples.size();
	
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Number of records deleted: " << numberOfDeletedRecords;
	
	if(numberOfDeletedRecords == 0){
        API_PARAMETER_OUTPUT_TYPE *output = nullptr;
        csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		/*The delete was not successful. */
        output->deleted_msg_ids_count = 0;
		/* Copy the struct data onto the string buffer*/
        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		/* Free the struct*/
        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
    }else if(numberOfDeletedRecords > 0){
		API_PARAMETER_OUTPUT_TYPE *output = nullptr;
        csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		/* Set the number of deleted msg ids. */
        output->deleted_msg_ids_count = numberOfDeletedRecords;
        output->deleted_msg_ids = (char**)calloc(numberOfDeletedRecords, sizeof(char*));
        /* Copy over the msg ids that were deleted. */
        for(uint32_t i = 0; i < numberOfDeletedRecords; i++)
        {
            output->deleted_msg_ids[i] = strdup(tuples[i]->data[0]);
        }

        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);

        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
	}else{
		fprintf(stderr, "CSMIRasMsgTypeDelete: CreateByteArray: ERROR: Problem? we encountered %u records. Expected >= 0.", numberOfDeletedRecords);
		LOG(csmapi, error) << STATE_NAME ":CreateByteArray: ERROR: Problem? we encountered " << numberOfDeletedRecords << " records. Expected >= 0.";
		ctx->SetErrorCode(CSMERR_DB_ERROR);
		ctx->AppendErrorMessage("CSMIRasMsgTypeDelete: CreateByteArray: ERROR: Problem? we encountered a bad number of records? Expected >= 0. See master daemon error log for more info.");
		return false;
	}
	
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
