/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBLVCreate.cc

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
#include "CSMIBBLVCreate.h"

#define STATE_NAME "CSMIBBLVCreate:"
// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_bb_lv_create_input_t
#define OUTPUT_STRUCT 

bool CSMIBBLVCreate::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT *input = nullptr;
    
    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt= "SELECT fn_csm_lv_upsert ("
            "$1::text, $2::text, $3::bigint, "    // logical_volume_name, node_name, allocation_id
            "$4::text, $5::char(1), $6::bigint, " // vg_name, state, current_size
            "$7::bigint, 'now', 'now', "          // max_size, begin_time, updated_time
            "$8::text, $9::text ); ";             // file_system_mount, file_system_type

        // =======================================================================
        const int paramCount = 9;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount ); 

        dbReq->AddTextParam(input->logical_volume_name);
        dbReq->AddTextParam(input->node_name); 
        dbReq->AddNumericParam<int64_t>(input->allocation_id);

        dbReq->AddTextParam(input->vg_name);
        dbReq->AddCharacterParam(input->state);
        dbReq->AddNumericParam<int64_t>(input->current_size);
		dbReq->AddNumericParam<int64_t>(input->current_size); // current size, which at time of create is max
        
        dbReq->AddTextParam(input->file_system_mount);
        dbReq->AddTextParam(input->file_system_type);
        
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

bool CSMIBBLVCreate::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf   = NULL;
    bufLen = 0;

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}


