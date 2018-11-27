/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_forward/CSMIStatefulForwardRequest.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_STATEFUL_FORWARD_REQUEST_H__
#define __CSMI_STATEFUL_FORWARD_REQUEST_H__

#include "../csmi_network_message_state.h"

#define STATE_NAME "StatefulForwardRequest"

/** @brief A state for forwarding network requests.
 */
class StatefulForwardRequest: public NetworkMessageState
{
public:
    StatefulForwardRequest(
            uint64_t success,
            uint64_t failure,
            uint64_t final_state,
            uint64_t timeout = csm::daemon::helper::BAD_STATE,
            uint64_t timeout_length = csm::daemon::helper::BAD_TIMEOUT_LEN,
            uint64_t alternate = csm::daemon::helper::BAD_STATE):
        NetworkMessageState(success, failure, final_state, timeout, 
            timeout_length, alternate, STATE_NAME) {  }
    
protected:
    virtual bool HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;
    

    /** @brief See @ref CSMIHandlerState::DefaultHandleError for documentation. */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false) final
    {
        CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, byAggregator );
    }
    
    /** @brief Unused, returns nullptr. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { ; }
};
#undef STATE_NAME

#endif

