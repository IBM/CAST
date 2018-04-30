/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIIbCableUpdate.cc

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
#include "CSMIIbCableUpdate.h"

#define STATE_NAME "CSMIIbCableUpdate:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ib_cable_update_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ib_cable_update_output_t

bool CSMIIbCableUpdate::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	// Unpack the buffer.
    API_PARAMETER_INPUT_TYPE* input = nullptr;

	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0 )
    {
		LOG(csmapi, error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed...";
		LOG(csmapi, error) << "  bufferLength = " << bufferLength << " stringBuffer = " << stringBuffer.c_str();
		LOG(csmapi, error) << STATE_NAME ":CreatePayload: Exit";
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		//append to the err msg as to preserve other previous messages.
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		return false;
	}

	int SQLparameterCount = 0;

	std::string stmt = "WITH updated AS ( UPDATE csm_ib_cable SET ";
    
    add_param_sql( stmt, input->comment[0], ++SQLparameterCount, "comment=$",   "::text,")
    add_param_sql( stmt, input->guid_s1[0], ++SQLparameterCount, "guid_s1=$",   "::text,")
    add_param_sql( stmt, input->guid_s2[0], ++SQLparameterCount, "guid_s2=$",   "::text,")
    add_param_sql( stmt, input->port_s1[0], ++SQLparameterCount, "port_s1=$",   "::text,")
    add_param_sql( stmt, input->port_s2[0], ++SQLparameterCount, "port_s2=$",   "::text,")
    
    // Verify the payload.
    if ( SQLparameterCount >  0 )
    {
        // Remove the last comma.
        stmt.back()= ' ';
    }
    else
    {
        LOG(csmapi, error) << STATE_NAME ":CreatePayload: No values supplied";
        ctx->SetErrorCode(CSMERR_MISSING_PARAM);
        ctx->SetErrorMessage("Unable to build update query, no values supplied");
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
    
        LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
        return false;
    }

    // Build the array parameter.
    std::string array_param = "$";
    array_param.append(std::to_string(++SQLparameterCount)).append("::text[]");

    stmt.append("WHERE serial_number=ANY(").append(array_param).append(") RETURNING serial_number )"
        "SELECT sn FROM unnest (").append(array_param).append( ") as input(sn) "
        "LEFT JOIN updated ON (updated.serial_number=input.sn) "
        "WHERE updated.serial_number IS NULL");
	
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount);
	if(input->comment[0] != '\0'){ dbReq->AddTextParam(input->comment);}
	if(input->guid_s1[0] != '\0'){ dbReq->AddTextParam(input->guid_s1);}
	if(input->guid_s2[0] != '\0'){ dbReq->AddTextParam(input->guid_s2);}
	if(input->port_s1[0] != '\0'){ dbReq->AddTextParam(input->port_s1);}
	if(input->port_s2[0] != '\0'){ dbReq->AddTextParam(input->port_s2);}
	dbReq->AddTextArrayParam(input->serial_numbers, input->serial_numbers_count);
	
	*dbPayload = dbReq;

	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
         
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";
         
    return true;
}

bool CSMIIbCableUpdate::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Enter";

    *stringBuffer = nullptr;
    bufferLength = 0;

    uint32_t numberOfRecords = tuples.size();

    API_PARAMETER_OUTPUT_TYPE* output = nullptr;
    csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);

    output->failure_count = numberOfRecords;

    if(numberOfRecords > 0)
    {
        output->failure_ib_cables = (char**)malloc(numberOfRecords * sizeof(char*));
        
        for(uint32_t i = 0; i < numberOfRecords; ++i )
        {
            output->failure_ib_cables[i] = strdup(tuples[i]->data[0]);
        }
    }

    csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
    csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);

    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
