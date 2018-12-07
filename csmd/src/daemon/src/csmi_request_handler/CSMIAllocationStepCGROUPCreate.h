/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepCGROUPCreate.h
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
    
#ifndef  __CSMI_ALLOCATION_STEP_CGROUP_CREATE_H__
#define  __CSMI_ALLOCATION_STEP_CGROUP_CREATE_H__

#include "csmi_stateful.h"
#include "csmi/include/csm_api.h"
#include "csmi_network_message_state.h"

/**
 * An enumeration tracking the step create states.
 */
enum CSMIAllocationStepCreateStates
{
    AllocStepCGCreateInit = 0, ///< The initial state, this will create a cgroup.
    AllocStepCGCreateFinal     ///< The final state, end of execution for a context.
};

class AllocStepCGCreateInitState : public NetworkMessageState
{
    using NetworkMessageState::NetworkMessageState;
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

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};

/**
 * A handler for creating an allocation step cgroup. 
 */
class CSMIAllocationStepCGROUPCreate : public CSMIStateful
{
public:
    CSMIAllocationStepCGROUPCreate( csm::daemon::HandlerOptions& options ) :
        CSMIStateful( CSM_CMD_allocation_step_cgroup_create , options )
    {
        // Set the start state for the machine.
        SetInitialState( AllocStepCGCreateInit );

        // Resize the state vector then populate it.
        ResizeStates( AllocStepCGCreateFinal );
        SetState( AllocStepCGCreateInit,      ///< State ID
            new AllocStepCGCreateInitState(
                AllocStepCGCreateFinal,       ///< Success State
                AllocStepCGCreateFinal,       ///< Failure State
                AllocStepCGCreateFinal ));    ///< Final State
    }

};


#endif 
