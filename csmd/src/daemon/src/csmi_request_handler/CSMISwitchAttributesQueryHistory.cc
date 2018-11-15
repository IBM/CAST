/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMISwitchAttributesQueryHistory.cc

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
#include "CSMISwitchAttributesQueryHistory.h"
/* Includes for ras structs and helper functions for those structs including serialization. */
#include "csmi/include/csmi_type_inv.h"
#include "csmi/include/csmi_type_inv_funct.h"

#include "include/csm_event_type_definitions.h"

#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>     // Provides gettimeofday()

#define STATE_NAME "CSMINodeAttributesQueryHistory:"
#define INPUT_STRUCT  csm_switch_attributes_query_history_input_t
#define OUTPUT_STRUCT csmi_switch_history_record_t 

bool CSMISwitchAttributesQueryHistory::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";
	
	INPUT_STRUCT* input = NULL;

    if ( csm_deserialize_struct( INPUT_STRUCT, &input, arguments.c_str(), len ) == 0 )
    {
        int paramCount = 1;
        std::string stmt = "SELECT "
                "history_time, "
				"switch_name, "
				"serial_number, "
				"discovery_time, "
				"collection_time, "
				"comment, "
				"description, "
                "fw_version, "
				"gu_id, "
				"has_ufm_agent, "
				"hw_version, "
				"ip, "
				"model, "
				"num_modules, "
                "physical_frame_location, " 
                "physical_u_location, "
				"ps_id, "
				"role, "
				"server_operation_mode, "
				"sm_mode, "
				"state, "
				"sw_version, "
				"system_guid, "
				"system_name, "
				"total_alarms, "
                "type, "
				"vendor, "
				"operation, "
				"archive_history_time "
            "FROM csm_switch_history "
            "WHERE switch_name = $1::text "
            "ORDER BY switch_name, history_time ASC NULLS LAST ";
        
        if ( input->limit > 0 ) 
            stmt.append("LIMIT $").append(
                std::to_string(++paramCount)).append("::int ");

        if ( input->offset > 0 )
            stmt.append("OFFSET $").append(
                std::to_string(++paramCount)).append("::int ");

        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddTextParam(input->switch_name);  // $1 text
        if ( input->limit  > 0 ) dbReq->AddNumericParam<int>(input->limit); // $2 int
        if ( input->offset > 0 ) dbReq->AddNumericParam<int>(input->offset);// $2/3 int

        *dbPayload = dbReq;
        csm_free_struct_ptr( INPUT_STRUCT, input );

        LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";

        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the query, struct could not be deserialized");
        return false;
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMISwitchAttributesQueryHistory::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    // Init the buffer
    *buf = nullptr;
    bufLen = 0;
	
	uint32_t recordCount = tuples.size();
    
    if (recordCount > 0 )
    {
        csm_switch_attributes_query_history_output_t* output = NULL;
        csm_init_struct_ptr(csm_switch_attributes_query_history_output_t, output);

        output->results_count = recordCount;
        output->results = (OUTPUT_STRUCT**) malloc( recordCount * sizeof(OUTPUT_STRUCT*));
	    
        for ( uint32_t record = 0; record < recordCount; ++record )
        {
            CreateOutputStruct( tuples[record], &(output->results[record]) );
        }

        csm_serialize_struct( csm_switch_attributes_query_history_output_t, output, buf, &bufLen );
        
        csm_free_struct_ptr( csm_switch_attributes_query_history_output_t, output);
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

void CSMISwitchAttributesQueryHistory::CreateOutputStruct(
        csm::db::DBTuple * const & fields, 
        OUTPUT_STRUCT **output)
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";
	
    if ( fields->nfields != 29 )
    {
        *output = nullptr;
        return;
    }
	
	/*Helper Variables*/
    char* pEnd;
	int d = 0; //data counter tracker

	OUTPUT_STRUCT *o = nullptr;
    csm_init_struct_ptr(OUTPUT_STRUCT, o);

	o->history_time            = strdup(fields->data[d]);                                                                d++;
	o->switch_name             = strdup(fields->data[d]);                                                                d++;
	o->serial_number           = strdup(fields->data[d]);                                                                d++;
	o->discovery_time          = strdup(fields->data[d]);                                                                d++;
	o->collection_time         = strdup(fields->data[d]);                                                                d++;
	o->comment                 = strdup(fields->data[d]);                                                                d++;
	o->description             = strdup(fields->data[d]);                                                                d++;
	o->fw_version              = strdup(fields->data[d]);                                                                d++;
	o->gu_id                   = strdup(fields->data[d]);                                                                d++;
	o->has_ufm_agent           = csm_convert_psql_bool(fields->data[d][0]);                                              d++;
	o->hw_version              = strdup(fields->data[d]);                                                                d++;
	o->ip                      = strdup(fields->data[d]);                                                                d++;
	o->model                   = strdup(fields->data[d]);                                                                d++;
	o->num_modules             = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->num_modules = -1.0;  d++;
	o->physical_frame_location = strdup(fields->data[d]);                                                                d++;
	o->physical_u_location     = strdup(fields->data[d]);                                                                d++;
	o->ps_id                   = strdup(fields->data[d]);                                                                d++;
	o->role                    = strdup(fields->data[d]);                                                                d++;
	o->server_operation_mode   = strdup(fields->data[d]);                                                                d++;
	o->sm_mode                 = strdup(fields->data[d]);                                                                d++;
	o->state                   = strdup(fields->data[d]);                                                                d++;
	o->sw_version              = strdup(fields->data[d]);                                                                d++;
	o->system_guid             = strdup(fields->data[d]);                                                                d++;
	o->system_name             = strdup(fields->data[d]);                                                                d++;
	o->total_alarms            = strtol(fields->data[d], &pEnd, 10); if(pEnd == fields->data[d]) o->total_alarms = -1.0; d++;
	o->type                    = strdup(fields->data[d]);                                                                d++;
	o->vendor                  = strdup(fields->data[d]);                                                                d++;
	o->operation               = strdup(fields->data[d]);                                                                d++;
	o->archive_history_time    = strdup(fields->data[d]);                                                                d++;
    
	
	
	*output = o;
	
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}

