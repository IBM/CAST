/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_handler_state.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_HANDLER_STATE_H__
#define __CSMI_HANDLER_STATE_H__

#include "csmi_base.h"
#include "csmi_handler_context.h"
#include "csmd/src/db/include/csm_db_event_content.h"
#include "helpers/database_helpers.h"
#include "helpers/NetworkHelpers.h"

/**@brief The base abstract class for handling CSMIHandleStates.
 *
 * This class should not be implemented, please implement one of the Event State Classes:
 * NetworkMessageState, DBRespState, or SystemEventState.
 *
 **/
class CSMIHandlerState 
{
protected:
    std::string _StateName;  ///< The name of the state, for timing and debugging.

private:
    uint64_t _SuccessState;  ///< State to transition to in a success.
    uint64_t _AlternateState;///< Alternate state for successful transition.
    uint64_t _FailureState;  ///< State to transition to in a failure.
    uint64_t _TimeoutState;  ///< State to be set in the event of a timeout.
    uint64_t _FinalState;    ///< The final state of this state machine.
    uint64_t _TimeoutLength; ///< The length of time for the timeout.

public:
    CSMIHandlerState():
        _StateName("DEFAULT"),
        _SuccessState(0),
        _AlternateState(0),
        _FailureState(0),
        _TimeoutState(0),
        _FinalState(0),
        _TimeoutLength(csm::daemon::helper::BAD_TIMEOUT_LEN)
    { }

    CSMIHandlerState(const CSMIHandlerState &obj):
        _StateName(obj._StateName),
        _SuccessState(obj._SuccessState),
        _AlternateState(obj._AlternateState),
        _FailureState(obj._FailureState),
        _TimeoutState(obj._TimeoutState),
        _FinalState(obj._FinalState),
        _TimeoutLength(obj._TimeoutLength)
    {} 


    /** @brief Constructor which specifies the valid state transitions of this state.
     * Note: There are only four supported states by default. If the timeout is not
     * initialized, it is assumed that the default timeout behavior is desired for 
     * this state (Sends an error back to the local api.
     *
     *  @param[in] handler The stateful handler that owns this state.
     *  @param[in] success The state to transition to the state executes properly.
     *  @param[in] failure The state to transition to if a failure occurs.
     *  @param[in] final_state The final state, this state is NOT implemented!
     *  @param[in] timeout Optional. The state to transition to if a timeout event is recieved.
     *      If not set the default behavior will take control and an error message will be sent to
     *      the front end.
     *  @param[in] timeout_length Optional. The length of the timeout produced by this state.
     *      If not set, no timeout will be produced.
     *  @param[in] alternate An alternate state for a secondary "success" path in the diagram.
     */
    CSMIHandlerState( 
            uint64_t success, 
            uint64_t failure, 
            uint64_t final_state, 
            uint64_t timeout = csm::daemon::helper::BAD_STATE, 
            uint64_t timeout_length = csm::daemon::helper::BAD_TIMEOUT_LEN,
            uint64_t alternate = csm::daemon::helper::BAD_STATE,
            const std::string& state_name = "DEFAULT"):
        _StateName(state_name),
        _SuccessState(success),
        _AlternateState(alternate),
        _FailureState(failure),
        _TimeoutState(timeout),
        _FinalState(final_state),
        _TimeoutLength(timeout_length)
    {
        if( timeout == csm::daemon::helper::BAD_STATE )
            _TimeoutState = _FinalState;
    }

    /** @brief Most states probably don't need to implement this. */
    virtual ~CSMIHandlerState() {};

    /** @defgroup State_Abstract_Functions A Set of Abstract functions that a state must implement when writing a child of @ref CSMIHandlerState. */

