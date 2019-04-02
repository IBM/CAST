/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMISwitchAttributesQueryDetails.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota, John Dunham
* Email: nbuonar@us.ibm.com, jdunham@us.ibm.com
*/

/* Header for this file. */
#include "CSMISwitchAttributesQueryDetails.h"

//Used for debug prints
#define STATE_NAME "CSMISwitchAttributesQueryDetails:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_switch_attributes_query_details_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_switch_attributes_query_details_output_t
#define DB_RECORD_STRUCT csmi_switch_details_t 

bool CSMISwitchAttributesQueryDetails::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    /*Unpack the buffer.*/
	API_PARAMETER_INPUT_TYPE* input = NULL;
	
	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, 
            &input, stringBuffer.c_str(), bufferLength) != 0)
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed...";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength 
            << " stringBuffer = " << stringBuffer.c_str();
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		return false;
	}
	
	int SQLparameterCount = 1;

	/*Open "std::string stmt"*/
	std::string stmt = "SELECT * FROM fn_csm_switch_attributes_query_details($1::text)";
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddTextParam(input->switch_name);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMISwitchAttributesQueryDetails::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

	bool success = true;
	
	*stringBuffer = NULL;
    bufferLength = 0;
	
	/*If we want to return stuff*/
	/*Implement code here*/

	/*Helper Variables*/
	uint32_t numberOfRecords = tuples.size();
	
	LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: numberOfRecords: " << numberOfRecords;

	if( numberOfRecords == 1  && 
            ( tuples[0]->nfields > 0 && tuples[0]->data[0][0] != 0 ) )
    {
        API_PARAMETER_OUTPUT_TYPE output;
        csm_init_struct_versioning(&output);
        output.result_count = numberOfRecords;
        output.result       = (DB_RECORD_STRUCT**)malloc( 
            output.result_count * sizeof(DB_RECORD_STRUCT*));

        for (uint32_t i = 0; i < numberOfRecords; i++)
        {
            CreateOutputStruct(tuples[i], &output.result[i]);
        }

		csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, &output, stringBuffer, &bufferLength);
		csm_free_struct(API_PARAMETER_OUTPUT_TYPE, output);
    }
	else if( numberOfRecords == 1  && 
            ( tuples[0]->nfields > 0 && tuples[0]->data[0][0] == 0 ) )
    {
		//zero records returned. 
        //send nothing back.
		//front end will set CSMI_NO_RESULTS
		
    }else{
		LOG( csmapi, error  ) << STATE_NAME ":CreateByteArray: Invalid number of records recovered.";
        
        ctx->SetErrorCode( CSMERR_DB_ERROR );
        ctx->SetErrorMessage( "Invalid number of records recovered, something has gone wrong." );
        success = false;
	}

    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return success;
}

void CSMISwitchAttributesQueryDetails::CreateOutputStruct(
    csm::db::DBTuple * const & fields,
    DB_RECORD_STRUCT **output )
{
        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";

		// Error check
        if(fields->nfields != 43 )
        {
            *output = nullptr;
            return;
        }
		
		/*Helper Variables*/
		char* pEnd;
		int d = 0; //keep place in data counter
		
		csm_prep_csv_to_struct();
        
		/*Set up data to call API*/
        DB_RECORD_STRUCT *o = nullptr;
		/* CSM API initialize and malloc function*/
        csm_init_struct_ptr(DB_RECORD_STRUCT, o);
		
		//===SWITCH_TABLE_INFORMATION===
		
		o->switch_data = NULL;
		csm_init_struct_ptr(csmi_switch_record_t, o->switch_data);
		
		/* switch_table */
        o->switch_data->switch_name             = strdup(fields->data[d]); d++;
		o->switch_data->serial_number             = strdup(fields->data[d]); d++;
		o->switch_data->discovery_time          = strdup(fields->data[d]); d++;
		o->switch_data->collection_time         = strdup(fields->data[d]); d++;
		o->switch_data->comment                 = strdup(fields->data[d]); d++;
		o->switch_data->description             = strdup(fields->data[d]); d++;
		o->switch_data->fw_version              = strdup(fields->data[d]); d++;
		o->switch_data->gu_id                   = strdup(fields->data[d]); d++;
		o->switch_data->has_ufm_agent           = csm_convert_psql_bool(fields->data[d][0]); d++;
		o->switch_data->hw_version              = strdup(fields->data[d]); d++;
		o->switch_data->ip                      = strdup(fields->data[d]); d++;
		o->switch_data->model                   = strdup(fields->data[d]); d++;
		o->switch_data->num_modules             = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->switch_data->num_modules = -1.0;  d++;
		o->switch_data->physical_frame_location = strdup(fields->data[d]); d++;
		o->switch_data->physical_u_location     = strdup(fields->data[d]); d++;
		o->switch_data->ps_id                   = strdup(fields->data[d]); d++;
		o->switch_data->role                    = strdup(fields->data[d]); d++;
		o->switch_data->server_operation_mode   = strdup(fields->data[d]); d++;
		o->switch_data->sm_mode                 = strdup(fields->data[d]); d++;
		o->switch_data->state                   = strdup(fields->data[d]); d++;
		o->switch_data->sw_version              = strdup(fields->data[d]); d++;
		o->switch_data->system_guid             = strdup(fields->data[d]); d++;
		o->switch_data->system_name             = strdup(fields->data[d]); d++;
		o->switch_data->total_alarms            = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->switch_data->total_alarms = -1.0; d++;
		o->switch_data->type                    = strdup(fields->data[d]); d++;
		o->switch_data->vendor                  = strdup(fields->data[d]); d++;
		
		//save for when we switch some values to non text numbers
        // o->switch_data->available_cores               = strtol(fields->data[1], &pEnd, 10);
		// if(pEnd == fields->data[1]){ o->switch_data->available_cores = -1.0;}
		
		/* Create space for each sub component. */
		
		//===SWITCH_INVENTORY_TABLE_INFORMATION===
		d = 26; //update counter
		o->inventory_count = strtoul(fields->data[d], NULL, 10); d++;
		if ( o->inventory_count > 0 )
        {
		    o->inventory = (csmi_switch_inventory_record_t**)malloc(o->inventory_count * sizeof(csmi_switch_inventory_record_t*));

		    for(uint32_t j = 0; j < o->inventory_count; j++)
		    {
		    	csm_init_struct_ptr(csmi_switch_inventory_record_t, o->inventory[j]);
		    }
		    
            csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->name, strdup("N/A"), strdup);             d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->host_system_guid, strdup("N/A"), strdup); d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->discovery_time, strdup("N/A"), strdup);   d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->collection_time, strdup("N/A"), strdup);  d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->comment, strdup("N/A"), strdup);	       d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->description, strdup("N/A"), strdup);	   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->device_name, strdup("N/A"), strdup);	   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->device_type, strdup("N/A"), strdup);	   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->hw_version, strdup("N/A"), strdup);	   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->max_ib_ports, -1, csm_to_int32);	   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->module_index, -1, csm_to_int32);	   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->number_of_chips, -1, csm_to_int32);  d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->path, strdup("N/A"), strdup);	           d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->serial_number, strdup("N/A"), strdup);	   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->severity, strdup("N/A"), strdup);	       d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->inventory, o->inventory_count, ->status, strdup("N/A"), strdup);           d++;
        }
		
        *output = o;

        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
}
