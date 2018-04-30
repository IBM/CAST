/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIDiagResultCreate.cc

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

#include "CSMIDiagResultCreate.h"

#define STATE_NAME "CSMIDiagResultCreate:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT  csm_diag_result_create_input_t
#define OUTPUT_STRUCT  

bool CSMIDiagResultCreate::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt = "INSERT INTO csm_diag_result ( run_id, test_name, node_name, serial_number, "
            "begin_time, end_time, status, log_file ) VALUES ( $1::bigint, $2::text, $3::text, "
            "$4::text, $5::timestamp, 'now', $6::char(16), $7::text )";

        const int paramCount = 7;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );

        dbReq->AddNumericParam<int64_t>( input->run_id );
        dbReq->AddTextParam( input->test_name );
        dbReq->AddTextParam( input->node_name );
        dbReq->AddTextParam( input->serial_number );
        dbReq->AddTextParam( input->begin_time );
        dbReq->AddTextParam( input->status, 16 );  // FIXME MAGIC NUMBER 
        dbReq->AddTextParam( input->log_file );

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

bool CSMIDiagResultCreate::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf   = NULL;
    bufLen = 0;

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}
