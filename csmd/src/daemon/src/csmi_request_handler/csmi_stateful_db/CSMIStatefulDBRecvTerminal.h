/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db/CSMIStatefulDBRecvTerminal.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef CSMIStatefulDBRecvTerminal_h
#define CSMIStatefulDBRecvTerminal_h

#include "../csmi_stateful_db.h"
#include "../csmi_db_resp_state.h"

typedef bool( *TerminalFunct )(const std::vector<csm::db::DBTuple *>&,
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr);

#define STATE_NAME "StatefulDBRecvTerminal"

/** @brief A state for processing a terminal state, this state precedes the final state.
 *
 */

template<TerminalFunct TF>
class  StatefulDBRecvTerminal : public DBRespState
{
public:
    StatefulDBRecvTerminal(
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
        LOG( csmapi, trace ) << "StatefulDBRecvTerminal::HandleDBResp: Enter";
        
        char *buffer = nullptr;
        uint32_t bufferLength = 0;

        bool success = TF( tuples, &buffer, bufferLength, ctx ); 

        if( success )
        {
            this->PushReply( buffer, bufferLength, ctx, postEventList, false);
        }

        if(buffer) free(buffer);

        LOG( csmapi, trace ) << "StatefulDBRecvTerminal::HandleDBResp: Exit";

        return success;
    }


    /** @brief See @ref CSMIHandlerState::DefaultHandleError for documentation. */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false) final
    { 
        CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, byAggregator );
    } 

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};
#undef STATE_NAME

#endif
