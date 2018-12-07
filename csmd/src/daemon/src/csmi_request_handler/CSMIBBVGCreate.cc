/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBVGCreate.cc

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
#include "CSMIBBVGCreate.h"

#define STATE_NAME "CSMIBBVGCreate:"
// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_bb_vg_create_input_t
#define API_PARAMETER_OUTPUT_TYPE 

bool CSMIBBVGCreate::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
    // Unpack the buffer.
	API_PARAMETER_INPUT_TYPE* input = NULL;

	// Error in case something went wrong with the unpack
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0 )
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed...";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength << " stringBuffer = " << stringBuffer.c_str();
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		//append to the err msg as to preserve other previous messages.
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		//release memory using CSM API function
		csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
		return false;
	}
	
	/* Rip the ssd info into arrays for db function */
	char** ssd_serial_numbers = NULL;
	int64_t* ssd_allocations = NULL;
	ssd_serial_numbers = (char**)calloc(input->ssd_info_count, sizeof(char*));
	ssd_allocations = (int64_t*)calloc(input->ssd_info_count, sizeof(int64_t));

	for(uint32_t i = 0; i < input->ssd_info_count; i++){
		ssd_serial_numbers[i] = strdup(input->ssd_info[i]->ssd_serial_number);
		ssd_allocations[i] = input->ssd_info[i]->ssd_allocation;
	}
	
	const int SQLparameterCount = 8;
	
	/* Call special db function */
	std::string stmt = 
		"SELECT fn_csm_vg_create("
			"$1::bigint, "
			"$2::text, "
			"$3::int, "
			"$4::text[], "
			"$5::bigint[], "
			"$6::bigint, "
			"$7::text, "
			"$8::boolean)";
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, SQLparameterCount );
	dbReq->AddNumericParam<int64_t>(input->available_size );
	dbReq->AddTextParam(input->node_name );
	dbReq->AddNumericParam<int32_t>(input->ssd_info_count );
	dbReq->AddTextArrayParam(ssd_serial_numbers, input->ssd_info_count);
	dbReq->AddNumericArrayParam<int64_t>(ssd_allocations, input->ssd_info_count);
	dbReq->AddNumericParam<int64_t>(input->total_size );
	dbReq->AddTextParam(input->vg_name );
	dbReq->AddCharacterParam(input->scheduler);
	
	*dbPayload = dbReq;
	
	//Free memory we allocated
	for(uint32_t i = 0; i < input->ssd_info_count; i++){
		free(ssd_serial_numbers[i]);
	}
	free(ssd_serial_numbers);
	free(ssd_allocations);
	
	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMIBBVGCreate::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf   = NULL;
    bufLen = 0;

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

