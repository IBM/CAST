/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIIbCableQuery.cc

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
#include "CSMIIbCableQuery.h"

//Used for debug prints
#define STATE_NAME "CSMIIbCableQuery:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_ib_cable_query_input_t
#define API_PARAMETER_OUTPUT_TYPE csm_ib_cable_query_output_t
#define DB_RECORD_STRUCT csmi_ib_cable_record_t 

bool CSMIIbCableQuery::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
    // Unpack the buffer.
    API_PARAMETER_INPUT_TYPE* input = nullptr;

	/* Error in case something went wrong with the unpack*/
	if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, stringBuffer.c_str(), bufferLength) != 0 )
    {
		LOG(csmapi,error) << STATE_NAME ":CreatePayload: csm_deserialize_struct failed...";
		LOG(csmapi,error) << "  bufferLength = " << bufferLength << " stringBuffer = " << stringBuffer.c_str();
		LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";
		ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
		//append to the err msg as to preserve other previous messages.
		ctx->AppendErrorMessage("CreatePayload: csm_deserialize_struct failed...");
		return false;
	}

	// =====================================================================
	// Construct the query's constraints via "WHERE" Statement.

	std::string stmtParams = "";
	int SQLparameterCount = 0;
	
	//where statement parameters
	add_param_sql(stmtParams, input->comments_count > 0, ++SQLparameterCount, "comment LIKE ANY ( $","::text[] ) AND ")
	if(input->guid_count > 0)
	{
		SQLparameterCount++;
		add_param_sql(stmtParams, input->guids_count > 0, SQLparameterCount, "( guid_s1 = ANY ( $","::text[] ) OR ")
		add_param_sql(stmtParams, input->guids_count > 0, SQLparameterCount, "guid_s2 = ANY ( $","::text[] ) ) AND ")
	}
	add_param_sql(stmtParams, input->identifiers_count > 0, ++SQLparameterCount, "identifier LIKE ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->lengths_count > 0, ++SQLparameterCount, "length LIKE ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->names_count > 0, ++SQLparameterCount, "name LIKE ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->part_numbers_count > 0, ++SQLparameterCount, "part_number LIKE ANY ( $","::text[] ) AND ")
	if(input->ports_count > 0)
	{
		SQLparameterCount++;
		add_param_sql(stmtParams, input->ports_count > 0, SQLparameterCount, "( port_s1 = ANY ( $","::text[] ) OR ")
		add_param_sql(stmtParams, input->ports_count > 0, SQLparameterCount, "port_s2 = ANY ( $","::text[] ) ) AND ")
	}
	add_param_sql(stmtParams, input->revisions_count > 0, ++SQLparameterCount, "revision LIKE ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->serial_numbers_count > 0, ++SQLparameterCount, "serial_number = ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->severities_count > 0, ++SQLparameterCount, "severity LIKE ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->types_count > 0, ++SQLparameterCount, "type LIKE ANY ( $","::text[] ) AND ")
	add_param_sql(stmtParams, input->widths_count > 0, ++SQLparameterCount, "width LIKE ANY ( $","::text[] ) AND ")
	
    // Replace the last 4 characters if any parameters were found.
    if ( SQLparameterCount > 0)
    {
        int len = stmtParams.length() - 1;
        for( int i = len - 3; i < len; ++i)
            stmtParams[i] = ' ';
    }

	/*Create the SQL Statement*/
	std::string stmt = 
		"SELECT "
			"serial_number, "
			"discovery_time, "
			"collection_time, "
			"comment, "
			"guid_s1, "
			"guid_s2, "
			"identifier, "
			"length, "
			"name, "
			"part_number, "
			"port_s1, "
			"port_s2, "
			"revision, "
			"severity, "
			"type, "
			"width "
		"FROM ";
	if(SQLparameterCount > 0)
	{
		//Filters have been provided. 
		//Filter query pased off of input.
		stmt.append("WHERE (");
		stmt.append( stmtParams );
		stmt.append(") ");
	}
	//stmt.append("ORDER BY ");
	/*
	switch (input->order_by)
	{
		case 'a':
			stmt.append("switch_name ASC NULLS LAST ");
			break;
		case 'b':
			stmt.append("switch_name DESC NULLS LAST ");
			break;
		default:
			stmt.append("switch_name ASC NULLS LAST ");
	}
	*/
	stmt.append(	"ORDER BY "
			"serial_number "
			"ASC NULLS LAST ");
	add_param_sql( stmt, input->limit > 0, ++SQLparameterCount, "LIMIT $", "::int ")

    add_param_sql( stmt, input->offset > 0, ++SQLparameterCount, "OFFSET $", "::int ")
			
	/* End of SQL statement. */
	
	/* Build the parameterized list. */
	csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
	if ( input->comments_count > 0 ) dbReq->AddTextArrayParam(input->comments, input->comments_count);
	if ( input->guids_count > 0 ) dbReq->AddTextArrayParam(input->guids, input->guids_count);
	if ( input->identifiers_count > 0 ) dbReq->AddTextArrayParam(input->identifiers, input->identifiers_count);
	if ( input->lengths_count > 0 ) dbReq->AddTextArrayParam(input->lengths, input->lengths_count);
	if ( input->names_count > 0 ) dbReq->AddTextArrayParam(input->names, input->names_count);
	if ( input->part_numbers_count > 0 ) dbReq->AddTextArrayParam(input->part_numbers, input->part_numbers_count);
	if ( input->ports_count > 0 ) dbReq->AddTextArrayParam(input->ports, input->ports_count);
	if ( input->revisions_count > 0 ) dbReq->AddTextArrayParam(input->revisions, input->revisions_count);
	if ( input->serial_numbers_count > 0 ) dbReq->AddTextArrayParam(input->serial_numbers, input->serial_numbers_count);
	if ( input->severities_count > 0 ) dbReq->AddTextArrayParam(input->severities, input->severities_count);
	if ( input->types_count > 0 ) dbReq->AddTextArrayParam(input->types, input->types_count);
	if ( input->widths_count > 0 ) dbReq->AddTextArrayParam(input->widths, input->widths_count);
	if(input->limit > 0) dbReq->AddNumericParam<int>(input->limit);
	if(input->offset > 0) dbReq->AddNumericParam<int>(input->offset);

	*dbPayload = dbReq;
	/* Release memory using CSM API function. */
	csm_free_struct_ptr( API_PARAMETER_INPUT_TYPE, input );
	/*Print the SQL statement to the log for reference. */
	LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
    
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMIIbCableQuery::CreateByteArray(
		const std::vector<csm::db::DBTuple *>&tuples, 
		char **stringBuffer, 
		uint32_t &bufferLength, 
		csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    // Init the buffer
    *stringBuffer = nullptr;
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
        for(uint32_t i = 0; i < numberOfRecords; i++){
            CreateOutputStruct(tuples[i], &(output->results[i]));
        }
        
		// Pack the allocation up.
        csm_serialize_struct(API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength);
        
		// Free struct we made.
        csm_free_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
    }    

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    
    return true;
}

void CSMIIbCableQuery::CreateOutputStruct(
    csm::db::DBTuple * const & fields, 
    DB_RECORD_STRUCT **record)
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";
    
	// Error check
    if ( fields->nfields != 16 )
    {
        *record = nullptr;
        return;
    }
	
	// convert from DB tuple results to c data structure
	
	/*Set up data to call API*/
    DB_RECORD_STRUCT *r = nullptr;
	/* CSM API initialize and malloc function*/
    csm_init_struct_ptr(DB_RECORD_STRUCT, r);
	
	r->serial_number   = strdup(fields->data[0]);
	r->discovery_time  = strdup(fields->data[1]);
	r->collection_time = strdup(fields->data[2]);
	r->comment         = strdup(fields->data[3]);
	r->guid_s1         = strdup(fields->data[4]);
	r->guid_s2         = strdup(fields->data[5]);
	r->identifier      = strdup(fields->data[6]);
	r->length          = strdup(fields->data[7]);
	r->name            = strdup(fields->data[8]);
	r->part_number     = strdup(fields->data[9]);
	r->port_s1         = strdup(fields->data[10]);
	r->port_s2         = strdup(fields->data[11]);
	r->revision        = strdup(fields->data[12]);
	r->severity        = strdup(fields->data[13]);
	r->type            = strdup(fields->data[14]);
	r->width           = strdup(fields->data[15]);
	
	*record = r;

    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}
