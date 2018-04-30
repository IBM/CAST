/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMINodeAttributesQueryDetails.cc

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
#include "CSMINodeAttributesQueryDetails.h"

//Used for debug prints
#define STATE_NAME "CSMINodeAttributesQueryDetails:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_node_attributes_query_details_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_attributes_query_details_output_t
#define DB_RECORD_STRUCT csmi_node_details_t 

bool CSMINodeAttributesQueryDetails::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
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
	std::string stmt = "SELECT * FROM fn_csm_node_attributes_query_details($1::text)";
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	dbReq->AddTextParam(input->node_name);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMINodeAttributesQueryDetails::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
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
            success =  success && CreateOutputStruct(tuples[i], &output.result[i]);
        }

        if (success)
        {
		    csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, &output, stringBuffer, &bufferLength);
        }
        else
        {
            ctx->SetErrorCode( CSMERR_DB_ERROR );
            ctx->SetErrorMessage( "Creation of the output struct failed, something was wrong with the query response.");
        }

		csm_free_struct(API_PARAMETER_OUTPUT_TYPE, output);
    }
    else if (numberOfRecords > 1 )
    {
		LOG( csmapi, error  ) << STATE_NAME ":CreateByteArray: Invalid number of records recovered.";

        ctx->SetErrorCode( CSMERR_DB_ERROR );
        ctx->SetErrorMessage( "Invalid number of records recovered, something has gone wrong." );
        success = false;
	}

    LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";

    return success;
}

