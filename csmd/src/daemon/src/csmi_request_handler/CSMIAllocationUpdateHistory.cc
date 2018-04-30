/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationUpdateHistory.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: John Dunham
* Email: jdunham@us.ibm.com
*/

#include "CSMIAllocationUpdateHistory.h"

//Used for debug prints
#define STATE_NAME "CSMIAllcoationUpdateHistory:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_update_history_input_t
#define RECORD_STRUCT 
#define OUTPUT_STRUCT 

bool CSMIAllocationUpdateHistory::CreatePayload(
        const std::string& stringBuffer,
        const uint32_t bufferLength,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	/*Unpack the buffer.*/
	INPUT_STRUCT* input = NULL;

	/* Error in case something went wrong with the unpack*/
	if ( csm_deserialize_struct(INPUT_STRUCT, &input, stringBuffer.c_str(), bufferLength) == 0 )
    {
        // Initialize to 1 since allocation_id is static.
        int paramCount = 0;

        std::string stmt = "UPDATE csm_allocation_history "
            "SET ";

        add_param_sql( stmt, input->user_id >= 0      , ++paramCount, "user_id=$",     "::integer,")
        add_param_sql( stmt, input->user_group_id >= 0, ++paramCount, "user_group_id=$", "::integer,")
        add_param_sql( stmt, input->user_name[0]      , ++paramCount, "user_name=$",   "::text,")
        add_param_sql( stmt, input->account[0]        , ++paramCount, "account=$",     "::text,")
        add_param_sql( stmt, input->comment[0]        , ++paramCount, "comment=$",     "::text,")
        add_param_sql( stmt, input->job_name[0]       , ++paramCount, "job_name=$",    "::text,")
        add_param_sql( stmt, input->reservation[0]    , ++paramCount, "reservation=$", "::text,")

        // EARLY RETURN
		if ( paramCount > 0 )
        {
            // Replace the last comma.
            stmt[stmt.length() - 1] = ' ';
		}
        else
        {
            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: No values to update supplied";
            ctx->SetErrorCode(CSMERR_MISSING_PARAM);
            ctx->SetErrorMessage("Unable to build update query, no values to update supplied");
            csm_free_struct_ptr(INPUT_STRUCT, input);

            LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Exit";
            return false;
        }

        add_param_sql_fixed( stmt, ++paramCount, "WHERE allocation_id=$", "::bigint ");
        stmt.append("RETURNING allocation_id"); // Return the allocation id to verify the update happened.

        //==========================================================================
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );

        if(input->user_id >= 0)       dbReq->AddNumericParam<int32_t>(input->user_id);
        if(input->user_group_id >= 0) dbReq->AddNumericParam<int32_t>(input->user_group_id);
        if(input->user_name[0])      dbReq->AddTextParam(input->user_name);
        if(input->account[0])        dbReq->AddTextParam(input->account);
        if(input->comment[0])        dbReq->AddTextParam(input->comment);
        if(input->job_name[0])       dbReq->AddTextParam(input->job_name);
        if(input->reservation[0])    dbReq->AddTextParam(input->reservation);

        dbReq->AddNumericParam<int64_t>(input->allocation_id);

        *dbPayload = dbReq;
        csm_free_struct_ptr(INPUT_STRUCT, input);
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

bool CSMIAllocationUpdateHistory::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    // This should do nothing.
    bool success = true;
    *buf = NULL;
    bufLen = 0;
	
    // If the update had no responses return an error.
    if ( tuples.size() == 0 )
    {
        success = false;
        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->SetErrorMessage("Allocation was not present in the csm_allocation_history table.");
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return success;
}

