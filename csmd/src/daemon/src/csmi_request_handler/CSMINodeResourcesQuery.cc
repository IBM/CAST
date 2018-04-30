/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMINodeResourcesQuery.cc

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

#include "CSMINodeResourcesQuery.h"

//Used for debug prints
#define STATE_NAME "CSMINodeResourcesQuery:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_node_resources_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_resources_query_output_t

#define INPUT_STRUCT 
#define OUTPUT_STRUCT csm_node_resources_t

bool CSMINodeResourcesQuery::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	/*Unpack the buffer.*/
	API_PARAMETER_INPUT_TYPE* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if ( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0 ) 
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
	
	int SQLparameterCount = 1;

	/*Open "std::string stmt"*/
	std::string stmt = 
		"SELECT "
			"n.node_name AS node_name, "
			"n.discovered_cores AS node_discovered_cores, "
			"n.discovered_gpus AS node_discovered_gpus, "
			"n.discovered_sockets AS node_discovered_sockets, "
			"n.installed_memory AS node_installed_memory, "
			"n.update_time AS node_last_updated_time, "
			"n.state AS node_state, "
			"n.type AS node_type, "
			"COUNT(v.vg_name) AS vg_count, "
			"array_agg(v.vg_name) AS vg_name, "
			"array_agg(v.scheduler) AS vg_scheduler, "
			"array_agg(v.total_size) AS vg_total_size, "
			"array_agg(v.available_size) AS vg_available_size, "
			"array_agg(v.update_time) AS vg_update_time, "
			"COUNT(vgs.vg_name) AS vgs_count, "
			"array_agg(vgs.vg_name) AS vgs_vg_name, "
			"array_agg(vgs.node_name) AS vgs_node_name, "
			"array_agg(vgs.serial_number) AS vgs_serial_number, "
			"COUNT(ssd.serial_number) AS ssd_count, "
			"array_agg(ssd.serial_number) AS ssd_serial_number, "
			"array_agg(ssd.wear_lifespan_used) AS ssd_wear_lifespan_used, "
			"array_agg(ssd.update_time) AS ssd_update_time "
		"FROM "
			"csm_node AS n "
		"FULL OUTER JOIN csm_vg AS v ON n.node_name = v.node_name "
		"FULL OUTER JOIN csm_vg_ssd AS vgs ON (vgs.vg_name = v.vg_name AND vgs.node_name = n.node_name) "
		"FULL OUTER JOIN csm_ssd AS ssd ON (vgs.serial_number = ssd.serial_number) "
		"WHERE ( "
			"n.type = 'compute' AND "
			"n.node_name = ANY( $1::text[] ) "
			") "
		"GROUP BY "
			"n.node_name "
		"ORDER BY "
			"node_name "
			"ASC NULLS LAST " ;

	add_param_sql( stmt, input->limit > 0, ++SQLparameterCount, "LIMIT $", "::int ")

    add_param_sql( stmt, input->offset > 0, ++SQLparameterCount, "OFFSET $", "::int ")
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddTextArrayParam(input->node_names, input->node_names_count);
	if(input->limit > 0) dbReq->AddNumericParam<int>(input->limit);
	if(input->offset > 0) dbReq->AddNumericParam<int>(input->offset);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

// will return csmi defined error code
bool CSMINodeResourcesQuery::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer, uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx )
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
		output->results = (csmi_node_resources_record_t**)calloc(output->results_count, sizeof(csmi_node_resources_record_t*));
		
		/* Build the individual records for packing. */
		for (uint32_t i = 0; i < numberOfRecords; i++){
			CreateOutputStruct(tuples[i], &(output->results[i]));
		}
		
		// Pack the allocation up.
		csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		
		// Free struct we made.
		csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
		
		//Everything went well. SUCCESS!
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

