/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_mcast/CSMIMcastResponder.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**
 * @file CSMIMcastResponder.h
 *
 * @author John Dunham (jdunham@us.ibm)
 */

#ifndef CSMI_MCAST_RESPONDER_H
#define CSMI_MCAST_RESPONDER_H

#include <type_traits>
#include "../csmi_db_resp_state.h"
#include "CSMIMcast.h"
#include "CSMIMcastPrototypes.h"

#define ERR_MSG_DIVIDE " | "
#define STATE_NAME "McastResponder:"
#include "../csmi_network_message_state.h"

/** @brief Defines a state for handling multicast responses behavior is template driven.
 *
 * The following operations are performed in this state:
 *  -# Verify that the incoming network message is a valid response.
 *  -# Count the nodes that have responded from the compute agents.
 *  -# If the expected number of nodes return respond based on the template specification.
 *      -# This might be a direct response to the frontend or database request.
 *  -# If a timeout occured attempt to revert the previous states.
 *
 * @tparam PayloadType The type of the payload properties object.
 *
 * @tparam PayloadFT The function type of the payload generator for the success case
 *      of the multicast response. Currently supports the @ref TerminalByte 
 *      (for responses that need to return to the front end) and 
 *      @ref PayloadConstructor (for responses that interact with the Database) 
 *      function types.
 *
 * @tparam BuildSuccessPayload The function to invoke when the multicast responses
 *      have all responded successfully. This parameter is linked to @p PayloadFT 
 *      and uses SFINAE techniques to generate both the database interaction and
 *      response to the front end code paths. Please see @ref PayloadConstructor and
 *      @ref TerminalByte for formal definitions.
 *
 * @tparam DBRecover Invoked in @ref GenerateError, typically this will revert the 
 *      @ref CreatePayload database code, for default error handling this function
 *      should return a nullptr.
 *
 * @tparam PayloadParser Parses the multicast response and performs operations on the 
 *      recieved data, this function typically aggregates the results of the multicast.
 *
 * @tparam McastRecover A Template flag which selects the version of GenerateError
 *      to use. If set to True a recovery multicast will be generated using the
 *      member functions of the CSMIMcast object used in this handler instance.
 *      @ref CSMIMcast::PrepRecovery will be invoked followed by 
 *      @ref CSMIMcast::BuildMcastPayload, for unique behavior specialize these
 *      functions in the object definition. If set to False the multicast operation
 *      is not performed.
 */
template<typename PayloadType,
         typename PayloadFT,
         PayloadFT BuildSuccessPayload,
         PayloadConstructor<PayloadType> DBRecover,
         MCASTPayloadParser<PayloadType> PayloadParser, 
         bool McastRecover>
class McastResponder : public NetworkMessageState
{
    using NetworkMessageState::NetworkMessageState;

protected:

    /** @brief Handles the parsing and storage of the multicast response for the compute nodes.
     *
     *  @param[in] content The message received by the CSMIHandlerState from the compute node.
     *  @param[in] postEventList The event queue.
     *  @param[in,out] ctx The context of the event, holds details persistent across states.
     *
     *  @return True Everything went as planned and no errors were found.
     *  @return False An error was detected and details were placed in the context.
     */
    virtual bool HandleNetworkMessage(
        const csm::network::MessageAndAddress content,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) final
    {
        LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Enter";
        bool success = true;
        
        // Acquire the mutex lock before anything else.
        PayloadType* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock = 
            ctx->GetUserData<PayloadType*>(&mcastProps);
       
        // Parse the rsponse, only update if the message was correct.
        if( PayloadParser(mcastProps, content ) )
        {
            ctx->SetReceivedNumResponses(ctx->GetReceivedNumResponses() + 1);
        }

        // If the multicast has received the requisite number of responses, process the received data.
        if( ctx->ResponseCheck() )
        {
            // If this was a success we need to update the database tables.
            if ( ctx->GetErrorCode() == CSMI_SUCCESS  && !mcastProps->DidErrorOccur())
            {
                // Build the query the template function.
                dataLock.unlock();
                success = GenerateResponse(ctx, mcastProps, postEventList);
                dataLock.lock(); 
            }
            else
            {
                LOG(csmapi, error) << ctx << STATE_NAME " The multicast had errors.";
                success = false;
            }
        }

        dataLock.unlock();

        LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Exit";

        return success;
    }

