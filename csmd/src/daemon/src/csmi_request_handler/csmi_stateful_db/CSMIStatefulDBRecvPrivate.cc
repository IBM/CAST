/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db/CSMIStatefulDBRecvPrivate.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "CSMIStatefulDBRecvPrivate.h"

bool StatefulDBRecvPrivate::HandleDBResp(
        const std::vector<csm::db::DBTuple *>& tuples,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << "StatefulDBRecvPrivate::HandleDBResp: Enter";

    csm::db::DBReqContent *dbPayload = nullptr;
    csm::network::MessageAndAddress msg = 
        dynamic_cast<const csm::daemon::NetworkEvent *>( ctx->GetReqEvent() )->GetContent();
    
    bool hasPrivateAccess = _Handler->CompareDataForPrivateCheck(tuples, msg._Msg, ctx);

	if ( !hasPrivateAccess ) 
    {
        LOG( csmapi, trace ) << "StatefulDBRecvPrivate::HandleDBResp: ACCESS DENIED";
        LOG( csmapi, trace ) << "StatefulDBRecvPrivate::HandleDBResp: Exit";

        if ( ctx->GetErrorCode() == CSMI_SUCCESS )
        {
            ctx->SetErrorCode( CSMERR_PERM );
            ctx->AppendErrorMessage( "Permission Denied" );
        }
		return false;
	}
    

    bool success = csm::daemon::HandlerOptions::HasValidDBConn();
    if ( success )
    {
        success = _Handler->CreatePayload(
            msg._Msg.GetData(),
            msg._Msg.GetDataLen(),
            &dbPayload, ctx ); 

        if ( success )
            success = this->PushDBReq( *dbPayload, ctx, postEventList );
    }
    else
    {
        ctx->SetErrorCode(CSMERR_DAEMON_DB_ERR);
        ctx->SetErrorMessage("CSM ERROR - Database Error in Local Daemon");
        success = false;
    }

    delete dbPayload;

    LOG( csmapi, trace ) << "StatefulDBRecvPrivate::HandleDBResp: Exit";

    return success;
}
