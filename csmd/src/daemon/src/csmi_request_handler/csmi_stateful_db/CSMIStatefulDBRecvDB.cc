/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db/CSMIStatefulDBRecvDB.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "CSMIStatefulDBRecvDB.h"

bool StatefulDBRecv::HandleDBResp(
        const std::vector<csm::db::DBTuple *>& tuples,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG( csmapi, trace ) << "StatefulDBRecv::HandleDBResp: Enter";

    char* buffer = nullptr;
    uint32_t bufferLength = 0;

    bool success = _Handler->CreateByteArray(tuples, &buffer, bufferLength, ctx);
    
    if ( success )
    {
        this->PushReply( buffer, bufferLength, ctx, postEventList, false);
    }
    else if ( ctx->GetErrorCode() == CSMI_SUCCESS )
    {
        ctx->SetErrorCode(CSMERR_MSG_PACK_ERROR); 
        ctx->SetErrorMessage(
            "CSM ERROR - Could not create responding byte array (Default error)");
    }

    if ( buffer )
    {
        free(buffer);
    }
    
    LOG( csmapi, trace ) << "StatefulDBRecv::HandleDBResp: Exit";
    return success;
}
