/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIDiagRunBegin.cc

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

#include "CSMIDiagRunBegin.h"

#define STATE_NAME "CSMIDiagRunBegin:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT  csm_diag_run_begin_input_t
#define OUTPUT_STRUCT

bool CSMIDiagRunBegin::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt = "INSERT INTO csm_diag_run "
            "( run_id, allocation_id, begin_time, status, inserted_ras, log_dir, cmd_line ) "
        "VALUES "
            "( $1::bigint, $2::bigint, 'now', 'RUNNING', '0', $3::text, $4::text )";

        const int paramCount = 4;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( input->run_id );
        dbReq->AddNumericParam<int64_t>( input->allocation_id );
        dbReq->AddTextParam( input->log_dir );
        dbReq->AddTextParam( input->cmd_line );

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

bool CSMIDiagRunBegin::CreateByteArray(
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
