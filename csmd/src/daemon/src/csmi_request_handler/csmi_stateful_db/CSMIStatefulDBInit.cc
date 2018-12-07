/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db/CSMIStatefulDBInit.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIStatefulDBInit.h"

bool StatefulDBInit::HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << "StatefulDBInit::HandleNetworkMessage: Enter";

    bool success = csm::daemon::HandlerOptions::HasValidDBConn();
    csm::db::DBReqContent *dbPayload = nullptr;

    if ( success )
    {
        LOG( csmapi, debug ) << "StatefulDBInit::HandleNetworkMessage: DB Connection Found";
        LOG(csmapi, debug ) << "Requires User Compare " << content._Msg.PrivateRequiresUserCompare();

        // Set the negation because that mean the user has unfettered access.
        ctx->SetHasPrivateAccess(!content._Msg.PrivateRequiresUserCompare());
        if ( content._Msg.PrivateRequiresUserCompare() )
        {
            success = _Handler->RetrieveDataForPrivateCheck(
                content._Msg.GetData(),
                content._Msg.GetDataLen(),
                &dbPayload, ctx );

            if (success)
            {
                success = this->PushDBReq( *dbPayload, ctx, postEventList );
                this->SetAsAlternate(ctx);
            }
        }
        else
        {
            LOG( csmapi, debug ) << "StatefulDBInit::HandleNetworkMessage: Creating Payload";
            success = _Handler->CreatePayload(
                content._Msg.GetData(),
                content._Msg.GetDataLen(),
                &dbPayload, ctx ); 
            LOG( csmapi, debug ) << "StatefulDBInit::HandleNetworkMessage: Payload Created";

            if (success)
                success = this->PushDBReq( *dbPayload, ctx, postEventList );
        }
    }
    else
    {
        ctx->SetErrorCode(CSMERR_DAEMON_DB_ERR);
        ctx->SetErrorMessage("CSM ERROR - Database Error in Local Daemon");
    }

    delete dbPayload;

    LOG( csmapi, trace ) << "StatefulDBInit::HandleNetworkMessage: Exit";
    return success;
}