bool CSMINodeAttributesQueryDetails::CreateOutputStruct(
    csm::db::DBTuple * const & fields,
    DB_RECORD_STRUCT **output )
{
        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";

		// Error check
        if(fields->nfields != 66 )
        {
            *output = nullptr;
            return false;
        }

		/*Helper Variables*/
        char* pEnd;
		int d = 0; //keep place in data counter
		
		csm_prep_csv_to_struct();
        
		/*Set up data to call API*/
        DB_RECORD_STRUCT *o = nullptr;
		/* CSM API initialize and malloc function*/
        csm_init_struct_ptr(DB_RECORD_STRUCT, o);
		
		//===NODE_TABLE_INFORMATION===
		
		o->node = NULL;
		csm_init_struct_ptr(csmi_node_attributes_record_t, o->node);
		
		/* node_table */
        o->node->node_name               = strdup(fields->data[d]);                                                                              d++;
		o->node->collection_time         = strdup(fields->data[d]);                                                                              d++;
		o->node->update_time             = strdup(fields->data[d]);                                                                              d++;
        o->node->discovered_cores        = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->discovered_cores = -1.0;}   d++;
		o->node->discovered_dimms        = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->discovered_dimms = -1.0;}   d++;
		o->node->discovered_gpus         = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->discovered_gpus = -1.0;}    d++;
		o->node->discovered_hcas         = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->discovered_hcas = -1.0;}    d++;
        o->node->discovered_sockets      = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->discovered_sockets = -1.0;} d++;
		o->node->discovered_ssds         = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->discovered_ssds = -1.0;}    d++;
        o->node->comment                 = strdup(fields->data[d]);                                                                              d++;
		o->node->feature_1               = strdup(fields->data[d]);                                                                              d++;
        o->node->feature_2               = strdup(fields->data[d]);                                                                              d++;
        o->node->feature_3               = strdup(fields->data[d]);                                                                              d++;
        o->node->feature_4               = strdup(fields->data[d]);                                                                              d++;
        o->node->hard_power_cap          = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->hard_power_cap = -1.0;}     d++;
        o->node->installed_memory        = strtoll(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->installed_memory = -1.0;}  d++;
		o->node->installed_swap          = strtoll(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]){ o->node->installed_swap = -1.0;}    d++;
        o->node->kernel_release          = strdup(fields->data[d]);                                                                              d++;
        o->node->kernel_version          = strdup(fields->data[d]);                                                                              d++;
        o->node->machine_model           = strdup(fields->data[d]);                                                                              d++;
        o->node->os_image_name           = strdup(fields->data[d]);                                                                              d++;
		o->node->os_image_uuid           = strdup(fields->data[d]);                                                                              d++;
        o->node->physical_frame_location = strdup(fields->data[d]);                                                                              d++;
        o->node->physical_u_location     = strdup(fields->data[d]);                                                                              d++;
        o->node->primary_agg             = strdup(fields->data[d]);                                                                              d++;
        o->node->secondary_agg           = strdup(fields->data[d]);                                                                              d++;
        o->node->serial_number           = strdup(fields->data[d]);                                                                              d++;
	    o->node->state                   = (csmi_node_state_t)csm_get_enum_from_string(csmi_node_state_t,fields->data[d]);                       d++;
	    o->node->type                    = (csmi_node_type_t)csm_get_enum_from_string(csmi_node_type_t,fields->data[d]);                         d++;
		
		
		/* Create space for each sub component. */
		
		//===DIMM_TABLE_INFORMATION===
		d = 29; //update counter
		o->dimms_count = strtoul(fields->data[d], NULL, 10); d++;
        if ( o->dimms_count > 0 )
        {
		    o->dimms = (csmi_dimm_record_t**)malloc(o->dimms_count * sizeof(csmi_dimm_record_t*));

		    for(uint32_t j = 0; j < o->dimms_count; j++)
		    {
		    	csm_init_struct_ptr(csmi_dimm_record_t, o->dimms[j]);
		    }
		    
            csm_parse_psql_array_to_struct( fields->data[d], o->dimms, o->dimms_count, ->serial_number, strdup("N/A"), strdup);     d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->dimms, o->dimms_count, ->physical_location, strdup("N/A"), strdup); d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->dimms, o->dimms_count, ->size, -1, csm_to_int64);                   d++;
        }
		
		//===GPU_TABLE_INFORMATION===		
		/*Set number of gpus on this node.*/
		d = 33; //update counter
		o->gpus_count = strtoul(fields->data[d], NULL, 10); d++;
        if ( o->gpus_count > 0 )
        {
		    o->gpus = (csmi_gpu_record_t**)malloc(o->gpus_count * sizeof(csmi_gpu_record_t*));

		    for(uint32_t j = 0; j < o->gpus_count; j++)
		    {
		    	csm_init_struct_ptr(csmi_gpu_record_t, o->gpus[j]);
		    }

            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->gpu_id, -1, csm_to_int32);                     d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->device_name, strdup("N/A"), strdup);           d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->hbm_memory, -1, csm_to_int64);                 d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->inforom_image_version, strdup("N/A"), strdup); d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->pci_bus_id, strdup("N/A"), strdup);            d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->serial_number, strdup("N/A"), strdup);         d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->uuid, strdup("N/A"), strdup);                  d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->gpus, o->gpus_count, ->vbios, strdup("N/A"), strdup);                 d++;
        }
		                                         
		
		//===HCA_TABLE_INFORMATION===
		/*Set number of hcas on this node.*/
		d = 42; //update counter 
		o->hcas_count = strtoul(fields->data[d], NULL, 10); d++;
        if ( o->hcas_count > 0 )
        {
		    o->hcas = (csmi_hca_record_t**)malloc(o->hcas_count * sizeof(csmi_hca_record_t*));

		    for(uint32_t j = 0; j < o->hcas_count; j++)
		    {
		    	csm_init_struct_ptr(csmi_hca_record_t, o->hcas[j]);
		    }
		
	        csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->serial_number, strdup("N/A"), strdup); d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->board_id, strdup("N/A"), strdup);      d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->device_name, strdup("N/A"), strdup);   d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->fw_ver, strdup("N/A"), strdup);		  d++;	
			csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->guid, strdup("N/A"), strdup);	      d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->hw_rev, strdup("N/A"), strdup);	      d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->part_number, strdup("N/A"), strdup);	  d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->hcas, o->hcas_count, ->pci_bus_id, strdup("N/A"), strdup);    d++;
			
		}
		
		
		//===PROCESSOR_TABLE_INFORMATION===
		d = 51; //update counter 
		o->processors_count = strtoul(fields->data[d], NULL, 10); d++;
        if ( o->processors_count > 0 ) 
        {
		    o->processors = (csmi_processor_record_t**)malloc( o->processors_count * sizeof(csmi_processor_record_t*));

		    for(uint32_t j = 0; j < o->processors_count; j++)
		    {
		    	csm_init_struct_ptr(csmi_processor_record_t, o->processors[j]);
		    }

	        csm_parse_psql_array_to_struct( fields->data[d], o->processors, o->processors_count, ->serial_number, strdup("N/A"), strdup);     d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->processors, o->processors_count, ->discovered_cores, -1, csm_to_int32);       d++;
	        csm_parse_psql_array_to_struct( fields->data[d], o->processors, o->processors_count, ->physical_location, strdup("N/A"), strdup); d++;
        }
		

		//===SSD_TABLE_INFORMATION===
		d = 55; //update counter 
		o->ssds_count = strtoul(fields->data[d], NULL, 10); d++;
        if ( o->ssds_count > 0 )
        {
            o->ssds = (csmi_ssd_record_t**)malloc( o->ssds_count * sizeof(csmi_ssd_record_t*));

            for(uint32_t j = 0; j < o->ssds_count; j++)
            {
                csm_init_struct_ptr(csmi_ssd_record_t, o->ssds[j]);
            }
		
	        csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->serial_number, strdup("N/A"), strdup);                d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->device_name, strdup("N/A"), strdup);                  d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->fw_ver, strdup("N/A"), strdup);                       d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->pci_bus_id, strdup("N/A"), strdup);                   d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->size, -1, csm_to_int64);                              d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->update_time, strdup("N/A"), strdup);                  d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->wear_lifespan_used, -1.0, csm_to_double);             d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->wear_percent_spares_remaining,  -1.0, csm_to_double); d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->wear_total_bytes_read, -1, csm_to_int64);             d++;
			csm_parse_psql_array_to_struct( fields->data[d], o->ssds, o->ssds_count, ->wear_total_bytes_written, -1, csm_to_int64);          d++;
        }

        *output = o;

        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
        return true;
}
