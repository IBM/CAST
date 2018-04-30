/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepCGROUPDelete.h
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
    
#ifndef  __CSMI_ALLOCATION_STEP_CGROUP_DELETE_H__
#define  __CSMI_ALLOCATION_STEP_CGROUP_DELETE_H__

#include "csmi_stateful.h"
#include "csmi/include/csm_api.h"
#include "csmi_network_message_state.h"

/**
 * @brief An enumeration for tracking the Step Delete States.
 */
enum CSMIAllocationStepDeleteStates
{
    AllocStepCGDeleteInit = 0, ///< The initial state, this will delete a cgroup.
    AllocStepCGDeleteFinal     ///< The final state, end of execution for a context.
};

/** @brief Kills any processes associated with the supplied Allocation_Step id, then removes the state.
 *
 * @param state_name   AllocStepCGDeleteInit
 * @param target_state AllocStepCGDeleteFinal
 * @param fail_state   AllocStepCGDeleteFinal
 */
class AllocStepCGDeleteInitState : public NetworkMessageState
{
    using NetworkMessageState::NetworkMessageState;
protected:
    virtual bool HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

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

/**
 * @brief A handler that deletes the cgroup associated with the supplied Allocation_Step id.
 *
 * This class should only be called from the compute node.
 */
class CSMIAllocationStepCGROUPDelete : public CSMIStateful
{
public:
    CSMIAllocationStepCGROUPDelete( csm::daemon::HandlerOptions& options ) :
        CSMIStateful( CSM_CMD_allocation_step_cgroup_delete , options )
    {
        // Set the start state for the machine.
        SetInitialState( AllocStepCGDeleteInit );

        // Resize the state vector then populate it.
        ResizeStates( AllocStepCGDeleteFinal );
        SetState( AllocStepCGDeleteInit,      ///< State ID
            new AllocStepCGDeleteInitState(
                AllocStepCGDeleteFinal,       ///< Success State
                AllocStepCGDeleteFinal,       ///< Failure State
                AllocStepCGDeleteFinal ));    ///< Final State
    }

};

#endif 
