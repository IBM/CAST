/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcastTerminal.h

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

#ifndef CSMI_MCAST_TERMINAL_H
#define CSMI_MCAST_TERMINAL_H

#include <type_traits>
#include "../csmi_db_resp_state.h"
#include "CSMIMcast.h"
#include "CSMIMcastPrototypes.h"

#define STATE_NAME "McastTerminal:"
#include "../csmi_network_message_state.h"


/** @brief Defines a terminal state for multicast based handlers.
 *  If this state is reached with no errors, it simply returns to the front end, otherwise
 *  A recovery multicast is attempted.
 * @tparam PayloadType The type of the payload properties object.
 *
 * @tparam InforParser A function to parse the database response and ensure it was valid and/or
 *      populate the payload object with details from the database.
 *
 * @tparam CreateByteArray Generates a byte array to return to the frontend API.
 *
 * @tparam DBRecover Invoked in @ref GenerateError, this should revert any database changes.
 */
template<typename                        PayloadType,
         PayloadParser<PayloadType>      InfoParser,
         TerminalByte                    CreateByteArray,
         PayloadConstructor<PayloadType> DBRecover>
class McastTerminal : public DBRespState
{
    using DBRespState::DBRespState;

protected:

    /** @brief Parses and responds to a DBResp event, generally sending a message to the frontend.
     * @param[in] tuples The tuples returned in the DBResp event.
     * @param[in] postEventList The event queue.
     * @param[in,out] ctx The context of the event, holds details persistent across states.
     *
     * @return True if no unhandled errors were detected.
     */
    virtual bool HandleDBResp(
        const std::vector<csm::db::DBTuple *>& tuples,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) final
    {
        LOG( csmapi, trace ) << STATE_NAME ":HandleDBResp: Enter";

        // Get the payload object.
        PayloadType* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock =
            ctx->GetUserData<PayloadType*>(&mcastProps);

        bool success = InfoParser(ctx, tuples, mcastProps);
        dataLock.unlock();

        // If the parser was successful attempt to send a message to the frontend.
        if( success )
        {
            char*    buffer       = nullptr;            
            uint32_t bufferLength = 0;

            success = CreateByteArray(&buffer, bufferLength, ctx);
            if( success )
            {
                this->PushReply( buffer, bufferLength, ctx, postEventList, false);
            }
            
            if(buffer) free(buffer);
        }

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
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator ) final
    {
        LOG( csmapi, trace ) << STATE_NAME ":HandleError: Enter";
        
        PayloadType* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock =
            ctx->GetUserData<PayloadType*>(&mcastProps);

        char*    buffer       = nullptr;
        uint32_t bufferLength = 0;
        bool     success      = mcastProps != nullptr;

        if( success )
        {
            // Build a recovery multicast.
            mcastProps->PrepRecovery();
            mcastProps->BuildMcastPayload(&buffer, &bufferLength);
        }
        
        // If the buffer could be made, attempt to pusha multicast.
        if( buffer )
        {
            csm::network::Message message;
            std::vector<std::string> nodeList = mcastProps->UpdateNodeMap();

            // If the message was initialized, push it onto the event list.
            if( message.Init( mcastProps->GetCommandType(), 0,
                                CSM_PRIORITY_DEFAULT, 0,
                                MASTER, AGGREGATOR,
                                geteuid(), getegid(),
                                std::string( buffer, bufferLength ) ) )
            {
                success = this->PushMCAST( message, nodeList, ctx, 
                                            postEventList, GetTimeoutState() );
                dataLock.unlock();
            }
            else
            {
                ctx->AppendErrorMessage("Unable to construct the Recovery multicast message.");
                success = false;
            }

            free(buffer);
        }
        else
        {
            ctx->AppendErrorMessage("Unable to serialize Recovery Multicast Payload.");
            success = false;
        }

        // If the multicast was a failure attempt to clean up the database.
        if( !success )
        {
            csm::db::DBReqContent *dbReq = DBRecover(ctx, mcastProps);
            dataLock.unlock();

            if( dbReq )
            {
                this->PushDBReq( *dbReq, ctx, postEventList );
                this->SetAsFailure( ctx );
                delete dbReq;
            }
            else
            {
                ctx->AppendErrorMessage("Unable to build the response query.");
                CSMIHandlerState::DefaultHandleError(ctx, aEvent, postEventList, byAggregator);
            }
        }

        LOG( csmapi, trace ) << STATE_NAME ":HandleError: Exit";
    }

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};


#undef STATE_NAME
#endif

