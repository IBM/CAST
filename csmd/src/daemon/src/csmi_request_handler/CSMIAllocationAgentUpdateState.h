/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationAgentUpdateState.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef CSMIALLOCATION_AGENT_UPDATE_STATE_H
#define CSMIALLOCATION_AGENT_UPDATE_STATE_H

#include "csmi_network_message_state.h"
#include "csmi/include/csm_api.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include <mutex>       ///< File Output guard

/** @brief  
 * @todo document me.
 * @param state_name   
 * @param target_state 
 * @param fail_state   
 */
class AllocationAgentUpdateState: public NetworkMessageState
{
    using NetworkMessageState::NetworkMessageState;
private:
    std::mutex _ActiveListMutex; ///< Mutex for locking writes to the activelist.

protected:

    /** @brief Sets up the node, or rolls back an initialization.
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
        csm::daemon::EventContextHandlerState_sptr ctx ) final;
    
    /**
     * @brief Initializes the Allocation on the compute node, this is used when transitioning allocations to the ready state.
     *
     * @param[in,out] payload A multicast payload, contains the configuration settings and acts as a container for the node metrics.
     * @param[in,out] postEventList The list of events to push new events to.
     * @param[in,out] ctx  The context of the handler, tracks errors.
     * 
     * @return True if the node was initialized
     * @return False if the node was unable to be initialized.
     */
    bool InitNode( 
        csmi_allocation_mcast_payload_request_t *payload,
        csmi_allocation_mcast_payload_response_t *respPayload,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx );

    /**
     * @brief Deallocates the compute node for the allocation, this is used when transitioning allocations to the staging out state.
     *
     * @param[in,out] payload A multicast payload, contains the configuration settings and acts as a container for the node metrics.
     * @param[in,out] postEventList The list of events to push new events to.
     * @param[in,out] ctx  The context of the handler, tracks errors.
     *
     * @return True if the node was deallocated.
     * @return False if the node was unable to be deallocated.
     */
    bool RevertNode( 
        csmi_allocation_mcast_payload_request_t *payload,
        csmi_allocation_mcast_payload_response_t *respPayload,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx );

    /**
     * @brief Performs the data aggregation step of the allocation, ignores failures (Beta 2).
     *
     * @param[in] payload The payload object that will be sent back to the back end.
     *                      This object will have several metric variables.
     */
    void DataAggregators(csmi_allocation_mcast_payload_response_t *payload);

    /**
     * @brief Performs the user defined data aggregation step of the allocation, ignores failures (Beta 2).
     *
     * @param[in] payload The payload object that will be sent back to the back end.
     *                      This object will have several metric variables.
     */
    void AfterPrologDataAggregators(csmi_allocation_mcast_payload_response_t *payload);

    /** @brief See @ref CSMIHandlerState::DefaultHandleError for documentation. 
     * Prep ends the hostname to any error message received.
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
    
    /** @brief Register the allocation with the local compute node.
     *
     * Currently writes to the `/etc/pam.d/csm/activelist` file with the following format:
     * ```
     * [username];[allocation_id]
     * ```
     *
     * @param [in] allocationId The allocation id to be registered.
     * @param [in] username The user associated with the allocation.
     *
     * @return 0; The Registration was a success.
     * @return >0; The file write failed.
     */
    int RegisterAllocation( int64_t allocationId, const char* username );
    
    /** @brief Removes the allocation from the local compute node.
     *
     * @param[in] allocationId The allocation to remove from the node.
     *
     * @return 0; The allocation was successfully removed.
     * @return >0; The allocation couldn't be removed.
     */
    int RemoveAllocation( int64_t allocationId );
};
#endif
