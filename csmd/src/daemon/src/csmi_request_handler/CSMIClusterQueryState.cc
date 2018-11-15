/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIClusterQueryState.cc

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
#include "CSMIClusterQueryState.h"

//Used for debug prints
#define STATE_NAME "CSMIClusterQueryState:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_cluster_query_state_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_cluster_query_state_output_t
#define DB_RECORD_STRUCT csmi_cluster_query_state_record_t 

bool CSMIClusterQueryState::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
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
	std::string stmtParams = "";
	int SQLparameterCount = 0;
	
	//num allocs is special. don't add it here. need to use the "HAVING" SQL key word
	//add_param_sql(stmtParams, input->num_allocs >= 0, ++SQLparameterCount, "num_allocs = $","::int AND ")
	add_param_sql(stmtParams, input->state && input->state < csm_enum_max(csmi_node_state_t), ++SQLparameterCount, "n.state = $", "::compute_node_states  AND ")
	add_param_sql(stmtParams, input->type && input->type < csm_enum_max(csmi_node_type_t), ++SQLparameterCount, "n.type = $", "::text AND ")
		
		
	// TODO should this fail if the parameter count is zero?
    // Replace the last 4 characters if any parameters were found.
    if ( SQLparameterCount > 0)
    {
        int len = stmtParams.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams[i] = ' ';
    }
	
	//ToDo: turn this into a DB function.
	/*Open "std::string stmt"*/
	std::string stmt = 
		"SELECT "
			"n.node_name, n.collection_time, n.update_time, n.state, n.type, COUNT(an.allocation_id) AS num_allocs, array_agg(an.allocation_id) AS allocs, array_agg(an.state) AS states, array_agg(an.shared) AS shared "
		"FROM csm_node AS n "
		"LEFT JOIN csm_allocation_node AS an ON an.node_name = n.node_name ";
		if(SQLparameterCount > 0)
		{
		stmt.append("WHERE (");
			stmt.append( stmtParams );
		stmt.append(") ");
		}
		stmt.append("GROUP BY n.node_name ");
		if(input->num_allocs >= 0)
		{
			stmt.append("HAVING ( COUNT(an.allocation_id) = $");
			stmt.append(std::to_string(++SQLparameterCount));
			stmt.append("::int ) ");
		}
		stmt.append("ORDER BY ");
		switch (input->order_by)
		{
			case 'a':
				stmt.append("node_name ASC NULLS LAST ");
				break;
			case 'b':
				stmt.append("node_name DESC NULLS LAST ");
				break;
			case 'c':
				stmt.append("state ASC NULLS LAST ");
				break;
			case 'd':
				stmt.append("state DESC NULLS LAST ");
				break;
			case 'e':
				stmt.append("type ASC NULLS LAST ");
				break;
			case 'f':
				stmt.append("type DESC NULLS LAST ");
				break;
			case 'g':
				stmt.append("num_allocs ASC NULLS LAST ");
				break;
			case 'h':
				stmt.append("num_allocs DESC NULLS LAST ");
				break;
			default:
				stmt.append("node_name ASC NULLS LAST ");
		}
		add_param_sql( stmt, input->limit > 0, ++SQLparameterCount,
            "LIMIT $", "::int ")
		add_param_sql( stmt, input->offset > 0, ++SQLparameterCount,
            "OFFSET $", "::int ")
	/*Close "std::string stmt"*/
	
	// Build the parameterized list.
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	if(input->state && input->state < csm_enum_max(csmi_node_state_t)) dbReq->AddTextParam  (csm_get_string_from_enum(csmi_node_state_t, input->state));
	if(input->type && input->type < csm_enum_max(csmi_node_type_t)) dbReq->AddTextParam  (csm_get_string_from_enum(csmi_node_type_t, input->type));
	if(input->num_allocs >= 0 ) dbReq->AddNumericParam<int>(input->num_allocs);
	if(input->limit      > 0  ) dbReq->AddNumericParam<int>(input->limit);
	if(input->offset     > 0  ) dbReq->AddNumericParam<int>(input->offset);
	
	*dbPayload = dbReq;

	//release memory using CSM API function
	csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);
	
	LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;

    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMIClusterQueryState::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer,
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
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

void CSMIClusterQueryState::CreateOutputStruct(
    csm::db::DBTuple * const & fields,
    DB_RECORD_STRUCT **output )
{
        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Enter";

		// Error check -- bad error check
        if(fields->nfields != 9){
            *output = nullptr;
            return;
        }
		
		/*Helper Variables*/
		//char* pEnd; // comparison pointer for data conversion check.
		int d = 0; //keep place in data counter
		
		csm_prep_csv_to_struct();
        
		/*Set up data to call API*/
        DB_RECORD_STRUCT *o = nullptr;
		/* CSM API initialize and malloc function*/
        csm_init_struct_ptr(DB_RECORD_STRUCT, o);

        o->node_name               = strdup(fields->data[d]);                                                        d++;
		o->collection_time         = strdup(fields->data[d]);                                                        d++;
		o->update_time             = strdup(fields->data[d]);                                                        d++;
		o->state                   = (csmi_node_state_t)csm_get_enum_from_string(csmi_node_state_t,fields->data[d]); d++;
        o->type                    = (csmi_node_type_t)csm_get_enum_from_string(csmi_node_type_t,fields->data[d]);   d++;
		o->num_allocs              = strtoul(fields->data[d], NULL, 10); d++;
		if(o->num_allocs > 0)
		{
			o->allocs = (int64_t*)malloc(o->num_allocs * sizeof(int64_t));
			o->states = (char**)malloc(o->num_allocs * sizeof(char*));
			o->shared = (char**)malloc(o->num_allocs * sizeof(char*));

		    for(uint32_t j = 0; j < o->num_allocs; j++)
		    {
				o->allocs[j] = 0;
				o->states[j] = NULL;
				o->shared[j] = NULL;
		    }
		    
            csm_parse_psql_array_to_struct( fields->data[d], o->allocs, o->num_allocs, CSM_NO_MEMBER, -1, csm_to_int64);      d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->states, o->num_allocs, CSM_NO_MEMBER, strdup("N/A"), strdup); d++;
            csm_parse_psql_array_to_struct( fields->data[d], o->shared, o->num_allocs, CSM_NO_MEMBER, strdup("N/A"), strdup); d++;
		}
		
		
       
	    
		

        *output = o;

        LOG(csmapi, trace) << STATE_NAME ":CreateOutputStruct: Exit";
}