    /** @brief Generates a database response after this state receives all of the multicast responses.
      * 
      * Executes the @ref BuildSuccessPayload to generate the database request.
      *
      * @tparam FT The Function type of the response generator, used in SFINAE 
      *     conditional compilation.
      *
      * @param[in] ctx The context object of this state.
      * @param[in] mcastProps A multicast properties object containing details about the multicast.
      * @param[in,out] postEventList The event queue, the database request is pushed here.
      *
      * @return True If the database query could be built.
      */
    template<typename FT=PayloadFT,
             typename std::enable_if<std::is_same<FT, PayloadConstructor<PayloadType>>::value>::type* = nullptr>
    inline bool GenerateResponse( 
        csm::daemon::EventContextHandlerState_sptr ctx, 
        PayloadType* mcastProps, 
        std::vector<csm::daemon::CoreEvent*>& postEventList )
    {
        csm::db::DBReqContent *dbReq = BuildSuccessPayload(ctx, mcastProps);
        bool success = true;
        
        if( dbReq )
        {
            this->PushDBReq( *dbReq, ctx, postEventList );
            delete dbReq;
        }
        else
        {
            LOG(csmapi, error) << ctx << STATE_NAME "Unable to build the database query.";
            success = false;
        }
        return success;
    }

    /** @brief Generates a network reply to the frontend after this state receives all of the multicast responses.
      * 
      * Executes the @ref BuildSuccessPayload to generate the network reply.
      *
      * @tparam FT The Function type of the response generator, used in SFINAE 
      *     conditional compilation.
      *
      * @param[in] ctx The context object of this state.
      * @param[in] mcastProps A multicast properties object containing details about the multicast.
      * @param[in,out] postEventList The event queue, the reply is pushed here. 
      *
      * @return True If the database query could be built.
      */
    template<typename FT=PayloadFT, 
             typename std::enable_if<std::is_same<FT, TerminalByte>::value>::type* = nullptr>
    inline bool GenerateResponse( 
        csm::daemon::EventContextHandlerState_sptr ctx, 
        PayloadType* mcastProps, 
        std::vector<csm::daemon::CoreEvent*>& postEventList )
    {
        // Define the variables.
        char* buffer          = nullptr;
        uint32_t bufferLength = 0;
        bool success          = true;

        // Build the payload.
        BuildSuccessPayload( &buffer, bufferLength, ctx );

        this->PushReply(buffer, bufferLength, ctx, postEventList);
        ctx->SetUserData(nullptr);
        
        // If the buffer was created send a reply and return
        if( buffer )
        {
            free(buffer);
        }
        return success;
    }

