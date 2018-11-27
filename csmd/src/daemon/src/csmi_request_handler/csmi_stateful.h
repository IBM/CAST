/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMIStateful_H__
#define __CSMIStateful_H__

#include "csmi_base.h"
#include "csmi_handler_state.h"
#include "csmutil/include/timing.h"
#include "csmi/include/csm_api.h"

/** @brief The base class of stateful handlers.
 *
 */
class CSMIStateful : public CSMI_BASE 
{
private:
    std::vector<CSMIHandlerState*> _States; ///< The State Space of this handler.
    uint32_t _InitialState;                 ///< The Initial State for the execution.

public:
    CSMIStateful(
        csmi_cmd_t cmd, csm::daemon::HandlerOptions& options) : 
        CSMI_BASE(cmd,options),
        _InitialState(0) {}

    virtual void Process(
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final
    {
        // Grab the start time of the state.
        START_TIMING()

        // Get the context as a Handler context.
        csm::daemon::EventContextHandlerState_sptr  ctx;
        csm::daemon::EventContext_sptr gen_ctx = aEvent.GetEventContext();
        if (!gen_ctx)
        {
            ctx = CreateContext( CopyEvent(aEvent) );

            if ( ! ctx )
                throw csm::daemon::Exception("ERROR: Context type cannot be created.");

            // Log when the API is first called for a context.
            LOG (csmapi, info) << ctx << _cmdName << " start";
        }
        else
        {
            ctx = std::dynamic_pointer_cast<csm::daemon::EventContextHandlerState>(gen_ctx);
            if( ! ctx )
                throw csm::daemon::Exception(
                    "ERROR: Context type cannot be dyn-casted to EventContextHandlerState");
        }
    
        // Cache the state id for later.
        uint32_t stateID = ctx->GetAuxiliaryId();

        // If we're already in the Final State there's no reason to worry about this
        // context anymore. 
        if( stateID >= _States.size() )
            return;

        CSMIHandlerState* state = _States[stateID];
    
        // If this is a valid state, process it.
        if ( state )
            state->Process(ctx, aEvent, postEventList);
        else
            throw csm::daemon::Exception("ERROR: State was unable to be opened");

        // Grab the end time of the state.
        END_TIMING( csmapi, debug, ctx->GetRunID(), _cmdType, stateID )

        // Log when the API exits for a context.
        if( ctx->GetAuxiliaryId() >= _States.size() )
             LOG(csmapi, info) << ctx << _cmdName << " end";

        ctx.reset();
        gen_ctx.reset();
        //ctx = nullptr;
        //gen_ctx = nullptr;
    }
    
protected:
    inline uint32_t GetInitialState() { return _InitialState; }
    inline void     SetInitialState( uint64_t initialState ) { _InitialState = initialState; }

    inline void ResizeStates( uint32_t size ) { _States.resize( size ); }
    inline void SetState( uint32_t stateIndex, CSMIHandlerState* handlerState)
    {
        if ( stateIndex < _States.size() )
            _States[stateIndex] = handlerState;
        else
            LOG(csmapi, error) << "Specified state: " << stateIndex << "is an invalid state.";
    }

    /** @brief Creates a new context using the invoking handlers and its initial state.
     * @todo Should we replace this with a template/is there a better way?
     * @note If a handler extends this class it should override this function if the 
     *  context differs.
     * @param[in] aReqEvent The event affiliated with this context.
     *
     * @return The new context of the specified type.
     */
    virtual inline csm::daemon::EventContextHandlerState_sptr CreateContext(
        csm::daemon::CoreEvent *aReqEvent )
    {
        return std::make_shared<csm::daemon::EventContextHandlerState>(
            this, _InitialState, aReqEvent ); 
    }
};


#endif

