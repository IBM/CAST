/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIStepUpdateAgent.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef _CSMI_STEP_UPDATE_AGENT_H_
#define _CSMI_STEP_UPDATE_AGENT_H_

#include "csmi_network_message_state.h"
#include "csmi/include/csm_api.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"

/** @brief  
 */
class StepAgentUpdateState : public NetworkMessageState
{
    using NetworkMessageState::NetworkMessageState;

protected:
    /** @brief Sets up the node, or rolls back an initialization.
     *
     * @param[in]     content The mesaage containing the payload.
     * @param[in,out] postEventList The list of events to push new events to.
     * @param[in,out] ctx  The context of the handler, tracks errors.
     * 
     * @return The success of the handler.
     */
    virtual bool HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;
    
    /**
     * @brief Runs the prolog for the step.
     *
     * @param[in,out] payload A multicast payload, contains the configuration settings.
     * @param[in,out] postEventList The list of events to push new events to.
     * @param[in,out] ctx  The context of the handler, tracks errors.
     * 
     * @return True if the node was initialized
     * @return False if the node was unable to be initialized.
     */
    bool StepBegin(
        csmi_allocation_step_mcast_payload_t *step,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx );

    /**
     * @brief Runs the epilog for the step.
     *
     * @param[in,out] payload A multicast payload, contains the configuration settings.
     * @param[in,out] postEventList The list of events to push new events to.
     * @param[in,out] ctx  The context of the handler, tracks errors.
     *
     * @return True if the node was deallocated.
     * @return False if the node was unable to be deallocated.
     */
    bool StepEnd( 
        csmi_allocation_step_mcast_payload_t *step,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx );

    /** @brief See @ref CSMIHandlerState::DefaultHandleError for documentation. 
     * Prepends the hostname to any error message received.
     */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false) final;

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};
#endif
