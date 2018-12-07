/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMINodeDelete.cc

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
#include "CSMINodeDelete.h"
/* Includes for inventory structs and helper functions for those structs including serialization. */
#include "csmi/include/csmi_type_inv.h"
#include "csmi/include/csmi_type_inv_funct.h"

#include "include/csm_event_type_definitions.h"

#define STATE_NAME "CSMINodeDelete:"

// Use this to make changing struct names easier.
#define API_PARAMETER_INPUT_TYPE csm_node_delete_input_t
#define API_PARAMETER_OUTPUT_TYPE  csm_node_delete_output_t


bool CSMINodeDelete::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    API_PARAMETER_INPUT_TYPE* input = nullptr;

    if( csm_deserialize_struct(API_PARAMETER_INPUT_TYPE, &input, arguments.c_str(), len) == 0 )
    {
        int SQLparameterCount = 1;
        
        std::string stmt = "SELECT * FROM fn_csm_node_delete($1::text[])";

        // Build the parameterized list.
		csm::db::DBReqContent *dbReq = new csm::db::DBReqContent(stmt, SQLparameterCount); 
		dbReq->AddTextArrayParam(input->node_names, input->node_names_count);

        *dbPayload = dbReq;
        csm_free_struct_ptr(API_PARAMETER_INPUT_TYPE, input);

        LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";
        
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the query, struct could not be deserializ    ed");
        return false;
    }
    
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMINodeDelete::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **stringBuffer, 
		uint32_t &bufferLength,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";
	
	bool success = true;

    *stringBuffer = NULL;
    bufferLength = 0;
	/*Helper Variables*/
    uint32_t numResults = tuples.size();
	
	API_PARAMETER_OUTPUT_TYPE* output = NULL;

    if(numResults == 1 )
    {
		char* pEnd;
		uint32_t actualRecords = strtol(tuples[0]->data[0], &pEnd, 10);
		if(pEnd == tuples[0]->data[0]){ output->failure_count = -1;}
		
		if(actualRecords == 0){
			//API work 100%
			
			csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
			/* Say how many results there are. */
			output->failure_count = 0;
			
		}else if(actualRecords > 0){
			//failure feedback records found
			csm_init_struct_ptr(API_PARAMETER_OUTPUT_TYPE, output);
			csm_prep_csv_to_struct();
			/* Say how many results there are. */
			output->failure_count = atoi(tuples[0]->data[0]);
			//copy the nodes that didnt delete
			output->failure_node_names = (char**)calloc(output->failure_count, sizeof(char*));
			csm_parse_csv_to_struct( tuples[0]->data[1], output->failure_node_names, actualRecords, CSM_NO_MEMBER, strdup("N/A"), strdup);
			
			//set return codes
			// success = false;
			// ctx->SetErrorCode( CSMERR_DEL_MISMATCH );
			// ctx->SetErrorMessage( "DATABASE FUNCTION numRecord != 1" );
			
		}else{
			//default
			LOG( csmapi, error  ) << STATE_NAME ":CreateByteArray: DATABASE ERROR.";
			
			success = false;
			ctx->SetErrorCode( CSMERR_DB_ERROR );
		}
    }else{
		//bad stuff
		LOG( csmapi, error  ) << STATE_NAME ":CreateByteArray: DATABASE FUNCTION ERROR.";
		LOG( csmapi, error  ) << STATE_NAME ":CreateByteArray: DATABASE FUNCTION numRecord != 1.";
        
        ctx->SetErrorCode( CSMERR_DB_ERROR );
        ctx->SetErrorMessage( "DATABASE FUNCTION numRecord != 1" );
        success = false;
	}

    csm_serialize_struct( API_PARAMETER_OUTPUT_TYPE, output, stringBuffer, &bufferLength );
    csm_free_struct_ptr( API_PARAMETER_OUTPUT_TYPE, output );

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return success;
}

