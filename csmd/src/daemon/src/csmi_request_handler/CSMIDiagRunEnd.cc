/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIDiagRunEnd.cc

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

#include "CSMIDiagRunEnd.h"

#define STATE_NAME "CSMIDiagRunEnd:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_diag_run_end_input_t
#define OUTPUT_STRUCT
     
bool CSMIDiagRunEnd::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT* input = nullptr;

    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt = "SELECT fn_csm_diag_run_history_dump( "
            "$1::bigint, 'now', $2::text, $3::boolean );";

        const int paramCount = 3;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );

        dbReq->AddNumericParam<int64_t>( input->run_id );
        dbReq->AddTextParam( input->status, 16 );  // FIXME MAGIC NUMBER 
        dbReq->AddCharacterParam(input->inserted_ras == 't' || 
            input->inserted_ras == 'T' || 
            input->inserted_ras == '1'); 

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

bool CSMIDiagRunEnd::CreateByteArray(
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
