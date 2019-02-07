/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBLVDelete.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

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
#include "CSMIBBLVDelete.h"

#define STATE_NAME "CSMIBBLVDelete:"
// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_bb_lv_delete_input_t
#define OUTPUT_STRUCT csm_BbLvDelete_output_t

bool CSMIBBLVDelete::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT *input = nullptr;
    
    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0)
    {
        //Number of parameters from input. Let's database know how many parameters to expect. 
        int paramCount = 5;

        std::string stmt= "SELECT fn_csm_lv_history_dump ( "
            "$1::text, "   // logical_volume_name
			"$2::text, "   // node_name
			"$3::bigint, " // allocation_id
			"'now', 'now',"      // updated_time, end_time
            "$4::bigint, $5::bigint, "; // num_bytes_read,num_bytes_written

        // num_reads
        if(input->num_reads < 0)
        {
            stmt.append("NULL, ");
        }else{
            paramCount++;
            stmt.append("$").append(std::to_string(paramCount)).append("::bigint, ");
        }

        // num_writes
        if(input->num_writes < 0)
        {
            stmt.append("NULL) ");
        }else{
            paramCount++;
            stmt.append("$").append(std::to_string(paramCount)).append("::bigint) ");
        }

        

        // =======================================================================
        
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount ); 
        dbReq->AddTextParam(input->logical_volume_name);
		dbReq->AddTextParam(input->node_name);
        dbReq->AddNumericParam<int64_t>(input->allocation_id);
		dbReq->AddNumericParam<int64_t>(input->num_bytes_read);
		dbReq->AddNumericParam<int64_t>(input->num_bytes_written);
        if(input->num_reads >= 0) { dbReq->AddNumericParam<int64_t>(input->num_reads); }
        if(input->num_writes >= 0) { dbReq->AddNumericParam<int64_t>(input->num_writes); }
        
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

bool CSMIBBLVDelete::CreateByteArray(
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

