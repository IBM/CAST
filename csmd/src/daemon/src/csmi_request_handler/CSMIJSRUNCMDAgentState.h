/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIJSRUNCMDAgentState.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef CSMI_JSRUN_CMD_AGENT_STATE_H
#define CSMI_JSRUN_CMD_AGENT_STATE_H

#include "csmi_network_message_state.h"
#include "csmi/include/csmi_type_bb_funct.h"

/** @brief  
 * @param state_name   
 * @param target_state 
 * @param fail_state   
 */
class JSRUNCMDAgentState: public NetworkMessageState
{
    using NetworkMessageState::NetworkMessageState;
protected:

    /** @brief Executes the Burst Buffer command and collects the error code and output.
     *
     * @param[in]     content The message containing the payload.
     * @param[in,out] postEventList The list of events to push new events to.
     * @param[in,out] ctx  The context of the handler, tracks errors.
     * 
     * @return The success of the handler.
     */
    virtual bool HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final;
    
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false) final;

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};
#endif
