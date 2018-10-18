/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIIbCableInventoryCollection.cc

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
#include "CSMIIbCableInventoryCollection.h"

#define STATE_NAME "CSMIIbCableInventoryCollection:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ib_cable_inventory_collection_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ib_cable_inventory_collection_output_t

bool CSMIIbCableInventoryCollection::CreatePayload(
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
		LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		//append to the err msg as to preserve other previous messages.
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		return false;
	}
	
	
	/* Rip the info into arrays for db function */
	
	//temp db arrays
	char** serial_numbers = NULL;
	char** comments = NULL;
	char** guid_s1s = NULL;
	char** guid_s2s = NULL;
	char** identifiers = NULL;
	char** lengths = NULL;
	char** names = NULL;
	char** part_numbers = NULL;
	char** port_s1s = NULL;
	char** port_s2s = NULL;
	char** revisions = NULL;
	char** severities = NULL;
	char** types = NULL;
	char** widths = NULL;
	
	serial_numbers = (char**)calloc(input->inventory_count, sizeof(char*));
	comments       = (char**)calloc(input->inventory_count, sizeof(char*));
	guid_s1s       = (char**)calloc(input->inventory_count, sizeof(char*));
	guid_s2s       = (char**)calloc(input->inventory_count, sizeof(char*));
	identifiers    = (char**)calloc(input->inventory_count, sizeof(char*));         
	lengths        = (char**)calloc(input->inventory_count, sizeof(char*));
	names          = (char**)calloc(input->inventory_count, sizeof(char*));
	part_numbers   = (char**)calloc(input->inventory_count, sizeof(char*));
	port_s1s       = (char**)calloc(input->inventory_count, sizeof(char*));
	port_s2s       = (char**)calloc(input->inventory_count, sizeof(char*));
	revisions      = (char**)calloc(input->inventory_count, sizeof(char*));
	severities     = (char**)calloc(input->inventory_count, sizeof(char*));
	types          = (char**)calloc(input->inventory_count, sizeof(char*));
	widths         = (char**)calloc(input->inventory_count, sizeof(char*));
	
	for(uint32_t i = 0; i < input->inventory_count; i++){
		serial_numbers[i] = strdup(input->inventory[i]->serial_number);
		comments[i]       = strdup(input->inventory[i]->comment);
		guid_s1s[i]       = strdup(input->inventory[i]->guid_s1);
		guid_s2s[i]       = strdup(input->inventory[i]->guid_s2);
		identifiers[i]    = strdup(input->inventory[i]->identifier);
		lengths[i]        = strdup(input->inventory[i]->length);
		names[i]          = strdup(input->inventory[i]->name);
		part_numbers[i]   = strdup(input->inventory[i]->part_number);
		port_s1s[i]       = strdup(input->inventory[i]->port_s1);
		port_s2s[i]       = strdup(input->inventory[i]->port_s2);
		revisions[i]      = strdup(input->inventory[i]->revision);
		severities[i]     = strdup(input->inventory[i]->severity);
		types[i]          = strdup(input->inventory[i]->type);
		widths[i]         = strdup(input->inventory[i]->width);
	}

	int SQLparameterCount = 15;

	std::string stmt = 
		"SELECT * FROM fn_csm_ib_cable_inventory_collection("
			"$1::int, "
			"$2::text[], "
			"$3::text[], "
			"$4::text[], "
			"$5::text[], "
			"$6::text[], "
			"$7::text[], "
			"$8::text[], "
			"$9::text[], "
			"$10::text[], "
			"$11::text[], "
			"$12::text[], "
			"$13::text[], "
			"$14::text[], "
			"$15::text[] "
			")";
    
	/* Build the parameterized list. */
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddNumericParam(input->inventory_count);
	dbReq->AddTextArrayParam(serial_numbers, input->inventory_count);
	dbReq->AddTextArrayParam(comments, input->inventory_count);
	dbReq->AddTextArrayParam(guid_s1s, input->inventory_count);
	dbReq->AddTextArrayParam(guid_s2s, input->inventory_count);
	dbReq->AddTextArrayParam(identifiers, input->inventory_count);
	dbReq->AddTextArrayParam(lengths, input->inventory_count);
	dbReq->AddTextArrayParam(names, input->inventory_count);
	dbReq->AddTextArrayParam(part_numbers, input->inventory_count);
	dbReq->AddTextArrayParam(port_s1s, input->inventory_count);
	dbReq->AddTextArrayParam(port_s2s, input->inventory_count);
	dbReq->AddTextArrayParam(revisions, input->inventory_count);
	dbReq->AddTextArrayParam(severities, input->inventory_count);
	dbReq->AddTextArrayParam(types, input->inventory_count);
	dbReq->AddTextArrayParam(widths, input->inventory_count);
	
	//free memory
	for(uint32_t i = 0; i < input->inventory_count; i++){
		free(serial_numbers[i]);
		free(comments[i]);
		free(guid_s1s[i]);
		free(guid_s2s[i]);
		free(identifiers[i]);
		free(lengths[i]);
		free(names[i]);
		free(part_numbers[i]);
		free(port_s1s[i]);
		free(port_s2s[i]);
		free(revisions[i]);
		free(severities[i]);
		free(types[i]);
		free(widths[i]);
	}
	//free original guys
	free(serial_numbers); 
	free(comments); 
	free(guid_s1s); 
	free(guid_s2s); 
	free(identifiers);               
	free(lengths); 
	free(names);   
	free(part_numbers); 
	free(port_s1s); 
	free(port_s2s); 
	free(revisions);   
	free(severities);   
	free(types); 
	free(widths);   
	
	*dbPayload = dbReq;

	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
         
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";
         
    return true;
}

bool CSMIIbCableInventoryCollection::CreateByteArray(
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

    if(numberOfRecords == 1)
    {
		//good case
        output->insert_count = strtol(tuples[0]->data[0], nullptr, 10);
		output->update_count = strtol(tuples[0]->data[1], nullptr, 10);
		output->delete_count = strtol(tuples[0]->data[2], nullptr, 10);
    }else {
		//bad case
		LOG(csmapi, error) << STATE_NAME ":CreateByteArray: Unexpected records returned from database. Expected: 1 Got: " << numberOfRecords;
	}

    csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
    csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);

    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