void CSMINodeResourcesQuery::CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        csmi_node_resources_record_t **output )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";

	// Error check
    if ( fields->nfields != 22 )
    {
        *output = nullptr;
        return;
    }
	
	/*Helper Variables*/
    char* pEnd;
	int d = 0; //data counter tracker
	
	/*Set up data to call API*/
	csmi_node_resources_record_t* o = nullptr;
	/* CSM API initialize and malloc function*/
	csm_init_struct_ptr(csmi_node_resources_record_t, o);
	
	o->node_name               = strdup(fields->data[d]);                                                                           d++;
	o->node_discovered_cores   = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->node_discovered_cores = -1.0;   d++;
	o->node_discovered_gpus    = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->node_discovered_gpus = -1.0;    d++;
    o->node_discovered_sockets = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->node_discovered_sockets = -1.0; d++;
    o->node_installed_memory   = strtoll(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->node_installed_memory = -1.0;  d++;
	o->node_update_time        = strdup(fields->data[d]);                                                                           d++;
	o->node_state              = (csmi_node_state_t)csm_get_enum_from_string(csmi_node_state_t,fields->data[d]);                    d++;
	//logic                     
	if(o->node_state == CSM_NODE_IN_SERVICE){
		o->node_ready = CSM_TRUE;
	}else{
		o->node_ready = CSM_FALSE;
	}  
	
	o->node_type               = (csmi_node_type_t)csm_get_enum_from_string(csmi_node_type_t,fields->data[d]); d++;
	
	//VG data
	int scheduler_vg_slot = -1;
	char* scheduler_vg_name = NULL;
	
	//get the number of VG on this node
	uint32_t vg_count = 0;
	vg_count = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) vg_count = 0; d++;
	
	if(vg_count > 0)
	{
		//all the VG fields
		char** vg_name = NULL;
		char** vg_scheduler = NULL;
		char** vg_total_size = NULL;
		char** vg_available_size = NULL;
		char** vg_update_time = NULL;
		
		//allocate mem
		vg_name = (char**)calloc(vg_count, sizeof(char*));
		vg_scheduler = (char**)calloc(vg_count, sizeof(char*));
		vg_total_size = (char**)calloc(vg_count, sizeof(char*));
		vg_available_size = (char**)calloc(vg_count, sizeof(char*));
		vg_update_time = (char**)calloc(vg_count, sizeof(char*));
		
		csm_prep_csv_to_struct();
		
		//Copy the VG data into temp string arrays.
		csm_parse_dsv_to_struct(fields->data[d], ",\"{}", vg_name, vg_count, CSM_NO_MEMBER, strdup("N/A"), strdup);           d++;
		csm_parse_dsv_to_struct(fields->data[d], ",\"{}", vg_scheduler, vg_count, CSM_NO_MEMBER, strdup("N/A"), strdup);      d++;
		csm_parse_dsv_to_struct(fields->data[d], ",\"{}", vg_total_size, vg_count, CSM_NO_MEMBER, strdup("N/A"), strdup);     d++;
		csm_parse_dsv_to_struct(fields->data[d], ",\"{}", vg_available_size, vg_count, CSM_NO_MEMBER, strdup("N/A"), strdup); d++;
		csm_parse_dsv_to_struct(fields->data[d], ",\"{}", vg_update_time, vg_count, CSM_NO_MEMBER, strdup("N/A"), strdup);    d++;
				
		
		//find which vg is the scheduler
		for(uint32_t counter = 0; counter < vg_count; counter++)
		{
			if(vg_scheduler[counter][0] == 't'){
				//this is the scheduler VG
				scheduler_vg_slot = counter;
				scheduler_vg_name = strdup(vg_name[counter]);
				//found match; exit early condition meet
				counter = vg_count;
			}
		}
		
		if(scheduler_vg_slot == -1){
			//no scheduler vg was found
			o->vg_total_size = -1;
			o->vg_available_size = -1;
			o->vg_update_time = NULL;
			//ssd info
			o->ssds_count = 0;
		}else{
			
			//scheduler vg was found
			o->vg_total_size = strtoll(vg_total_size[scheduler_vg_slot], &pEnd, 10);
			o->vg_available_size = strtoll(vg_available_size[scheduler_vg_slot], &pEnd, 10);
			o->vg_update_time = strdup(vg_update_time[scheduler_vg_slot]);
			
			//find proper ssd info
			//get the number of VG on this node
			uint32_t vgs_count = 0;
			vgs_count = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) vgs_count = 0; d++;
			
			//all the VG_SSD fields
			char** vgs_vg_name = NULL;
			//char** vgs_node_name = NULL;
			char** vgs_serial_number = NULL;
			
			//allocate mem
			vgs_vg_name = (char**)calloc(vgs_count, sizeof(char*));
			//vgs_node_name = (char**)calloc(vgs_count, sizeof(char*));
			vgs_serial_number = (char**)calloc(vgs_count, sizeof(char*));
			
			//Copy the VGS data into temp string arrays.
			csm_parse_dsv_to_struct(fields->data[d], ",\"{}", vgs_vg_name, vgs_count, CSM_NO_MEMBER, strdup("N/A"), strdup);       d++;
			d++; //increase for vgs_node_name
			csm_parse_dsv_to_struct(fields->data[d], ",\"{}", vgs_serial_number, vgs_count, CSM_NO_MEMBER, strdup("N/A"), strdup); d++;
		
			
			//helper vars
			uint32_t num_ssd_in_vg = 0;
			char** ssd_serial_number_max = NULL;
			ssd_serial_number_max = (char**)calloc(vgs_count, sizeof(char*));
			
			for(uint32_t counter = 0; counter < vgs_count; counter++){
				if(strcmp(scheduler_vg_name,vgs_vg_name[counter]) == 0){
					//this record matches the vg
					//copy serial_number into current placeholder
					ssd_serial_number_max[num_ssd_in_vg] = strdup(vgs_serial_number[counter]);
					//increase the placeholder
					num_ssd_in_vg++;
				}
			}
			
			//we know have a list of serial_numbers only in this VG
			o->ssds_count = num_ssd_in_vg;
			o->ssds = (csmi_ssd_resources_record_t**)malloc(o->ssds_count* sizeof(csmi_ssd_resources_record_t*));
			//create space for each guy
			for(uint32_t counter = 0; counter < o->ssds_count; counter++)
			{
				o->ssds[counter] = (csmi_ssd_resources_record_t*)malloc(sizeof(csmi_ssd_resources_record_t));
			}
			
			//get the number of total ssds on this node
			uint32_t ssd_count = 0;
			ssd_count = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) ssd_count = 0; d++;
			
			//all the SSD fields
			char** ssd_serial_number = NULL;
			char** ssd_wear_lifespan_used = NULL;
			char** ssd_update_time = NULL;
			
			//allocate mem
			ssd_serial_number = (char**)calloc(ssd_count, sizeof(char*));
			ssd_wear_lifespan_used = (char**)calloc(ssd_count, sizeof(char*));
			ssd_update_time = (char**)calloc(ssd_count, sizeof(char*));
			
			//Copy the ssd data into temp string arrays.
			csm_parse_dsv_to_struct(fields->data[d], ",\"{}", ssd_serial_number, ssd_count, CSM_NO_MEMBER, strdup("N/A"), strdup);      d++;
			csm_parse_dsv_to_struct(fields->data[d], ",\"{}", ssd_wear_lifespan_used, ssd_count, CSM_NO_MEMBER, strdup("N/A"), strdup); d++;
			csm_parse_dsv_to_struct(fields->data[d], ",\"{}", ssd_update_time, ssd_count, CSM_NO_MEMBER, strdup("N/A"), strdup);        d++;
		
			//keep track of where we are in o->ssds
			uint32_t ssd_tracker = 0;
		
			//copy data from temp into main ssd records
			for(uint32_t counter = 0; counter < ssd_count; counter++){
				//loop through all available ssd on node, then loop through all ssd names on VG, check for match
				for(uint32_t j = 0; j < num_ssd_in_vg; j++)
				{
					//for fernando seg 
					if(ssd_tracker >= o->ssds_count)
					{
						ssd_tracker = o->ssds_count -1;
					}
					//if match, then copy data from ssd record into struct
					if(strcmp(ssd_serial_number[counter], ssd_serial_number_max[j]) == 0){
						o->ssds[ssd_tracker]->serial_number = strdup(ssd_serial_number[counter]);
						o->ssds[ssd_tracker]->wear_lifespan_used = csm_to_double(ssd_wear_lifespan_used[counter]);
						o->ssds[ssd_tracker]->update_time = strdup(ssd_update_time[counter]);
						ssd_tracker++;
					}
				}
			}	
			//all data should be copied now
			//free vgs here.
			for(uint32_t counter = 0; counter < vgs_count; counter++)
			{
				free(vgs_vg_name[counter]);
				//free(vgs_node_name[counter]);
				free(vgs_serial_number[counter]);
			}
			free(vgs_vg_name);
			//free(vgs_node_name);
			free(vgs_serial_number);
			//serial_max
			for(uint32_t counter = 0; counter < num_ssd_in_vg; counter++)
			{
				free(ssd_serial_number_max[counter]);
			}
			free(ssd_serial_number_max);
			
			//ssd
			for(uint32_t counter = 0; counter < ssd_count; counter++)
			{
				free(ssd_serial_number[counter]);
				free(ssd_wear_lifespan_used[counter]);
				free(ssd_update_time[counter]);
			}
			free(ssd_serial_number);
			free(ssd_wear_lifespan_used);
			free(ssd_update_time);
		}
		
		//free the temps
		for(uint32_t counter = 0; counter < vg_count; counter++)
		{
			free(vg_name[counter]);
			free(vg_scheduler[counter]);
			free(vg_total_size[counter]);
			free(vg_available_size[counter]);
			free(vg_update_time[counter]);
		}
		free(vg_name);
		free(vg_scheduler);
		free(vg_total_size);
		free(vg_available_size);
		free(vg_update_time);
		
	}else{
		//no VGs on this node
		o->vg_total_size = -1;
		o->vg_available_size = -1;
		o->vg_update_time = NULL;
		//ssd info
		o->ssds_count = 0;
	}
	
	*output = o;

    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}