    virtual void ProcessError( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csmi_err_t* error ) final
    {
        // If the error was specified cache.
        if ( error )
        {
            int errorcode = error->errcode != ETIMEDOUT ? error->errcode : CSMERR_TIMEOUT;
            PayloadType* mcastProps = nullptr;
            std::unique_lock<std::mutex>dataLock = 
                ctx->GetUserData<PayloadType*>(&mcastProps);

            std::string errMsg = std::string( error->errmsg );

            // The following assumes the format:
            // <ID>;<Hostname>;<RespType>;<Message>
            char* hostname; // The hostname of the compute node that sent the message.
            char* savePtr;  // A save pointer for the strtok parse.
            char* tokenStr = strtok_r(error->errmsg, "; ", &savePtr); // Get the id first.
            
            // If the token string was valid, attempt to get the hostname.
            if ( tokenStr  && (hostname = strtok_r(NULL, "; ", &savePtr)))
            {
                mcastProps->SetHostError(hostname, errorcode); 
            }
            
            // Append the error message and set the error code.
            ctx->AppendErrorMessage( errMsg );
            ctx->SetErrorCode( errorcode );
        }
        else
        {
            ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR );
            ctx->AppendErrorMessage( "Unable to unpack error message" );
        }
    }

    /** @brief Generates a timeout response through invocation of @ref GenerateError.
     *
     * @param[in] ctx The context for the handler.
     * @param[in,out] postEventList The event queue.
     */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final
    {
       LOG( csmapi, trace ) << STATE_NAME ":GenerateTimeoutResponse: Enter";
        LOG(csmapi,debug)   << "Timeout Error Code: " << ctx->GetErrorCode();

       ctx->SetReceivedNumResponses(ctx->GetExpectedNumResponses());
       GenerateError(ctx, *(ctx->GetReqEvent()), postEventList);
       
       LOG( csmapi, trace ) << STATE_NAME ":GenerateTimeoutResponse: Exit";
    }

    /** @brief Pass through function to @ref GenerateError.
     *
     * @param[in] ctx The context for the handler.
     * @param[in] aEvent The event that is being processed.
     * @param[in,out] postEventList The event queue.
     * @param[in] byAggregator Specifies whether the error must be sent through the Aggregator.
     */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator ) final
    {
        this->GenerateError(ctx, aEvent, postEventList, byAggregator);
    }

    /** @brief Attempts to revert the database to its state before the multicast started.
     *
     * This implementation will first determine if all of the nodes have responded. 
     * The wait is performed to prvent missing any errors from remote nodes.
     * After receiving responses from all of the nodes a recovery multicast is first 
     * attempted, then a database rollback and finally a return to the frontend.
     *
     * @todo There's a potential bug in the counting operation, see function comments for details.
     *
     * @tparam M Whether or not this state should attempt a recovery multicast.
     *      This implementation is for instances where a multicast is performed.
     * 
     * @param[in] ctx The context for the handler.
     * @param[in] aEvent The event that is being processed.
     * @param[in,out] postEventList The event queue.
     * @param[in] byAggregator Specifies whether the error must be sent through the Aggregator.
     */
    template<bool M=McastRecover, typename std::enable_if<M>::type* = nullptr>
    void GenerateError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator=false )
    {
        LOG( csmapi, trace ) << STATE_NAME ":GenerateError: Enter";
        
        // FIXME if something times out then returns an error, this breaks things.
        ctx->SetReceivedNumResponses(ctx->GetReceivedNumResponses() + 1);

        PayloadType* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock = 
            ctx->GetUserData<PayloadType*>(&mcastProps);

        if ( mcastProps )
        {
            // Push the errorcode onto the heap to be sure
            mcastProps->PushError(ctx->GetErrorCode());
        }
        
        // First verify that all of the nodes have responded.
        // If all the nodes have responded attempt to recover (dependant on implementation).
        if( ctx->ResponseCheck() )
        {
            // If the error code was a success set to the generic multicast response error code.
            if ( ctx->GetErrorCode() == CSMI_SUCCESS ) 
                ctx->SetErrorCode(CSMERR_MULTI_RESP_ERROR);
            
            // Get the multicast properties first.
            //PayloadType* mcastProps = nullptr;
            //std::unique_lock<std::mutex>dataLock = 
            //    ctx->GetUserData<PayloadType*>(&mcastProps);

            // Helper Variables.
            bool success = mcastProps != nullptr;
            char* buffer = nullptr;
            uint32_t bufferLength = 0;

            // Make sure the multicast properties object exists before operating.
            if( success )
            {
                // Prepare the multicast for recovery.
                mcastProps->PrepRecovery();

                // Attempt to build the payload.
                mcastProps->BuildMcastPayload(&buffer, &bufferLength);

                // Push the errorcode onto the heap to be sure
                mcastProps->PushError(ctx->GetErrorCode());
                
                // Append additional errors.
                ctx->AppendErrorMessage(ERR_MSG_DIVIDE);
                ctx->AppendErrorMessage(mcastProps->GetErrorMessage());
             
                // Set the Error Code.
                ctx->SetErrorCode(mcastProps->GetMainErrorCode());
            }

            ctx->AppendErrorMessage(ERR_MSG_DIVIDE);

            // If the buffer was defined attmept to multicast.
            if ( buffer )
            {
                csm::network::Message message;
                std::vector<std::string> nodeList = mcastProps->UpdateNodeMap();

                // Attempt to initialize the multicast, sending on success.
                if( message.Init( mcastProps->GetCommandType(), 0,
                                    CSM_PRIORITY_DEFAULT, 0, 
                                    MASTER, AGGREGATOR,
                                    geteuid(), getegid(), 
                                    std::string( buffer, bufferLength ) ) )
                {
                    LOG(csmapi,debug) << STATE_NAME " Pushing Recovery Multicast message.";
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
            
            // If the multicast failed, attempt to recover via database.
            if( !success )
            {
                if ( mcastProps )
                {
                    // Only generate if the multicast isn't executed on this step.
                    mcastProps->GenerateRASEvents( postEventList ,ctx);
                    
                    // Attempt the database recovery function.
                    csm::db::DBReqContent *dbReq = DBRecover(ctx, mcastProps);
                    if( dbReq )
                    {
                        this->PushDBReq( *dbReq, ctx, postEventList );
                        this->SetAsFailure( ctx );
                        delete dbReq;
                    }
                    else
                    {
                        ctx->AppendErrorMessage(mcastProps->GenerateErrorListing());
                        LOG(csmapi, error) << ctx << STATE_NAME "Unable to build the response query.";
                        dataLock.unlock();
                        CSMIHandlerState::DefaultHandleError(ctx, aEvent, postEventList, byAggregator);
                    }
                }
                else
                {
                    LOG(csmapi, error) << ctx << STATE_NAME "Unable to build the response query.";
                    dataLock.unlock();
                    CSMIHandlerState::DefaultHandleError(ctx, aEvent, postEventList, byAggregator);
                }
            }
        }

        LOG( csmapi, trace ) << STATE_NAME ":GenerateError: Exit";
    }

    /** @brief Attempts to revert the database to its state before the multicast started.
     *
     * This implementation will first determine if all of the nodes have responded. 
     * The wait is performed to prvent missing any errors from remote nodes.
     * After receiving responses from all of the nodes a a database rollback is attempted
     * and if this fails an error is sent to the frontend.
     *
     * @todo There's a potential bug in the counting operation, see function comments for details.
     *
     * @tparam M Whether or not this state should attempt a recovery multicast.
     *      This implementation is for instances where a multicast is not performed.
     * 
     * @param[in] ctx The context for the handler.
     * @param[in] aEvent The event that is being processed.
     * @param[in,out] postEventList The event queue.
     * @param[in] byAggregator Specifies whether the error must be sent through the Aggregator.
     */
    template<bool M=McastRecover, typename std::enable_if<!M>::type* = nullptr>
    void GenerateError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false ) 
    {
        LOG( csmapi, trace ) << STATE_NAME ":GenerateError: Enter";
        
        // FIXME if something times out then returns an error, this breaks things.
        ctx->SetReceivedNumResponses(ctx->GetReceivedNumResponses() + 1);

        PayloadType* mcastProps = nullptr;
        std::unique_lock<std::mutex>dataLock = 
            ctx->GetUserData<PayloadType*>(&mcastProps);

        if ( mcastProps )
        {
            // Push the errorcode onto the heap to be sure
            mcastProps->PushError(ctx->GetErrorCode());
        }
        
        // First verify that all of the nodes have responded.
        // If all the nodes have responded attempt to recover (dependant on implementation).
        if( ctx->ResponseCheck())
        {
            // If the error code was a success set to the generic multicast response error code.
            if ( ctx->GetErrorCode() == CSMI_SUCCESS ) 
                ctx->SetErrorCode(CSMERR_MULTI_RESP_ERROR);
            
            if ( mcastProps )
            {
                // Generate the RAS event for the failure cases.
                mcastProps->GenerateRASEvents( postEventList ,ctx);
                
                // Append additional errors.
                ctx->AppendErrorMessage(ERR_MSG_DIVIDE);
                ctx->AppendErrorMessage(mcastProps->GetErrorMessage());
                ctx->AppendErrorMessage(ERR_MSG_DIVIDE);


                // Set the Error Code.
                ctx->SetErrorCode(mcastProps->GetMainErrorCode());
               
                // Attempt the database recovery function.
                csm::db::DBReqContent *dbReq = DBRecover(ctx, mcastProps);
                if( dbReq )
                {
                    this->PushDBReq( *dbReq, ctx, postEventList );
                    this->SetAsFailure( ctx );
                    delete dbReq;
                }
                else
                {
                    ctx->AppendErrorMessage(mcastProps->GenerateErrorListing());
                    LOG(csmapi, error) << ctx << STATE_NAME " Unable to build the response query.";
                    dataLock.unlock();
                    CSMIHandlerState::DefaultHandleError(ctx, aEvent, postEventList, byAggregator);
                }
            }
            else
            {
                LOG(csmapi, error) << ctx << STATE_NAME " Unable to build the response query.";
                dataLock.unlock();
                ctx->AppendErrorMessage(ERR_MSG_DIVIDE);
                CSMIHandlerState::DefaultHandleError(ctx, aEvent, postEventList, byAggregator);
            }
        }

        LOG( csmapi, trace ) << STATE_NAME ":GenerateError: Exit";
    }
};

#undef STATE_NAME
#endif

