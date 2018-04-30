/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_handler_context.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_HANDLER_STATE_CONTEXT_H__
#define __CSMI_HANDLER_STATE_CONTEXT_H__

#include "include/csm_core_event.h"
#include <mutex>
#include <functional>
#include <ostream>

namespace csm {
namespace daemon{



/** @brief A handler for the majority of Handler States, if a handler uses States it must use this class as a base for its contexts.
 *
 */
class EventContextHandlerState : public csm::daemon::EventContext 
{
private:
    // An enum for tracking whether the API should perform a permission check.
    // Run Data
    uint32_t      _RunID;                        /*! The run id of the handler (defined by the 
                                                   Reserved ID), immutable after 
                                                   creation; Defaults to UINT32_MAX*/
    // Database Data
    bool          _HasPrivateAccess;             /*! Flag for whether the user has access to 
                                                   private APIs; Defaults to true, access is 
                                                   revoked in current design. */
    std::mutex    _PrivateMutex;                 ///< Mutex for the private access of the context.

    std::mutex    _UserIDMutex;                  ///< Mutex for the user id access of the context.
    int64_t       _UserID;                       ///< The user id of the invoking context.
    
    bool          _PrivateCheck;                 ///< Whether the API should perform a private check.
    std::mutex    _PrivateCheckMutex;            ///< Mutex for determing if a private check should be performed.

    // MCAST/PTP Data
    uint32_t      _ExpectedNumResponses;         ///< Number of responses expected by the state.
    std::mutex    _ExpectedMutex;                ///< Mutex lock for responses expected.

    uint32_t      _ReceivedNumResponses;         ///< Number of responses received by the state.
    std::mutex    _ReceivedMutex;                ///< Mutex lock for the responses received.

    // General Data.
    int           _ErrorCode;                    ///< Holds the error code for error events.
    std::mutex    _ErrorCodeMutex;               ///< A mutex lock for the error code.

    std::string   _ErrorMessage;                 ///< Holds the error message for error events.
    std::mutex    _ErrorMessageMutex;            ///< A mutex lock for the error string.

    void*         _UserData;                     ///< Holds a pointer to the user data.
    std::mutex    _UserDataMutex;                ///< A mutex lock for the user data.

    std::function<void(void*)> _DataDestructor;  ///< Holds the destructor for the UserData.
    std::mutex                 _DestructorMutex; ///< A mutex for the Data Destructor.
public:

    EventContextHandlerState(
        void *aEventHandler, uint64_t aUid, 
        csm::daemon::CoreEvent *aReqEvent = nullptr );

    EventContextHandlerState(const csm::daemon::EventContext_sptr aCtx);
   
    /** @brief Invokes the _DataDestructor function if _UserData is not.
     * Sets _UserData to nullptr.
     */
    virtual ~EventContextHandlerState();

    /** @brief Context logger outputs a unique identifier for this context object.
     *
     * @param [in]os The @ref ostream operator.
     * @param [in]ctx The context to output
     */
    friend std::ostream& operator<<(std::ostream& os, const EventContextHandlerState* ctx);


    /** @defgroup Getters_and_Setters 
     * @{ */
    /** @brief Gets the run id, set at the instantiation of the context object and immutable over its lifetime. 
     *  @return The unique identifier of the context object.
     */
    uint32_t GetRunID( );
    
    /** 
     *  @brief Sets the private access of the of the context for handler execution, retrieves a mutex lock.
     *  @param[in] check Whether or not this context should have private access in executing the API.
     */
    void SetHasPrivateAccess( bool check );

    /** 
     * @brief Retrieves the access level for the conext in handler execution.
     *  @return The access level.
     */
    bool GetHasPrivateAccess( );

    /**
     * @brief Sets up the @ref _UserID and @ref _PrivateCheck fields.
     * Updates the fields if and only if a user hasn't been set for the context.
     *
     * @param[in] userID The new user id (if user id hasn't been set).
     * @param[in] performPrivateCheck Whether the API should perform aprivate check.
     */
    void SetPrivateCheckConfig(int64_t userID, bool performPrivateCheck);

    /**
     * @brief Performs a check to see if the user can legally access the API.
     * Checks @ref _UserID and @ref _PrivateCheck.
     *
     * @param[in] userID The user id to check for access.
     *
     * @return True if the user has access.
     */
    bool CanUserAccess(int64_t userID);

    /**@brief Sets the expected number of responses for a multicast job, retrieves a mutex lock.
     * @param[in] responses The number of responses expected for a multicast.
     */ 
    void SetExpectedNumResponses( uint32_t responses );

    /**@brief Retrieves the number of responses expected for a multicast job.
     * @return The number of responses expected by a multicast.
     */
    uint32_t GetExpectedNumResponses( );

    /** @brief Sets the number of responses received by a multicast, retrieves a mutex lock.
     *  @param[in] response The number of responses.
     */
    void SetReceivedNumResponses( uint32_t responses );
    
    /** @brief Retrieves the number of responses received by a multicast.
     *  @return The number of responses.
     */ 
    uint32_t GetReceivedNumResponses( );
    
    /** @brief Checks to see if all of the responses were received.
     * @return True if the number of received responses exceeds or equals the expected number.
     */
    bool ResponseCheck();

    /** @brief Sets the error code of the context, retrieves a mutex lock.
     *  @param[in] errorCode The new error code.
     */
    void SetErrorCode( int errorCode );

    /** @brief Retrieves the error code of the context.
     *  @return The current error code for the context.
     */
    int GetErrorCode( );

    /** @brief Set the error message, retrieves a mutex lock.
     *  @param[in] message The message to set for the error.
     */
    void SetErrorMessage( const std::string & message );

    /** @brief Appends a string to the existing error message.
     *  @param[in] message A message to append to the existing error message.
     *  @param[in] joiner A character for joining the new message with the old one, ` ` is default.
     */
    void AppendErrorMessage( const std::string & message, char joiner = ' ' );

    /** @brief Prepends the supplied message to the error message.
     *  @param[in] message The message to prepend.
     *  @param[in] joiner The character to join the string.
     *
     *  @return The composed string.
     */
    void PrependErrorMessage( const std::string & message, char joiner );

    /** @brief Generates a unique string for the context "<api_name>[<run_id>]".
     *
     * @return A string in the format of "<api_name>[<run_id>]".
     */
    std::string GenerateUniqueID();

    /** @brief Retrieves a constant reference to the error message.
     *  @return The error message.
     */
    const std::string GetErrorMessage( );
    
    /** @brief Using lambda syntax, define the destructor for the void* _UserData.
     * A Sample function would look something like 
     *  [](void* var) { delete (test*)var; }
     *
     *  @param[in] dest A Destructor tuned for the dynamically declared context.
     */
    void SetDataDestructor( std::function<void(void*)> dest );
    void SetUserData( void* userData );

    /**
     * @brief Get the command name from the event handler.
     * @return The string representing the command.
     */
    std::string GetCommandName();

    /** @brief Retrieves the user data, performing a static cast to the correct type.
     * 
     * @tparam T The class for the _UserData void pointer.
     * @return The User Data cast to the template type.
     */
    template<class T>
    inline std::unique_lock<std::mutex>  GetUserData( T* dataContainer ) 
    { 
        std::unique_lock<std::mutex> lock(_UserDataMutex);
        *dataContainer = static_cast<T>( _UserData ); 

        return lock;
    }

    /** @} */ // Getters_and_Setters
   
};

using EventContextHandlerState_sptr = std::shared_ptr<EventContextHandlerState>;

} // End daemon
} // End csm

#endif

