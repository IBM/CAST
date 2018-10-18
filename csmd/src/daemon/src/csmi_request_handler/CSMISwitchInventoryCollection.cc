/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMISwitchInventoryCollection.cc

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
#include "CSMISwitchInventoryCollection.h"

#define STATE_NAME "CSMISwitchInventoryCollection:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_switch_inventory_collection_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_switch_inventory_collection_output_t

bool CSMISwitchInventoryCollection::CreatePayload(
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
	char** switch_names = NULL;
	char** serial_numbers = NULL;
	char** comments = NULL;
	char** descriptions = NULL;
	char** fw_versions = NULL;
	char** gu_ids = NULL;
	char** has_ufm_agents = NULL;
	char** hw_versions = NULL;
	char** ips = NULL;
	char** models = NULL;
	int32_t* num_modules = NULL;
	char** frame_locations = NULL;
	char** u_locations = NULL;
	char** ps_ids = NULL;
	char** roles = NULL;
	char** server_operation_modes = NULL;
	char** sm_modes = NULL;
	char** states = NULL;
	char** sw_versions = NULL;
	char** system_guids = NULL;
	char** system_names = NULL;
	int32_t* total_alarms = NULL;
	char** types = NULL;
	char** vendors = NULL;
	
	switch_names           = (char**)calloc(input->inventory_count, sizeof(char*));
	serial_numbers         = (char**)calloc(input->inventory_count, sizeof(char*));
	comments               = (char**)calloc(input->inventory_count, sizeof(char*));
	descriptions           = (char**)calloc(input->inventory_count, sizeof(char*));
	fw_versions            = (char**)calloc(input->inventory_count, sizeof(char*));
	gu_ids                 = (char**)calloc(input->inventory_count, sizeof(char*));
	has_ufm_agents         = (char**)calloc(input->inventory_count, sizeof(char*));
	hw_versions            = (char**)calloc(input->inventory_count, sizeof(char*));
	ips                    = (char**)calloc(input->inventory_count, sizeof(char*));
	models                 = (char**)calloc(input->inventory_count, sizeof(char*));
	num_modules            = (int32_t*)calloc(input->inventory_count, sizeof(int32_t));
	frame_locations        = (char**)calloc(input->inventory_count, sizeof(char*));
	u_locations            = (char**)calloc(input->inventory_count, sizeof(char*));
	ps_ids                 = (char**)calloc(input->inventory_count, sizeof(char*));
	roles                  = (char**)calloc(input->inventory_count, sizeof(char*));
	server_operation_modes = (char**)calloc(input->inventory_count, sizeof(char*));
	sm_modes               = (char**)calloc(input->inventory_count, sizeof(char*));
	states                 = (char**)calloc(input->inventory_count, sizeof(char*));
	sw_versions            = (char**)calloc(input->inventory_count, sizeof(char*));
	system_guids           = (char**)calloc(input->inventory_count, sizeof(char*));
	system_names           = (char**)calloc(input->inventory_count, sizeof(char*));
	total_alarms           = (int32_t*)calloc(input->inventory_count, sizeof(int32_t));
	types                  = (char**)calloc(input->inventory_count, sizeof(char*));
	vendors                = (char**)calloc(input->inventory_count, sizeof(char*));
	
	//base switch record
	for(uint32_t i = 0; i < input->inventory_count; i++){
		switch_names[i]           = strdup(input->inventory[i]->switch_data->switch_name);
		serial_numbers[i]         = strdup(input->inventory[i]->switch_data->serial_number);
		comments[i]               = strdup(input->inventory[i]->switch_data->comment);
		descriptions[i]           = strdup(input->inventory[i]->switch_data->description);
		fw_versions[i]            = strdup(input->inventory[i]->switch_data->fw_version);
		gu_ids[i]                 = strdup(input->inventory[i]->switch_data->gu_id);
		//temp copy bool into a string array 
		has_ufm_agents[i]         = NULL;
		has_ufm_agents[i] = (char*)calloc (2, sizeof(char));
		has_ufm_agents[i][0] = csm_print_bool_custom(input->inventory[i]->switch_data->has_ufm_agent,'t','f');
		//space 1 is set to null via calloc 
		hw_versions[i]            = strdup(input->inventory[i]->switch_data->hw_version);
		ips[i]                    = strdup(input->inventory[i]->switch_data->ip);
		models[i]                 = strdup(input->inventory[i]->switch_data->model);
		num_modules[i]            = input->inventory[i]->switch_data->num_modules;
		frame_locations[i]        = strdup(input->inventory[i]->switch_data->physical_frame_location);
		u_locations[i]            = strdup(input->inventory[i]->switch_data->physical_u_location);
		ps_ids[i]                 = strdup(input->inventory[i]->switch_data->ps_id);
		roles[i]                  = strdup(input->inventory[i]->switch_data->role);
		server_operation_modes[i] = strdup(input->inventory[i]->switch_data->server_operation_mode);
		sm_modes[i]               = strdup(input->inventory[i]->switch_data->sm_mode);
		states[i]                 = strdup(input->inventory[i]->switch_data->state);
		sw_versions[i]            = strdup(input->inventory[i]->switch_data->sw_version);
		system_guids[i]           = strdup(input->inventory[i]->switch_data->system_guid);
		system_names[i]           = strdup(input->inventory[i]->switch_data->system_name);
		total_alarms[i]           = input->inventory[i]->switch_data->total_alarms;
		types[i]                  = strdup(input->inventory[i]->switch_data->type);
		vendors[i]                = strdup(input->inventory[i]->switch_data->vendor);
	}

	int SQLparameterCount = 25;

	std::string stmt = 
		"SELECT * FROM fn_csm_switch_inventory_collection("
			"$1::int, "
			"$2::text[], "
			"$3::text[], "
			"$4::text[], "
			"$5::text[], "
			"$6::text[], "
			"$7::text[], "
			"$8::boolean[], "
			"$9::text[], "
			"$10::text[], "
			"$11::text[], "
			"$12::int[], "
			"$13::text[], "
			"$14::text[], "
			"$15::text[], "
			"$16::text[], "
			"$17::text[], "
			"$18::text[], "
			"$19::text[], "
			"$20::text[], "
			"$21::text[], "
			"$22::text[], "
			"$23::int[], "
			"$24::text[], "
			"$25::text[] "
			");";
    
	/* Build the parameterized list. */
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddNumericParam(input->inventory_count);
	dbReq->AddTextArrayParam(switch_names, input->inventory_count);
	dbReq->AddTextArrayParam(serial_numbers, input->inventory_count);
	dbReq->AddTextArrayParam(comments, input->inventory_count);
	dbReq->AddTextArrayParam(descriptions, input->inventory_count);
	dbReq->AddTextArrayParam(fw_versions, input->inventory_count);
	dbReq->AddTextArrayParam(gu_ids, input->inventory_count);
	dbReq->AddTextArrayParam(has_ufm_agents, input->inventory_count);
	dbReq->AddTextArrayParam(hw_versions, input->inventory_count);
	dbReq->AddTextArrayParam(ips, input->inventory_count);
	dbReq->AddTextArrayParam(models, input->inventory_count);
	dbReq->AddNumericArrayParam(num_modules, input->inventory_count);
	dbReq->AddTextArrayParam(frame_locations, input->inventory_count);
	dbReq->AddTextArrayParam(u_locations, input->inventory_count);
	dbReq->AddTextArrayParam(ps_ids, input->inventory_count);
	dbReq->AddTextArrayParam(roles, input->inventory_count);
	dbReq->AddTextArrayParam(server_operation_modes, input->inventory_count);
	dbReq->AddTextArrayParam(sm_modes, input->inventory_count);
	dbReq->AddTextArrayParam(states, input->inventory_count);
	dbReq->AddTextArrayParam(sw_versions, input->inventory_count);
	dbReq->AddTextArrayParam(system_guids, input->inventory_count);
	dbReq->AddTextArrayParam(system_names, input->inventory_count);
	dbReq->AddNumericArrayParam(total_alarms, input->inventory_count);
	dbReq->AddTextArrayParam(types, input->inventory_count);
	dbReq->AddTextArrayParam(vendors, input->inventory_count);
	
	//free memory
	for(uint32_t i = 0; i < input->inventory_count; i++){
		free(switch_names[i]           );
		free(serial_numbers[i]         );
		free(comments[i]               );
		free(descriptions[i]           );
		free(fw_versions[i]            );
		free(gu_ids[i]                 );
		free(has_ufm_agents[i]         );
		free(hw_versions[i]            );
		free(ips[i]                    );
		free(models[i]                 );
		//free(num_modules[i]            );
		free(frame_locations[i]        );
		free(u_locations[i]            );
		free(ps_ids[i]                 );
		free(roles[i]                  );
		free(server_operation_modes[i] );
		free(sm_modes[i]               );
		free(states[i]                 );
		free(sw_versions[i]            );
		free(system_guids[i]           );
		free(system_names[i]           );
		//free(total_alarms[i]           );
		free(types[i]                  );
		free(vendors[i]                );
	}
	//free original guys
	free(switch_names            );
	free(serial_numbers          );
	free(comments                );
	free(descriptions            );
	free(fw_versions             );
	free(gu_ids                  );
	free(has_ufm_agents          );
	free(hw_versions             );
	free(ips                     );
	free(models                  );
	free(num_modules             );
	free(frame_locations         );
	free(u_locations             );
	free(ps_ids                  );
	free(roles                   );
	free(server_operation_modes  );
	free(sm_modes                );
	free(states                  );
	free(sw_versions             );
	free(system_guids            );
	free(system_names            );
	free(total_alarms            );
	free(types                   );
	free(vendors                 );
	
	
	*dbPayload = dbReq;

	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
         
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";
         
    return true;
}

bool CSMISwitchInventoryCollection::CreateByteArray(
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
		output->delete_module_count = strtol(tuples[0]->data[3], nullptr, 10);
    }else {
		//bad case
		LOG(csmapi, error) << STATE_NAME ":CreateByteArray: Unexpected records returned from database. Expected: 1 Got: " << numberOfRecords;
	}

    csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
    csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);

    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
