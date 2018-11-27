/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasEventQueryAllocation.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
 * Author: John Dunham
 * Email:  jdunham@us.ibm.com
 */

/* ## INCLUDES ## */
/* Header for this file. */
#include "CSMIRasEventQueryAllocation.h"
/* ## DEFINES ## */
//Used for debug prints
#define STATE_NAME "CSMIRasEventQueryAllocation:"
// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_ras_event_query_allocation_input_t
#define OUTPUT_STRUCT csm_ras_event_query_allocation_output_t
#define DB_RECORD_STRUCT csmi_ras_event_action_t

bool CSMIRasEventQueryAllocation::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent** dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	/*Unpack the buffer.*/
	INPUT_STRUCT* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(INPUT_STRUCT, 
            &input, stringBuffer.c_str(), bufferLength) == 0 )
    {

        // =====================================================================
	    int paramCount = 1;
        std::string stmt = 
           "SELECT "
               "r.rec_id, r.msg_id, r.msg_id_seq, r.time_stamp, r.location_name, "
               "r.count, r.message, r.raw_data, r.archive_history_time "
            "FROM "
               "(SELECT "
                    "begin_time, NULL as end_time "
                "FROM csm_allocation "
                "WHERE allocation_id=$1::bigint "
                "UNION ALL SELECT "
                    "begin_time, end_time "
                "FROM csm_allocation_history "
                "WHERE allocation_id=$1::bigint ) a, csm_ras_event_action r "
            "WHERE r.time_stamp >= a.begin_time "
                "AND (a.end_time is NULL OR r.time_stamp <= a.end_time) "
            "ORDER BY r.rec_id ";

        // =====================================================================
	    csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, paramCount);
        dbReq->AddNumericParam<int64_t>(input->allocation_id);
	    
	    *dbPayload = dbReq;

	    csm_free_struct_ptr(INPUT_STRUCT, input);
    }
    else
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength
                            << " stringBuffer = " << stringBuffer.c_str();

		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed");
		return false;
	}
	
	
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMIRasEventQueryAllocation::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char** stringBuffer,
	uint32_t &bufferLength,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
	LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *stringBuffer = nullptr;
    bufferLength = 0;

    uint32_t numRecords = tuples.size();

    if ( numRecords > 0 )
    {
        OUTPUT_STRUCT output;
        csm_init_struct_versioning(&output);
        output.num_events = numRecords;
        output.events = 
            (DB_RECORD_STRUCT**) malloc( numRecords * sizeof(DB_RECORD_STRUCT*));
    
        for ( uint32_t i = 0; i < numRecords; ++i )
        {
            CreateOutputStruct( tuples[i], &output.events[i] );
        }

        csm_serialize_struct( OUTPUT_STRUCT, &output, stringBuffer, &bufferLength);
	    
        csm_free_struct( OUTPUT_STRUCT, output );
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    return true;
}

void CSMIRasEventQueryAllocation::CreateOutputStruct(
	csm::db::DBTuple * const & fields,
    DB_RECORD_STRUCT **output)
{
	LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";

	// Error check
	if(fields->nfields != 9){
		*output = nullptr;
		return;
	}

	DB_RECORD_STRUCT *o = nullptr;
    csm_init_struct_ptr(DB_RECORD_STRUCT, o);
	
    o->rec_id          = strtoll(fields->data[0], nullptr, 10);
	o->msg_id          = strdup(fields->data[1]);
    o->msg_id_seq      = strtol(fields->data[2], nullptr, 10);
    o->time_stamp      = strdup(fields->data[3]);
    o->location_name   = strdup(fields->data[4]);
    o->count           = strtol(fields->data[5], nullptr, 10);
    o->message         = strdup(fields->data[6]);
    o->raw_data        = strdup(fields->data[7]);
    o->archive_history_time = strdup(fields->data[8]);

	*output = o;
	
	LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
	
	return;
}

