/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasEventQuery.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
// implement the CSM api ras event query command...
//

/* ## INCLUDES ## */
/* Header for this file. */
#include "CSMIRasEventQuery.h"
/* ## DEFINES ## */
//Used for debug prints
#define STATE_NAME "CSMIRasEventQuery:"
// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ras_event_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ras_event_query_output_t
#define DB_RECORD_STRUCT csmi_ras_event_t

using namespace std;

std::string CSMIRasEventQuery::trim(const std::string& str)
{
    if (str.size() == 0)
        return(str);
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    if (first == std::string::npos)      // completly blank??
        return("");
    if (last == std::string::npos)
        return(str.substr(first, std::string::npos));
    return str.substr(first, (last-first+1));
}

void CSMIRasEventQuery::addWhere(std::ostringstream &where, 
                                 const std::string &f, 
                                 const char *v)
{
    if ((v) && (*v)) {
        string t = v;
        std::replace( t.begin(), t.end(), '\'', '_' );      // don't allow quote characters, 
                                                            // this will guard against sql injection..
        if (where.str().size()) where << " AND ";
        where << f << " LIKE " << "'" << t << "'";
    }

}

bool CSMIRasEventQuery::CreatePayload(
    const std::string& stringBuffer,
    const uint32_t bufferLength,
    csm::db::DBReqContent** dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

	/*Unpack the buffer.*/
	API_PARAMETER_INPUT_TYPE* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0 )
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength
                            << " stringBuffer = " << stringBuffer.c_str();

		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed");
		return false;
	}

    // =====================================================================
	std::string stmtParams = "";
	int SQLparameterCount = 0;
	
	add_param_sql(stmtParams, input->msg_id[0]           != '\0', ++SQLparameterCount, "msg_id LIKE $","::text AND ")
	add_param_sql(stmtParams, input->severity > CSM_RAS_NO_SEV && input->severity < csm_enum_max(csmi_ras_severity_t), ++SQLparameterCount, "severity = $","::ras_event_severity AND ")
	add_param_sql(stmtParams, input->location_name[0]    != '\0', ++SQLparameterCount, "location_name LIKE $","::text AND ")
	add_param_sql(stmtParams, input->control_action[0]   != '\0', ++SQLparameterCount, "control_action LIKE $","::text AND ")
    add_param_sql(stmtParams, input->message[0]          != '\0', ++SQLparameterCount, "message LIKE $","::text AND ")
	add_param_sql(stmtParams, input->start_time_stamp[0] != '\0', ++SQLparameterCount, "time_stamp >= $","::timestamp AND ")	
	add_param_sql(stmtParams, input->end_time_stamp[0]   != '\0', ++SQLparameterCount, "time_stamp <= $","::timestamp AND ")	
	add_param_sql(stmtParams, input->master_time_stamp_search_begin[0] != '\0', ++SQLparameterCount, "master_time_stamp >= $","::timestamp AND ")	
	add_param_sql(stmtParams, input->master_time_stamp_search_end[0]   != '\0', ++SQLparameterCount, "master_time_stamp <= $","::timestamp AND ")	
	add_param_sql(stmtParams, input->rec_id > 0, ++SQLparameterCount, "rec_id = $","::bigint AND ")	

    // TODO should this fail if the parameter count is zero?
    // Replace the last 4 characters if any parameters were found.
    if ( SQLparameterCount  > 0)
    {
        int len = stmtParams.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams[i] = ' ';
    }
	// =====================================================================
	
	/*Open "std::string stmt"*/
	std::string stmt = 
		"SELECT " 
			"rec_id,"
			"msg_id,"
			"severity,"
			"time_stamp,"
			"master_time_stamp,"
			"location_name,"
			"count,"
			"control_action,"
			"message,"
			"raw_data, "
			"kvcsv "
		"FROM csm_ras_event_action_view "
		"WHERE (";
	stmt.append( stmtParams );
    stmt.append(") "
		"ORDER BY ");
		switch (input->order_by)
		{
			case 'a':
				stmt.append("rec_id ASC NULLS LAST ");
				break;
			case 'b':
				stmt.append("rec_id DESC NULLS LAST ");
				break;
			case 'c':
				stmt.append("time_stamp ASC NULLS LAST ");
				break;
			case 'd':
				stmt.append("time_stamp DESC NULLS LAST ");
				break;
			case 'e':
				stmt.append("master_time_stamp ASC NULLS LAST ");
				break;
			case 'f':
				stmt.append("master_time_stamp DESC NULLS LAST ");
				break;
			case 'g':
				stmt.append("location_name ASC NULLS LAST ");
				break;
			case 'h':
				stmt.append("location_name DESC NULLS LAST ");
				break;
			case 'i':
				stmt.append("msg_id ASC NULLS LAST ");
				break;
			case 'j':
				stmt.append("msg_id DESC NULLS LAST ");
				break;
			case 'k':
				stmt.append("severity ASC NULLS LAST ");
				break;
			case 'l':
				stmt.append("severity DESC NULLS LAST ");
				break;
			default:
				stmt.append("rec_id ASC NULLS LAST ");
		}
	add_param_sql( stmt, input->limit > 0, ++SQLparameterCount, "LIMIT $", "::int ")
    add_param_sql( stmt, input->offset > 0, ++SQLparameterCount, "OFFSET $", "::int ")
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	
	if(input->msg_id[0]           != '\0') dbReq->AddTextParam(input->msg_id);
	if(input->severity > CSM_RAS_NO_SEV && input->severity < csm_enum_max(csmi_ras_severity_t)) dbReq->AddTextParam(csm_get_string_from_enum(csmi_ras_severity_t, input->severity) );
	if(input->location_name[0]    != '\0') dbReq->AddTextParam(input->location_name);
	if(input->control_action[0]   != '\0') dbReq->AddTextParam(input->control_action);
	if(input->message[0]          != '\0') dbReq->AddTextParam(input->message);
	if(input->start_time_stamp[0] != '\0') dbReq->AddTextParam(input->start_time_stamp);
	if(input->end_time_stamp[0]   != '\0') dbReq->AddTextParam(input->end_time_stamp);
	if(input->master_time_stamp_search_begin[0] != '\0') dbReq->AddTextParam(input->master_time_stamp_search_begin);
	if(input->master_time_stamp_search_end[0]   != '\0') dbReq->AddTextParam(input->master_time_stamp_search_end);
	if(input->rec_id > 0)                  dbReq->AddNumericParam<int64_t>(input->rec_id);
	if(input->limit > 0)                   dbReq->AddNumericParam<int>(input->limit);
	if(input->offset > 0)                  dbReq->AddNumericParam<int>(input->offset);
	
	*dbPayload = dbReq;
	
	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi, debug) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
	
	LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

