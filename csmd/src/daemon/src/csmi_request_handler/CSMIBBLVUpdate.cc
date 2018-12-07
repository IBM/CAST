/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBLVUpdate.cc

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

/* CSM Includes*/
/* Header for this file. */
#include "CSMIBBLVUpdate.h"

#define STATE_NAME "CSMIBBLVUpdate:"

#define INPUT_STRUCT csm_bb_lv_update_input_t
#define OUTPUT_STRUCT csm_BbLvUpdate_output_t

bool CSMIBBLVUpdate::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT *input = nullptr;
    
    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt= "UPDATE csm_lv SET "
                "state = $1::text, current_size = $2::bigint, updated_time = 'now' "
            "WHERE "
                "logical_volume_name = $3::text AND "
                "allocation_id =  $4::bigint AND "
				"node_name = $5::text";

        const int paramCount = 5;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount ); 

        dbReq->AddCharacterParam(input->state);
        dbReq->AddNumericParam<int64_t>(input->current_size);
        dbReq->AddTextParam(input->logical_volume_name);
        dbReq->AddNumericParam<int64_t>(input->allocation_id);
		dbReq->AddTextParam(input->node_name);
        
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

bool CSMIBBLVUpdate::CreateByteArray(
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
