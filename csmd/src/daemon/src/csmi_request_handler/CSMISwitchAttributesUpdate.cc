/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMISwitchAttributesUpdate.cc

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
#include "CSMISwitchAttributesUpdate.h"

#define STATE_NAME "CSMISwitchAttributesUpdate:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_switch_attributes_update_input_t
#define OUTPUT_STRUCT  csm_switch_attributes_update_output_t

bool CSMISwitchAttributesUpdate::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

	INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        int paramCount = 0;

        std::string stmt = "WITH updated AS ( UPDATE csm_switch SET ";

		if(input->comment[0]){ add_param_sql( stmt, input->comment, ++paramCount, "comment=$", "::text,") }
		if(input->physical_frame_location[0]){ add_param_sql( stmt, input->physical_frame_location, ++paramCount, "physical_frame_location=$", "::text,") }
		if(input->physical_u_location[0]){ add_param_sql( stmt, input->physical_u_location, ++paramCount, "physical_u_location=$", "::text,") }
        
        // Verify the payload.
        if ( paramCount >  0 )
        {
            // Remove the last comma.
            stmt.back()= ' ';
        }
        else
        {
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: No values supplied";
            ctx->SetErrorCode(CSMERR_MISSING_PARAM);
            ctx->SetErrorMessage("Unable to build update query, no values supplied");
            csm_free_struct_ptr(INPUT_STRUCT, input);

            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
            return false;
        }

        // Build the array parameter.
        std::string array_param = "$";
        array_param.append(std::to_string(++paramCount)).append("::text[]");
        stmt.append("WHERE switch_name=ANY(").append(array_param).append(") RETURNING switch_name )"
            "SELECT name FROM unnest (").append(array_param).append( ") as input(name) "
            "LEFT JOIN updated ON (updated.switch_name=input.name) "
            "WHERE updated.switch_name IS NULL");

        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
		if ( input->comment[0] ) dbReq->AddTextParam(input->comment);
		if ( input->physical_frame_location[0] ) dbReq->AddTextParam(input->physical_frame_location);
		if ( input->physical_u_location[0] ) dbReq->AddTextParam(input->physical_u_location);
        dbReq->AddTextArrayParam( input->switch_names , input->switch_names_count );
        
        *dbPayload = dbReq;

        csm_free_struct_ptr(INPUT_STRUCT, input);

        LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
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

bool CSMISwitchAttributesUpdate::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf = nullptr;
    bufLen = 0;

    uint32_t numRecords = tuples.size();

    OUTPUT_STRUCT* output = nullptr;
    csm_init_struct_ptr( OUTPUT_STRUCT, output );

    output->failure_count  = numRecords;
    if ( numRecords > 0 )
    {
        output->failure_switches = (char**)malloc( numRecords * sizeof(char*) );
        
        for ( uint32_t i = 0; i < numRecords; ++i )
        {
            output->failure_switches[i] = strdup(tuples[i]->data[0]);
        }
    }

    csm_serialize_struct(OUTPUT_STRUCT, output, buf, &bufLen);
    csm_free_struct_ptr(OUTPUT_STRUCT, output);

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

