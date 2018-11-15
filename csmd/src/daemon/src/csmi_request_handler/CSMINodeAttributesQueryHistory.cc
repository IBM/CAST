/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMINodeAttributesQueryHistory.cc

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
#include "CSMINodeAttributesQueryHistory.h"

//Used for debug prints
#define STATE_NAME "CSMINodeAttributesQueryHistory:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_node_attributes_query_history_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_attributes_query_history_output_t
#define DB_RECORD_STRUCT csmi_node_attributes_history_record_t 

bool CSMINodeAttributesQueryHistory::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

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
	
	int SQLparameterCount = 1;

	/*Open "std::string stmt"*/
	std::string stmt = 
		"SELECT "
			"history_time, "
			"node_name, "
			"collection_time, "
			"update_time, "
			"comment, "
			"discovered_cores, "
			"discovered_dimms, "
			"discovered_gpus, "
			"discovered_hcas, "
            "discovered_sockets, "
			"discovered_ssds, "
			"feature_1, "
			"feature_2, "
			"feature_3, "
			"feature_4, "
			"hard_power_cap, "
            "installed_memory, "
			"installed_swap, "
			"kernel_release, "
			"kernel_version, "
			"machine_model, "
            "os_image_name, "
			"os_image_uuid, "
			"physical_frame_location, "
			"physical_u_location, "
			"primary_agg, "
            "secondary_agg, "
			"serial_number, "
			"state, "
			"type "
		"FROM "
			"csm_node_history "
		"WHERE ("
			"node_name = $1::text"
			") "
		"ORDER BY "
			"node_name, ";
			if(input->order_by == 'a'){
				stmt.append("history_time ASC NULLS LAST ");
			}else if(input->order_by == 'd'){
				stmt.append("history_time DESC NULLS LAST ");
			}else{
				//default case
				stmt.append("history_time ASC NULLS LAST ");
			}
		add_param_sql( stmt, input->limit > 0, ++SQLparameterCount,
            "LIMIT $", "::int ")

		add_param_sql( stmt, input->offset > 0, ++SQLparameterCount,
            "OFFSET $", "::int ")
			
		
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddTextParam(input->node_name);
	if(input->limit > 0) dbReq->AddNumericParam<int>(input->limit);
	if(input->offset > 0) dbReq->AddNumericParam<int>(input->offset);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMINodeAttributesQueryHistory::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

	*stringBuffer = NULL;
    bufferLength = 0;
	
	/*If we want to return stuff*/
	/*Implement code here*/

	/*Helper Variables*/
	uint32_t numberOfRecords = tuples.size();

	if(numberOfRecords > 0){
		/*Our SQL query found at least one matching record.*/
		
        /* Prepare the data to be returned. */
		API_PARAMETER_OUTPUT_TYPE* output = NULL;
		csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		/* Say how many results there are. */
		output->results_count = numberOfRecords;
		/* Create space for each result. */
		output->results = (DB_RECORD_STRUCT**)calloc(output->results_count, sizeof(DB_RECORD_STRUCT*));
		
		/* Build the individual records for packing. */
		for (uint32_t i = 0; i < numberOfRecords; i++){
			CreateOutputStruct(tuples[i], &(output->results[i]));
		}
		
		// Pack the allocation up.
		csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		
		// Free struct we made.
		csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
    }
    
    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

void CSMINodeAttributesQueryHistory::CreateOutputStruct(
    csm::db::DBTuple * const & fields, 
    DB_RECORD_STRUCT **output )
{
    LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";
	
	// Error check
    if(fields->nfields != 30) 
    {
        *output = nullptr;
        return;
    }

    DB_RECORD_STRUCT *o = nullptr;
    csm_init_struct_ptr(DB_RECORD_STRUCT, o);
	
	/*Helper Variables*/
	char* pEnd; // comparison pointer for data conversion check.
	int i = 0; //keep place in data counter
	
	o->history_time            = strdup(fields->data[i]);                                                        i++;
	o->node_name               = strdup(fields->data[i]);                                                        i++;
	o->collection_time         = strdup(fields->data[i]);                                                        i++;
	o->update_time             = strdup(fields->data[i]);                                                        i++;
	o->comment                 = strdup(fields->data[i]);                                                        i++;
    o->discovered_cores        = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->discovered_cores = -1.0;} i++;
	o->discovered_dimms        = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->discovered_dimms = -1.0;} i++;
    o->discovered_gpus         = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->discovered_gpus = -1.0;} i++;
	o->discovered_hcas         = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->discovered_hcas = -1.0;} i++;
    o->discovered_sockets      = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->discovered_sockets = -1.0;} i++;
	o->discovered_ssds         = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->discovered_ssds = -1.0;} i++;
	o->feature_1               = strdup(fields->data[i]);                                                        i++;
	o->feature_2               = strdup(fields->data[i]);                                                        i++;
	o->feature_3               = strdup(fields->data[i]);                                                        i++;
	o->feature_4               = strdup(fields->data[i]);                                                        i++;
    o->hard_power_cap          = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->hard_power_cap = -1.0;} i++;
    o->installed_memory        = strtoll(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->installed_memory = -1.0;} i++;
    o->installed_swap          = strtoll(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->installed_swap = -1.0;} i++;
	o->kernel_release          = strdup(fields->data[i]);                                                        i++;
	o->kernel_version          = strdup(fields->data[i]);                                                        i++;
	o->machine_model           = strdup(fields->data[i]);                                                        i++;
	o->os_image_name           = strdup(fields->data[i]);                                                        i++;
	o->os_image_uuid           = strdup(fields->data[i]);                                                        i++;
	o->physical_frame_location = strdup(fields->data[i]);                                                        i++;
	o->physical_u_location     = strdup(fields->data[i]);                                                        i++;
	o->primary_agg             = strdup(fields->data[i]);                                                        i++;
	o->secondary_agg           = strdup(fields->data[i]);                                                        i++;
	o->serial_number           = strdup(fields->data[i]);                                                        i++;
    o->state                   = (csmi_node_state_t)csm_get_enum_from_string(csmi_node_state_t,fields->data[i]); i++;
	o->type                    = (csmi_node_type_t)csm_get_enum_from_string(csmi_node_type_t,fields->data[i]);   i++;
	
	*output = o;
	
    LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
}
