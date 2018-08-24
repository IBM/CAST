/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMINodeFindJob.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

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
#include "CSMINodeFindJob.h" 

//Used for debug prints
#define STATE_NAME "CSMINodeFindJob:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_node_find_job_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_node_find_job_output_t
#define DB_RECORD_STRUCT csmi_node_find_job_record_t 

bool CSMINodeFindJob::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    /*Unpack the buffer.*/
	API_PARAMETER_INPUT_TYPE* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0)
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed...";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength << " stringBuffer = " 
            << stringBuffer.c_str();
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		//append to the err msg as to preserve other previous messages.
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		return false;
	}
	
	// =====================================================================
	//statements to construct
	std::string stmtParams = "";
	std::string stmtParams_history_only = "";
	std::string stmtParams_live_range = "";
	std::string stmtParams_history_range = "";
	std::string stmtParams_live_midpoint = "";
	std::string stmtParams_history_midpoint = "";
	std::string midpoint_search_begin = "";
	std::string midpoint_search_end = "";
	//counter
	int SQLparameterCount = 0;
	//var spot placeholders
	int SQLNum_node_names = 0;
	int SQLNum_begin_time_search_begin = 0;
	int SQLNum_begin_time_search_end = 0;
	int SQLNum_end_time_search_begin = 0;
	int SQLNum_end_time_search_end = 0;
	int SQLNum_midpoint = 0;
	int SQLNum_midpoint_interval = 0;
	int SQLNum_search_range_begin = 0;
	int SQLNum_search_range_end = 0;
	int SQLNum_user_name = 0;
	
	//save placeholders
	if(input->node_names_count > 0)
	{
		SQLparameterCount++;
		SQLNum_node_names = SQLparameterCount;
	}
	if(input->begin_time_search_begin[0] != '\0')
	{
		SQLparameterCount++;
		SQLNum_begin_time_search_begin = SQLparameterCount;
	}
	
	if(input->begin_time_search_end[0] != '\0')
	{
		SQLparameterCount++;
		SQLNum_begin_time_search_end = SQLparameterCount;
	}
	if(input->end_time_search_begin[0] != '\0')
	{
		SQLparameterCount++;
		SQLNum_end_time_search_begin = SQLparameterCount;
	}
	
	if(input->end_time_search_end[0] != '\0')
	{
		SQLparameterCount++;
		SQLNum_end_time_search_end = SQLparameterCount;
	}

	//if midpoint or interval input, then create a search range from these values	
	
	if(input->midpoint[0] != '\0' && input->midpoint_delta[0] != '\0')
	{
		//increase the SQL parameter counters
		SQLparameterCount++;
		SQLNum_midpoint = SQLparameterCount;
		SQLparameterCount++;
		SQLNum_midpoint_interval = SQLparameterCount;
		
		// if mid point and interval 
		//t1
		midpoint_search_begin.append(" ($");
		midpoint_search_begin.append(std::to_string(SQLNum_midpoint));
		midpoint_search_begin.append("::timestamp - $");
		midpoint_search_begin.append(std::to_string(SQLNum_midpoint_interval));
		midpoint_search_begin.append("::interval");
		midpoint_search_begin.append(") ");
		
		//t2
		midpoint_search_end.append(" ($");
		midpoint_search_end.append(std::to_string(SQLNum_midpoint));
		midpoint_search_end.append("::timestamp + $");
		midpoint_search_end.append(std::to_string(SQLNum_midpoint_interval));
		midpoint_search_end.append("::interval");
		midpoint_search_end.append(") ");
		
	}else if(input->midpoint[0] != '\0' && input->midpoint_delta[0] == '\0')
	{
		//if mid point but no interval
		// no range
		
		//increase the SQL parameter counters
		SQLparameterCount++;
		SQLNum_midpoint = SQLparameterCount;
		
		//t1
		midpoint_search_begin.append(" ($");
		midpoint_search_begin.append(std::to_string(SQLNum_midpoint));
		midpoint_search_begin.append("::timestamp ");
		midpoint_search_begin.append(") ");
		
		//t2
		midpoint_search_end.append(" ($");
		midpoint_search_end.append(std::to_string(SQLNum_midpoint));
		midpoint_search_end.append("::timestamp ");
		midpoint_search_end.append(") ");
		
	}else if(input->midpoint[0] == '\0' && input->midpoint_delta[0] != '\0')
	{
		//if interval but no mid point 
		//time is now
		
		//increase the SQL parameter counters
		SQLparameterCount++;
		SQLNum_midpoint_interval = SQLparameterCount;
		
		//t1
		midpoint_search_begin.append(" ('now' - $");
		midpoint_search_begin.append(std::to_string(SQLNum_midpoint_interval));
		midpoint_search_begin.append("::interval");
		midpoint_search_begin.append(") ");
		
		//t2
		midpoint_search_end.append(" ('now' + $");
		midpoint_search_end.append(std::to_string(SQLNum_midpoint_interval));
		midpoint_search_end.append("::interval");
		midpoint_search_end.append(") ");
		
	}
	if(input->search_range_begin[0] != '\0')
	{
		SQLparameterCount++;
		SQLNum_search_range_begin = SQLparameterCount;
	}
	if(input->search_range_end[0] != '\0')
	{
		SQLparameterCount++;
		SQLNum_search_range_end = SQLparameterCount;
	}
	if(input->user_name[0] != '\0')
	{
		SQLparameterCount++;
		SQLNum_user_name = SQLparameterCount;
	}
	
	//for the standard WHERE
	add_param_sql( stmtParams, input->node_names_count > 0, SQLNum_node_names, "node_name = ANY ( $","::text[] ) AND ")
	add_param_sql( stmtParams, input->user_name [0] != '\0', SQLNum_user_name, "user_name = $","::text AND ")
	add_param_sql( stmtParams, input->begin_time_search_begin [0] != '\0', SQLNum_begin_time_search_begin, "begin_time >= $", "::timestamp AND ")
	add_param_sql( stmtParams, input->begin_time_search_end [0] != '\0', SQLNum_begin_time_search_end, "begin_time <= $", "::timestamp AND ")
	
	// TODO should this fail if the parameter count is zero?
    // Replace the last 4 characters if any parameters were found.
    if ( SQLNum_node_names > 0 || SQLNum_user_name > 0 || SQLNum_begin_time_search_begin > 0 || SQLNum_begin_time_search_end > 0)
    {
        int len = stmtParams.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams[i] = ' ';
    }
	
	//history expansion where
	add_param_sql( stmtParams_history_only, input->end_time_search_begin [0] != '\0', SQLNum_end_time_search_begin, "end_time >= $", "::timestamp AND ")
	add_param_sql( stmtParams_history_only, input->end_time_search_end [0] != '\0', SQLNum_end_time_search_end, "end_time <= $", "::timestamp AND ")
	 // Replace the last 4 characters if any parameters were found.
    if ( SQLNum_end_time_search_begin > 0 || SQLNum_end_time_search_end > 0)
    {
        int len = stmtParams_history_only.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams_history_only[i] = ' ';
    }
	
	//for the extra WHERE
	add_param_sql( stmtParams_live_range, input->search_range_begin[0] != '\0', SQLNum_search_range_begin, " begin_time >= $", "::timestamp AND ")
	add_param_sql( stmtParams_live_range, input->search_range_end[0] != '\0', SQLNum_search_range_end, " begin_time <= $", "::timestamp AND ")
    // Replace the last 4 characters if any parameters were found.
    if ( SQLNum_search_range_begin > 0 || SQLNum_search_range_end > 0)
    {
        int len = stmtParams_live_range.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams_live_range[i] = ' ';
    }
	
	//for the extra WHERE
	add_param_sql( stmtParams_history_range, input->search_range_begin[0] != '\0', SQLNum_search_range_begin, " end_time >= $", "::timestamp AND ")
	add_param_sql( stmtParams_history_range, input->search_range_end[0] != '\0', SQLNum_search_range_end, " end_time <= $", "::timestamp AND ")
    // Replace the last 4 characters if any parameters were found.
    if ( SQLNum_search_range_begin > 0 || SQLNum_search_range_end > 0)
    {
        int len = stmtParams_history_range.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams_history_range[i] = ' ';
    }
	
	
	
	//where based off a midpoint
	if(midpoint_search_begin.length() > 3)
	{
		stmtParams_live_midpoint.append(" begin_time >= ");
		stmtParams_live_midpoint.append(midpoint_search_begin);
		stmtParams_live_midpoint.append(" AND ");
	}
	if(midpoint_search_end.length() > 3)
	{
		stmtParams_live_midpoint.append(" begin_time <= ");
		stmtParams_live_midpoint.append(midpoint_search_end);
		stmtParams_live_midpoint.append(" AND ");
	}
    // Replace the last 4 characters if any parameters were found.
    if ( SQLNum_midpoint > 0 || SQLNum_midpoint_interval > 0)
    {
        int len = stmtParams_live_midpoint.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams_live_midpoint[i] = ' ';
    }
	
	if(midpoint_search_begin.length() > 3)
	{
		stmtParams_history_midpoint.append(" end_time >= ");
		stmtParams_history_midpoint.append(midpoint_search_begin);
		stmtParams_history_midpoint.append(" AND ");
	}
	if(midpoint_search_end.length() > 3)
	{
		stmtParams_history_midpoint.append(" end_time <= ");
		stmtParams_history_midpoint.append(midpoint_search_end);
		stmtParams_history_midpoint.append(" AND ");
	}
    // Replace the last 4 characters if any parameters were found.
    if ( SQLNum_midpoint > 0 || SQLNum_midpoint_interval > 0)
    {
        int len = stmtParams_history_midpoint.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams_history_midpoint[i] = ' ';
    }
	
	
	
	
	
	
	/*Open "std::string stmt"*/
	std::string stmt = "";
	
	//if there are specific end time constraints, then don't search the live table. 
	if ( SQLNum_end_time_search_begin == 0 && SQLNum_end_time_search_end == 0)
	{
		stmt.append(
		"SELECT "
			"an.node_name, "
			"an.allocation_id, "
			"a.primary_job_id, "
			"a.user_name, "
			"a.num_nodes, "
			"a.begin_time, "
			"NULL as end_time "
		"FROM "
			"csm_allocation_node AS an "
		"INNER JOIN "
			"csm_allocation AS a "
			"ON an.allocation_id = a.allocation_id "
		"WHERE (");
			//statement generated above
			stmt.append( stmtParams );
			if(SQLNum_search_range_end > 0)
			{
				stmt.append(" AND "
				"("
					"(");
						add_param_sql( stmt, input->search_range_end[0] != '\0', SQLNum_search_range_end, " begin_time <= $", "::timestamp ")
				stmt.append(
					")"
				")"
				);
			}
			if(midpoint_search_end.length() > 3)
			{
				stmt.append(" AND "
				"("
					"(");
						//add_param_sql( stmt, midpoint_search_end.length() > 3, midpoint_search_end, " begin_time <= ", " ")
						if(midpoint_search_end.length() > 3)
						{
							stmt.append(" begin_time <= ");
							stmt.append(midpoint_search_end);
							stmt.append(" ");
						}
				stmt.append(
					")"
				")"
				);
			}
		stmt.append(") "
		"UNION "
		);
		
	}
	//regardless. append the history table search
		stmt.append(
		"SELECT DISTINCT "
			"anh.node_name, "
			"anh.allocation_id, "
			"ah.primary_job_id, "
			"ah.user_name, "
			"ah.num_nodes, "
			"ah.begin_time, "
			"ah.end_time "
		"FROM "
			"csm_allocation_node_history AS anh "
		"INNER JOIN "
			"csm_allocation_history AS ah "
			"ON anh.allocation_id = ah.allocation_id "
		"WHERE (");
			//statement generated above
			stmt.append( stmtParams );
			if ( SQLNum_end_time_search_begin > 0 || SQLNum_end_time_search_end > 0)
			{
				stmt.append(" AND ");
				stmt.append( stmtParams_history_only );
			}
			
			if(SQLNum_search_range_begin > 0 || SQLNum_search_range_end > 0)
			{
				stmt.append(" AND "
				"("
					"("); 
						stmt.append(stmtParams_history_range);
					stmt.append(
					") OR "
					"(");
						stmt.append(stmtParams_live_range);
					stmt.append(
					")");
					if(SQLNum_search_range_begin > 0 && SQLNum_search_range_end > 0)
					{
						stmt.append(
						" OR "
						"(");
							add_param_sql( stmt, input->search_range_begin[0] != '\0', SQLNum_search_range_begin, " begin_time <= $", "::timestamp AND ")
							add_param_sql( stmt, input->search_range_end[0] != '\0', SQLNum_search_range_end, " end_time >= $", "::timestamp ")
						stmt.append(")");
					}
				stmt.append(
				")");
			}
			if(SQLNum_midpoint > 0 || SQLNum_midpoint_interval > 0)
			{
				stmt.append(" AND "
				"("
					"("); 
						stmt.append(stmtParams_history_midpoint);
					stmt.append(
					") OR "
					"(");
						stmt.append(stmtParams_live_midpoint);
					stmt.append(
					")");
					if(midpoint_search_begin.length() > 3 && midpoint_search_end.length() > 3)
					{
						stmt.append(
						" OR "
						"(");
							if(midpoint_search_begin.length() > 3)
							{
								stmt.append(" begin_time <= ");
								stmt.append(midpoint_search_begin);
								stmt.append(" ");
							}
							stmt.append(" AND ");
							if(midpoint_search_end.length() > 3)
							{
								stmt.append(" end_time >= ");
								stmt.append(midpoint_search_end);
								stmt.append(" ");
							}
						stmt.append(")");
					}
				stmt.append(
				")");
			}
		stmt.append(")"
		"ORDER BY "
			"node_name, "
			"allocation_id "
			"ASC NULLS LAST ");
		add_param_sql( stmt, input->limit > 0, ++SQLparameterCount,
            "LIMIT $", "::int ")
		add_param_sql( stmt, input->offset > 0, ++SQLparameterCount,
            "OFFSET $", "::int ")
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	
	if(input->node_names_count            > 0    ) dbReq->AddTextArrayParam(input->node_names, input->node_names_count);
	if(input->begin_time_search_begin[0]  != '\0') dbReq->AddTextParam(input->begin_time_search_begin);
	if(input->begin_time_search_end[0]    != '\0') dbReq->AddTextParam(input->begin_time_search_end);
	if(input->end_time_search_begin[0]    != '\0') dbReq->AddTextParam(input->end_time_search_begin);
	if(input->end_time_search_end[0]      != '\0') dbReq->AddTextParam(input->end_time_search_end);
	if(SQLNum_midpoint                    > 0    ) dbReq->AddTextParam(input->midpoint);
	if(SQLNum_midpoint_interval           > 0    ) dbReq->AddTextParam(input->midpoint_delta);
	if(input->search_range_begin[0]       != '\0') dbReq->AddTextParam(input->search_range_begin);
	if(input->search_range_end[0]         != '\0') dbReq->AddTextParam(input->search_range_end);
	if(input->user_name[0]                != '\0') dbReq->AddTextParam(input->user_name);
	//after the placeholders we made above
	if(input->limit                       > 0    ) dbReq->AddNumericParam<int>(input->limit);
	if(input->offset                      > 0    ) dbReq->AddNumericParam<int>(input->offset);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi, debug) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMINodeFindJob::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

	*stringBuffer = NULL;
    bufferLength = 0;
	
	/*Helper Variables*/
	uint32_t numberOfRecords = tuples.size();

	if(numberOfRecords > 0)
    {
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

void CSMINodeFindJob::CreateOutputStruct(
    csm::db::DBTuple * const & fields,
    DB_RECORD_STRUCT **output )
{
        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";

		// Error check
        if(fields->nfields != 7){
            *output = nullptr;
            return;
        }
		
		/*Helper Variables*/
		char* pEnd; // comparison pointer for data conversion check.
		int i = 0; //keep place in data counter
        
		/*Set up data to call API*/
        DB_RECORD_STRUCT *o = nullptr;
		/* CSM API initialize and malloc function*/
        csm_init_struct_ptr(DB_RECORD_STRUCT, o);

        o->node_name      = strdup(fields->data[i]);                                                                     i++;
		o->allocation_id  = strtoll(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->allocation_id = -1.0;}  i++;
		o->primary_job_id = strtoll(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->primary_job_id = -1.0;} i++;
		o->user_name      = strdup(fields->data[i]);                                                                     i++;
		o->num_nodes      = strtol(fields->data[i], &pEnd, 10); if(pEnd == fields->data[i]){ o->num_nodes = -1.0;}       i++;
		o->begin_time     = strdup(fields->data[i]);                                                                     i++;
		o->end_time       = strdup(fields->data[i]);                                                                     i++;

        *output = o;

        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
}
