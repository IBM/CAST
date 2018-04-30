/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db/CSMIStatefulDBRecvSend.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef CSMIStatefulDBRecvSend_h
#define CSMIStatefulDBRecvSend_h

#include "../csmi_stateful_db.h"
#include "../csmi_db_resp_state.h"

typedef bool( *PayloadFunct )(const std::vector<csm::db::DBTuple *>&,
    csm::db::DBReqContent **,
    csm::daemon::EventContextHandlerState_sptr);

#define STATE_NAME "StatefulDBRecvSend"

/** @brief 
 *
 * @param state_name   
 * @param target_state 
 * @param fail_state   
 */

template<PayloadFunct PF>
class  StatefulDBRecvSend : public DBRespState
{
public:
    StatefulDBRecvSend(
            uint64_t success,
            uint64_t failure,
            uint64_t final_state,
            uint64_t timeout = csm::daemon::helper::BAD_STATE,
            uint64_t timeout_length = csm::daemon::helper::BAD_TIMEOUT_LEN,
            uint64_t alternate = csm::daemon::helper::BAD_STATE,
            const std::string & state_name = STATE_NAME):
        DBRespState(success, failure, final_state, timeout, 
            timeout_length, alternate, state_name)
        { }

protected:
    virtual bool HandleDBResp(
        const std::vector<csm::db::DBTuple *>& tuples,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) final
    {
        LOG( csmapi, trace ) << "StatefulDBRecvSend::HandleDBResp: Enter";

        csm::db::DBReqContent *dbPayload = nullptr;
        
        bool success = csm::daemon::HandlerOptions::HasValidDBConn();
        if ( success )
        {
            success = PF( tuples, &dbPayload, ctx ); 

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

        LOG( csmapi, trace ) << "StatefulDBRecvSend::HandleDBResp: Exit";

        return success;
    }


    /** 
     * @brief See @ref CSMIHandlerState::DefaultHandleError for documentation. 
     * 
     * If @p ctx has @ref CSMI_NO_RESULTS as an error code, an empty reply will be returned.
     */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false) final
    { 
        // If the error code is not null results, run the default handler.
        // Else push an empty reply back (No Results is not an error.
        if ( ctx->GetErrorCode() != CSMI_NO_RESULTS )
        {
            CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, byAggregator );
        }
        else
        {
            // Set to the final state and push the empty reply.
            ctx->SetAuxiliaryId( GetFinalState() );
            this->PushReply( nullptr, 0, ctx, postEventList, byAggregator);
        }
    } 

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};
#undef STATE_NAME

#endif