    /** 
     * @brief Invoked by the Handler's Process Function.
     * @ingroup State_Abstract_Functions
     *
     * @param ctx The context of the event. Cast in the invoking function to 
     *  prevent every implementation of Process from having perform error checking. 
     *  This context has a pointer to the invoking handler.
     * @param aEvent The event that triggered this call.
     * @param postEventList The list of events to push new events to.
     */
    virtual void Process( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent& aEvent, 
        std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;


protected:

    /** @defgroup Dispatch_Functions A set of functions for pushing events, to be used by implementations of @ref CSMIHandlerState.
     * @{ */

    /** 
     * @brief Generates a more elaborate response to a timer event.
     * @ingroup State_Abstract_Functions 
     * 
     * Example reasons for implementing this function include: backing off a database event,
     * Sending a message to a number of nodes, etc. 
     *
     * This function must push the event to the postEventList if used.
     * Only implement this function if you've set a timeout state.
     *
     * If this function is unused in your state add the following to the header:
     * virtual void GenerateTimeoutResponse(
     *  csm::daemon::EventContextHandlerState_sptr ctx,
     *  std::vector<csm::daemon::CoreEvent*>& postEventList) final {  }
     *
     * @param ctx The context of the timer event spawning this code block.
     * @param postEventList The event list, used in registering the final event.
     */
    virtual void GenerateTimeoutResponse( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList) = 0;

    
    /** 
     * @brief Handles an ErrorEvent, this must be explicitly be defined by the state.
     * @ingroup State_Abstract_Functions
     *
     * 
     *
     * @param[in] ctx The context of the handler, contains the 
     *      Error Code and Error Message.
     * @param[in] aEvent The event that is being processed.
     * @param[in] postEventList The event list.
     * @param[in] byAggregator Specifies whether the error must be sent through the Aggregator.
     */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent& aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false) = 0;

    /** 
     * @brief Pushes a database request(DBReq) to the event list.
     * 
     * This functionality will verify that a reply was properly generated and 
     * pushed to the event list. If this fails, depending on supplied arguments, will
     * result in one of two behaviors:
     * 
     * 1. errorOnFail:true : The state will be set to the _FinalState and an error
     *  will be pushed to the event list (CSMERR_DB_ERROR).
     *
     * 2. errorOnFail:false : The state will be set to the _FailureState and the
     *  invoker will react to the failure. This is for behavior that has a more
     *  complicated failure state (backing off DB updates for example) (CSMERR_DB_ERROR)
     *  is set in the context).
     *
     *
     * @param[in]  dbPayload The parameterized payload for the database request.
     * @param[out] ctx The context of the event dispatching this request.
     * @param[out] postEventList The event list, event is pushed to this.
     * @param[in]  errorOnFail If error handling for a failure is state specific, set to false. 
     *  Otherwise, an error will be pushed to the event list. Defaults to true.
     *
     * @return True  If successful.
     * @return False If failed.
     */
    bool PushDBReq(
        csm::db::DBReqContent const &dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool errorOnFail = true );
    
    /** @brief Pushes a multicast(MCAST) message to the event list.
     *
     * @param targets Sorts the targets for multicast.
     * @param payload
     * @param payloadLen
     * @param ctx
     * @param postEventList The event list, event is pushed to this.
     * @param targetState The state to transition to for listening to the multicast response, defaults to Success.
     * @param errorOnFail If error handling for a failure is state specific, set to false. 
     *  Otherwise, an error will be pushed to the event list. Defaults to true.
     *
     *
     * @return True  If successful.
     * @return False If failed.
     */
    bool PushMCAST(
        csm::network::Message message,
        std::vector<std::string>& targets,
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        uint64_t targetState = UINT64_MAX,
        bool errorOnFail = true );
    
