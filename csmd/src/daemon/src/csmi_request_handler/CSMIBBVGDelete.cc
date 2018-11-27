/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIBBVGDelete.cc

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
#include "CSMIBBVGDelete.h"

#define STATE_NAME "CSMIBBVGDelete:"

#define INPUT_STRUCT csm_bb_vg_delete_input_t
#define OUTPUT_STRUCT csm_bb_vg_delete_output_t

bool CSMIBBVGDelete::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    INPUT_STRUCT *input = nullptr;
    
    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt = 
			"SELECT fn_csm_vg_delete("
				"$1::text, "
				"$2::text)";

        const int paramCount = 2;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount ); 
        dbReq->AddTextParam( input->node_name );
		dbReq->AddTextParam( input->vg_name);
        
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

bool CSMIBBVGDelete::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf   = NULL;
    bufLen = 0;
	
    uint32_t numRecords = tuples.size();

    OUTPUT_STRUCT *output = nullptr;
    csm_init_struct_ptr(OUTPUT_STRUCT, output);
	
	if(numRecords == 1)
	{
		/* VG Delete always returns one record */
		/* field [0] = (STRING) a csv list of vg_names failed to delete */
		/* (not yet implemented) field [1] = (STRING) a csv list of vg_ssd relations failed to delete */
		
		if(tuples[0]->data[0] == NULL || tuples[0]->data[0][0] == '\0')
		{
			LOG( csmapi, debug ) << STATE_NAME ":CreateByteArray: list of vg names not deleted aka tuples[0]->data[0][0]: " << tuples[0]->data[0];
			
			/* VG delete has deleted all supplied vg_names */
			output->failure_count  = 0;
			output->failure_vg_names = NULL;
		}else{
			/* VG delete failed to delete all supplied vg_names */
			/* field [0] = (STRING) is a csv list of vg_names failed to delete */
			/* need to copy this info into "output->failure_vg_names" */
			
			LOG( csmapi, debug ) << STATE_NAME ":CreateByteArray: list of vg names not deleted aka tuples[0]->data[0]: " << tuples[0]->data[0];
			
			// No longer support multiple vg delete
			output->failure_count  = 0;
			output->failure_vg_names = NULL;
		}
		
	}else{
		/* Something went wrong */
		
		/*TODO: better error handle */
		output->failure_count  = 0;
		output->failure_vg_names = NULL;
	}
    
    csm_serialize_struct( OUTPUT_STRUCT, output, buf, &bufLen);
    csm_free_struct_ptr( OUTPUT_STRUCT, output );

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";

    return true;
}