// return the error code defined in csmi/src/common/include/csmi_cmd_error.h
bool CSMIRasEventQuery::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char** stringBuffer,
	uint32_t &bufferLength,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Enter";
	
	*stringBuffer = NULL;
    bufferLength = 0;
	
	/*If we want to return stuff*/
	/*Implement code here*/
	
	/*Helper Variables*/
    uint32_t numberOfRecords = tuples.size();
	char* pEnd;
    
    if ( numberOfRecords > 0 )
	{
		std::vector<RasEvent> rasEventVect;
		for (unsigned n = 0; n < numberOfRecords; n++) {
			csm::db::DBTuple* fields = tuples[n];        // we should only have 1 row, 
			assert(fields->nfields == 11);      // hm.. probably not the best error handling...
			rasEventVect.push_back(RasEvent());
			RasEvent &rasEvent = rasEventVect.back();       // now fill out the element we created...
			rasEvent.setValue(CSM_RAS_FKEY_REC_ID,             fields->data[0]);
			rasEvent.setValue(CSM_RAS_FKEY_MSG_ID,             fields->data[1]);
			rasEvent.setValue(CSM_RAS_FKEY_SEVERITY,           trim(fields->data[2]));
			rasEvent.setValue(CSM_RAS_FKEY_TIME_STAMP,         fields->data[3]);
			rasEvent.setValue(CSM_RAS_FKEY_MASTER_TIME_STAMP,  fields->data[4]);
			rasEvent.setValue(CSM_RAS_FKEY_LOCATION_NAME,      fields->data[5]);
			rasEvent.setValue(CSM_RAS_FKEY_COUNT,              fields->data[6]);
			rasEvent.setValue(CSM_RAS_FKEY_CONTROL_ACTION,     fields->data[7]);
			rasEvent.setValue(CSM_RAS_FKEY_MESSAGE,            fields->data[8]);
			rasEvent.setValue(CSM_RAS_FKEY_RAW_DATA,           fields->data[9]);
			rasEvent.setValue(CSM_RAS_FKEY_KVCSV,              fields->data[10]);
			
		}

		// stl type container to contain string pointers...
		/*
		typedef struct {
			string msg_id;
			string severity; 
			string time_stamp;
			string location_name;
			unsigned int count;
			string control_action;
			string message;
			string raw_data;
		} csm_ras_event_stl_t;
		*/
		uint32_t numberOfRecords = rasEventVect.size();
		
		API_PARAMETER_OUTPUT_TYPE *output = nullptr;
		csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);

		output->results_count = numberOfRecords;
		output->results = (DB_RECORD_STRUCT**)calloc(output->results_count, sizeof(DB_RECORD_STRUCT*));

		//DB_RECORD_STRUCT *rcsm_ras = nullptr;
		//csm_init_struct_ptr(DB_RECORD_STRUCT, rcsm_ras);

		LOG(csmapi, debug) << "output->results_count = " << output->results_count << endl;

		for (uint32_t n = 0; n < rasEventVect.size(); n++) {
			RasEvent &rasEvent = rasEventVect[n];

			DB_RECORD_STRUCT *rcsm_ras = nullptr;
			csm_init_struct_ptr(DB_RECORD_STRUCT, rcsm_ras);

			rcsm_ras->rec_id             = strtoll(rasEvent.getValue(CSM_RAS_FKEY_REC_ID).c_str(), &pEnd, 10); if(pEnd == rasEvent.getValue(CSM_RAS_FKEY_REC_ID).c_str()) rcsm_ras->rec_id = -1.0;
			rcsm_ras->msg_id             = strdup(rasEvent.getValue(CSM_RAS_FKEY_MSG_ID).c_str());
			rcsm_ras->severity           = strdup(rasEvent.getValue(CSM_RAS_FKEY_SEVERITY).c_str());
			rcsm_ras->time_stamp         = strdup(rasEvent.getValue(CSM_RAS_FKEY_TIME_STAMP).c_str());
			rcsm_ras->master_time_stamp  = strdup(rasEvent.getValue(CSM_RAS_FKEY_MASTER_TIME_STAMP).c_str());
			rcsm_ras->location_name      = strdup(rasEvent.getValue(CSM_RAS_FKEY_LOCATION_NAME).c_str());
			rcsm_ras->count              = atoi(rasEvent.getValue(CSM_RAS_FKEY_COUNT).c_str());
			rcsm_ras->control_action     = strdup(rasEvent.getValue(CSM_RAS_FKEY_CONTROL_ACTION).c_str());
			rcsm_ras->message            = strdup(rasEvent.getValue(CSM_RAS_FKEY_MESSAGE).c_str());
			rcsm_ras->raw_data           = strdup(rasEvent.getValue(CSM_RAS_FKEY_RAW_DATA).c_str());
			rcsm_ras->kvcsv              = strdup(rasEvent.getValue(CSM_RAS_FKEY_KVCSV).c_str());
			
			
			output->results[n] = rcsm_ras;
		}

		csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
		csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
	}
        
	LOG(csmapi, trace) << STATE_NAME ":CreateByteArray: Exit";
	return true;
}
