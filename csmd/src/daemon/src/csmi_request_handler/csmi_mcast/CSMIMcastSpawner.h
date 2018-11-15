/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcastSpawner.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
/**
 * @file CSMIMcastSpawner.h
 * @author John Dunham (jdunham@us.ibm.com) 
 */

#ifndef CSMI_MCAST_SPAWNER_H
#define CSMI_MCAST_SPAWNER_H

#include "../csmi_db_resp_state.h"
#include "CSMIMcast.h"
#include "CSMIMcastPrototypes.h"

#define STATE_NAME "McastSpawner:"

/** @brief Defines a state for spawning multicasts, behavior is template driven.
 *
 * The following operations are performed by this class:
 *  -# Verify the Database communication was valid and extract any meaningful values.
 *  -# Determine if a multicast should be preformed.
 *  -# If a multicast should be performed, create an abridged payload.
 *  -# Construct the message to sent to the nodes and send it.
 *
 * @todo Make a smaller payload for sending out.
 * @todo Generalize this class a bit more (not sure if it's worth the effort).
 * @todo get rid of virtual.
 * @tparam PayloadType The type of the payload properties object.
 *
 * @tparam InfoParser  Parses the results of the previous database query. 
 *      Returns true if the parsed information allows the multicast operation to be 
 *      performed. Returns false if an early exit condition of any kind is reached.
 *      Invoked in HandleDBResp.
 *
 * @tparam FailurePayload Function to construct the Database Request payload,
 *      returns a DBReqContent object. Invoked inHandleError. This payload reverts 
 *      the CreatePayload function typically.
 *
 * @tparam CreateByteArray Generates a byte array to return to the frontend API.
 *      Invoked in HandleError (typically if the state has an early exit).
 */
template<typename PayloadType,
         PayloadParser<PayloadType> InfoParser, 
         PayloadConstructor<PayloadType> FailurePayload, 
         TerminalByte CreateByteArray>
class McastSpawner : public DBRespState
{
    using DBRespState::DBRespState;

protected:
    /** @brief Parses and responds to a DBResp event, typically spawning a multicast.
     * @param[in] tuples The tuples returned in the DBResp event.
     * @param[in] postEventList The event queue.
     * @param[in,out] ctx The context of the event, holds details persistent across states.
     *
     * @return True if no unhandled errors were detected.
     */
    virtual bool HandleDBResp(
        const std::vector<csm::db::DBTuple *>& tuples,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr& ctx ) final
    {
        LOG( csmapi, trace ) << STATE_NAME ":HandleDBResp: Enter";
        // Helper Variables.
        bool success = true;
        char *buffer = nullptr;
        uint32_t bufferLength = 0;

        // Get the properties of the allocation.
        PayloadType* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock = 
            ctx->GetUserData<PayloadType*>(&mcastProps);

        // Parse the request, if the parse was valid we're in the sucess state.
        success = InfoParser(ctx, tuples, mcastProps);

        // Build the multicast if the previous step was a success.
        if( success )
        {
            // Initialize the payload.
            mcastProps->BuildMcastPayload(&buffer, &bufferLength );

            // If the buffer was made, create the message.
            if(buffer)
            {
                csm::network::Message message;
                std::vector<std::string> nodeList = mcastProps->UpdateNodeMap();

                if( message.Init( mcastProps->GetCommandType(), 0, 
                                    CSM_PRIORITY_DEFAULT, 0, 
                                    MASTER, AGGREGATOR,
                                    geteuid(), getegid(), 
                                    std::string( buffer, bufferLength ) ) )
                {
                    LOG(csmapi,debug) << STATE_NAME " Pushing Multicast message.";

                    // The target state for the Multicast response.
                    int64_t target_state = UINT64_MAX;
                    
                    // If processing indicates that the alternate route should be used
                    // take the alternate route (handler specific).
                    if ( mcastProps->IsAlternate() )
                    {
                        this->SetAsAlternate(ctx);
                        mcastProps->SetAlternate(false);
                        target_state = ctx->GetAuxiliaryId();
                    }

                    success = this->PushMCAST( message, nodeList, ctx, 
                        postEventList, target_state);
                }
                else
                {
                    ctx->SetErrorCode(CSMERR_MULTI_GEN_ERROR);
                    ctx->SetErrorMessage("Unable to construct the multicast message.");
                    success = false;
                }

                free(buffer);
            }
            else
            {
                ctx->SetErrorCode(CSMERR_MSG_PACK_ERROR);
                ctx->SetErrorMessage("Unable to serialize Multicast Payload.");
                success = false;
            }
        }
        dataLock.unlock();

        LOG( csmapi, trace ) << STATE_NAME ":HandleDBResp: Exit";
        return success;
    }


    /** @brief Attempts to revert the database to its state before this handler was invoked.
     *
     * 
     * @param[in] ctx The context for the handler.
     * @param[in] aEvent The event that is being processed.
     * @param[in] postEventList The event queue.
     * @param[in] byAggregator Specifies whether the error must be sent through the Aggregator.
     */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator ) final 
    {
        LOG( csmapi, trace ) << STATE_NAME ":HandleError: Enter";

        // Gtab the properties.
        PayloadType* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock = 
            ctx->GetUserData<PayloadType*>(&mcastProps);

        // Determine if this needs to perform error recovery.
        bool failureRecovery= ctx->GetErrorCode() != CSMI_SUCCESS;

        // If this state replies to the frontend and failure recovery wasn't triggered.
        // Attempt to react to create in the expected manner.
        if( ( mcastProps && mcastProps->DoesEarlyExitReply()) && !failureRecovery )
        {
            // Forgot to free buffer.
            char* buffer = nullptr;
            uint32_t bufferLength = 0;

            // Unlock the data, because CreateByteArray may access mcastProps if necessary.
            dataLock.unlock();
            CreateByteArray( &buffer, bufferLength, ctx );

            this->PushReply(buffer, bufferLength, ctx, postEventList);
            ctx->SetUserData(nullptr);

            // If the buffer was created send a reply and return.
            if( buffer )
            {
                free(buffer);
            }
            LOG( csmapi, trace ) << STATE_NAME ":HandleError: Exit";
            return; 
        }
        
        // If the early exit doesn't reply or a failure was found.
        // Attempt to perform the necessary database changes.
        if( mcastProps && (!mcastProps->DoesEarlyExitReply() || failureRecovery ) )
        {
            csm::db::DBReqContent* content = FailurePayload(ctx, mcastProps);
            // Unlock the data, as we no longer need it.
            dataLock.unlock();

            if( content )
            {
                this->PushDBReq( *content, ctx, postEventList );
                this->SetAsFailure( ctx );
                delete content;
            }
            else 
            {
                CSMIHandlerState::DefaultHandleError(ctx, aEvent, postEventList);
            }
        }

        LOG( csmapi, trace ) << STATE_NAME ":HandleError: Exit";
    }

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr& ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};

#undef STATE_NAME

#endif