    /** @brief Forwards the supplied message to the master daemon.
     *
     * This will transition the state to the 
     *
     * @param[in] message       The message to pass to the master.
     * @param[in] ctx           The context of the handler.
     * @param[in] postEventList The event list, event is pushed to this.
     * @param[in] errorOnFail   If error handling for a failure is state specific, 
     *                              set to false. Otherwise, an error will be pushed 
     *                              to the event list. Defaults to true.
     *
     * @return True  If successful.
     * @return False If failed.
     */
    bool ForwardToMaster(
        csm::network::Message message,
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool errorOnFail = true );

    
    /**
     * @brief Pushes a reply message to the event list, this transitions the context state to Final.
     * 
     * This function assumes a reply with no detected errors.
     *
     * @param[in] buffer A buffer containing the message/data to be recieved locally.
     * @param[in] bufferLength The length of the supplied buffer in characters.
     * @param[in] ctx The context to extract the reply address from.
     * @param[in] postEventList The event list to push the reply to.
     * @param[in] byAggregator True: Sends the Message to the Aggregator node. False: Sends the message to the caller in the context.
     * 
     * TODO Should this return a boolean?
     * @return True: When the reply was added successfully to the event list.
     */
    bool PushReply(
        const char* buffer,
        const uint32_t bufferLength,
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false );

    // TODO DOCUMENT
    void PushRASEvent(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        const std::string &msg_id,
        const std::string &location_name,
        const std::string &raw_data,
        const std::string &kvcsv);

    /** @brief Spawns a timer with _TimeoutLength for the next state of the handler.
     *
     *  @param[in] ctx The context of the handler, sets the target state of the timer to the 
     *      Auxiliary Id of the context. 
     *  @param[in] postEventList The event list.
     */
    void PushTimeout(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList);

    /**
     * @brief The generic handler for timeout events, invokes GenerateTimeoutResponse.
     * TODO deeper document.
     */
    void HandleTimeout( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent& aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList );

    /** @brief Generates a Error Event.
     * 
     * By Default this function changes the state of the context to "Final" and
     * pushes an error event on the PostEventList. It is assumed that the 
     * invoking handler has set the error in the context.
     *
     * @param[in] ctx The context of the handler, contains the 
     *      Error Code and Error Message.
     * @param[in] aEvent The event that is being processed.
     * @param[in] postEventList The event list.
     * @param[in] byAggregator Specifies whether the error must be sent through the Aggregator.
     */
    void DefaultHandleError( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent& aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false ); 

private:
    /** @brief Pushes an event onto the postEvent list Stack.
     * TODO add more docs.
     */
    bool PushEvent(
        csm::daemon::CoreEvent *reply,
        uint64_t errorType, 
        const std::string& errorMessage,
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool errorOnFail = false );
    /** @} */ // End Dispatch_Functions

protected:
    /** @defgroup Getters State Machine
     * @{ */
    inline uint64_t GetSuccessState()   { return _SuccessState; };
    inline uint64_t GetAlternateState() { return _AlternateState; };
    inline uint64_t GetFailureState()   { return _FailureState; };
    inline uint64_t GetTimeoutState()   { return _TimeoutState; };
    inline uint64_t GetFinalState()     { return _FinalState;   };
    inline uint64_t GetTimeoutLength()  { return _TimeoutLength;};

public:
    /**
     * @brief A special operator for the state machine to set failure events that are "expected".
     *
     * This function should be used in cases where the state has failed, but something like a 
     * DBReq event must be sent.
     *
     * @param[in] ctx The context to set the state to "Failure".
     */
    inline void SetAsFailure(csm::daemon::EventContextHandlerState_sptr ctx)
    {
        ctx->SetAuxiliaryId(GetFailureState());
    }

    /**
     * @brief A special path of execution for the state machine.
     *
     * Sets the state machin on an alternate "success" path.
     *
     * @param[in] ctx The context to set the state to "Alternate".
     *
     */
    inline void SetAsAlternate(csm::daemon::EventContextHandlerState_sptr ctx)
    {
        ctx->SetAuxiliaryId(GetAlternateState());
    }
    /** @} */

    /** @brief Getter for the Name of the state.
     *
     * @return The name of the state as a string.
     */
    inline const std::string GetStateName() { return _StateName; }
};

#endif

