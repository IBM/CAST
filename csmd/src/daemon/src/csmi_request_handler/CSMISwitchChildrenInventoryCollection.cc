/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMISwitchChildrenInventoryCollection.cc

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
#include "CSMISwitchChildrenInventoryCollection.h"

#define STATE_NAME "CSMISwitchChildrenInventoryCollection:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_switch_inventory_collection_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_switch_children_inventory_collection_output_t

bool CSMISwitchChildrenInventoryCollection::CreatePayload(
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
	char** names = NULL;
	char** host_system_guids = NULL;
	char** comments = NULL;
	char** descriptions = NULL;
	char** device_names = NULL;
	char** device_types = NULL;
	int32_t* max_ib_ports = NULL;
	int32_t* module_indexes = NULL;
	int32_t* number_of_chips = NULL;
	char** paths = NULL;
	char** serial_numbers = NULL;
	char** severities = NULL;
	char** statuses = NULL;
	
	int total_switch_inventories = 0;
	for(uint32_t i = 0; i < input->inventory_count; i++){
		for(uint32_t j = 0; j < input->inventory[i]->inventory_count; j++){
			total_switch_inventories++;
		}
	}
	
	names             = (char**)calloc(total_switch_inventories, sizeof(char*));
	host_system_guids = (char**)calloc(total_switch_inventories, sizeof(char*));
	comments          = (char**)calloc(total_switch_inventories, sizeof(char*));
	descriptions      = (char**)calloc(total_switch_inventories, sizeof(char*));
	device_names      = (char**)calloc(total_switch_inventories, sizeof(char*));
	device_types      = (char**)calloc(total_switch_inventories, sizeof(char*));
	max_ib_ports      = (int32_t*)calloc(total_switch_inventories, sizeof(int32_t));
	module_indexes    = (int32_t*)calloc(total_switch_inventories, sizeof(int32_t));
	number_of_chips   = (int32_t*)calloc(total_switch_inventories, sizeof(int32_t));
	paths             = (char**)calloc(total_switch_inventories, sizeof(char*));
	serial_numbers    = (char**)calloc(total_switch_inventories, sizeof(char*));
	severities        = (char**)calloc(total_switch_inventories, sizeof(char*));
	statuses          = (char**)calloc(total_switch_inventories, sizeof(char*));
	
	int total_switch_inventories_tracker = 0;
	
	//switch inventory record
	for(uint32_t i = 0; i < input->inventory_count; i++){
		for(uint32_t j = 0; j < input->inventory[i]->inventory_count; j++){
			names             [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->name            );
			host_system_guids [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->host_system_guid);
			comments          [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->comment         );
			descriptions      [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->description     );
			device_names      [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->device_name     );
			device_types      [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->device_type     );
			max_ib_ports      [total_switch_inventories_tracker] = input->inventory[i]->inventory[j]->max_ib_ports   ;
			module_indexes    [total_switch_inventories_tracker] = input->inventory[i]->inventory[j]->module_index   ;
			number_of_chips   [total_switch_inventories_tracker] = input->inventory[i]->inventory[j]->number_of_chips;
			paths             [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->path            );
			serial_numbers    [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->serial_number   );
			severities        [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->severity        );
			statuses          [total_switch_inventories_tracker] = strdup(input->inventory[i]->inventory[j]->status          );
			total_switch_inventories_tracker++;
		}
	}
	
	int SQLparameterCount = 14;

	std::string stmt = 
		"SELECT * FROM fn_csm_switch_children_inventory_collection("
			"$1::int, "
			"$2::text[], "
			"$3::text[], "
			"$4::text[], "
			"$5::text[], "
			"$6::text[], "
			"$7::text[], "
			"$8::int[], "
			"$9::int[], "
			"$10::int[], "
			"$11::text[], "
			"$12::text[], "
			"$13::text[], "
			"$14::text[] "
			");";
    
	/* Build the parameterized list. */
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddNumericParam(total_switch_inventories);
	dbReq->AddTextArrayParam(names            , total_switch_inventories);
	dbReq->AddTextArrayParam(host_system_guids, total_switch_inventories);
	dbReq->AddTextArrayParam(comments         , total_switch_inventories);
	dbReq->AddTextArrayParam(descriptions     , total_switch_inventories);
	dbReq->AddTextArrayParam(device_names     , total_switch_inventories);
	dbReq->AddTextArrayParam(device_types     , total_switch_inventories);
	dbReq->AddNumericArrayParam(max_ib_ports     , total_switch_inventories);
	dbReq->AddNumericArrayParam(module_indexes   , total_switch_inventories);
	dbReq->AddNumericArrayParam(number_of_chips  , total_switch_inventories);
	dbReq->AddTextArrayParam(paths            , total_switch_inventories);
	dbReq->AddTextArrayParam(serial_numbers   , total_switch_inventories);
	dbReq->AddTextArrayParam(severities       , total_switch_inventories);
	dbReq->AddTextArrayParam(statuses         , total_switch_inventories);
	
	//free memory
	for(int i = 0; i < total_switch_inventories; i++){
		free(names            [i]);
		free(host_system_guids[i]);
		free(comments         [i]);
		free(descriptions     [i]);
		free(device_names     [i]);
		free(device_types     [i]);
		free(paths            [i]);
		free(serial_numbers   [i]);
		free(severities       [i]);
		free(statuses         [i]);
	}
	//free original guys
	free(names            );
	free(host_system_guids);
	free(comments         );
	free(descriptions     );
	free(device_names     );
	free(device_types     );
	free(max_ib_ports     );
	free(module_indexes   );
	free(number_of_chips  );
	free(paths            );
	free(serial_numbers   );
	free(severities       );
	free(statuses         );
	
	
	
	*dbPayload = dbReq;

	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
         
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";
         
    return true;
}

bool CSMISwitchChildrenInventoryCollection::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Enter";

    *stringBuffer = nullptr;
    bufferLength = 0;

    //uint32_t numberOfRecords = tuples.size();

    API_PARAMETER_OUTPUT_TYPE* output = nullptr;
    csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);

    /* output->failure_count = numberOfRecords;

    if(numberOfRecords > 0)
    {
        output->failure_ib_cables = (char**)malloc(numberOfRecords * sizeof(char*));
        
        for(uint32_t i = 0; i < numberOfRecords; ++i )
        {
            output->failure_ib_cables[i] = strdup(tuples[i]->data[0]);
        }
    } */

    csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
    csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);

    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
