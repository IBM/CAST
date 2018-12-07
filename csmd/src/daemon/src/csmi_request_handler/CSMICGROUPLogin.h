/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMICGROUPLogin.h
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
    
#ifndef  __CSMI_CGROUP_LOGIN_H__
#define  __CSMI_CGROUP_LOGIN_H__

#include "csmi_stateful.h"
#include "csmi/include/csm_api.h"
#include "csmi_network_message_state.h"

/**
 * An enumeration tracking the login status.
 */
enum CSMICGLogin
{
    CGLoginInit = 0, ///< The initial state, moves the user. 
    CGLoginFinal     ///< The final state, clears the context.
};

class CGLoginInitState : public NetworkMessageState
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
 * A handler for placing a user into a cgroup on login.
 */
class CSMICGROUPLogin : public CSMIStateful
{
public:
    CSMICGROUPLogin( csm::daemon::HandlerOptions& options ) :
        CSMIStateful( CSM_CMD_cgroup_login , options )
    {
        // Set the start state for the machine.
        SetInitialState( CGLoginInit );

        // Resize the state vector then populate it.
        ResizeStates( CGLoginFinal );
        SetState( CGLoginInit,      ///< State ID
            new CGLoginInitState(
                CGLoginFinal,       ///< Success State
                CGLoginFinal,       ///< Failure State
                CGLoginFinal ));    ///< Final State
    }

};


#endif 
