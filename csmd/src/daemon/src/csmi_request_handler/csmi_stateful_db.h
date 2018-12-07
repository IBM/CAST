/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_STATEFUL_DB_H__
#define __CSMI_STATEFUL_DB_H__

#include "csmi_stateful.h"

enum StatefulStatesDefault
{
    STATEFUL_DB_INIT = 0,
    STATEFUL_DB_RECV_PRI,
    STATEFUL_DB_RECV_DB,
    STATEFUL_DB_DONE
};

/**
 * @brief A stateful handler for database interactions.
 */
class CSMIStatefulDB : public CSMIStateful{
public:

    /**
     * @brief Constructs a generic state machine for processing database events.
     *
     * The default state machine consists of 3 states:
     *
     *  1. @ref StatefulDBInit
     *  2. @ref StatefulDBRecvPrivate
     *  3. @ref StatefulDBRecv
     *
     *
     * @param[in] cmd The API's @ref csmi_cmd_t enumerated type.
     * @param[in] options A daemon attribute used for registering event contexts to system events.
     * @param[in] numStates The number of states present in the handler. If set to a value lower
     *                          than @ref STATEFUL_DB_DONE no states are initialized
     */
    CSMIStatefulDB(
        csmi_cmd_t cmd, 
        csm::daemon::HandlerOptions& options, 
        uint32_t numStates = STATEFUL_DB_DONE );

  /**
   * @brief Creates a Database payload using parameterization. 
   *
   * @param[in]  arguments A Buffer containing a serialized structure, API dependent.
   * @param[in]  len       The length of the argument buffer.
   * @param[out] dbPayload A Payload containing the database request.
   * @param[in]  ctx       The context of the handler, error details and private check status.
   *
   * @return The success of the generation of the payload.
   */
    virtual bool CreatePayload(
        const std::string& arguments, 
        const uint32_t len,
        csm::db::DBReqContent **dbPayload, 
        csm::daemon::EventContextHandlerState_sptr& ctx )  = 0;
    
    /**
     * @brief Assembles a buffer to respond to the user.
     *
     * @param[in]  tuples The database tuples retrived from the database query.
     * @param[out] buf    A serialized message to send to the customer.
     * @param[out] bufLen The length of the generated buffer.
     * @param[in]  ctx    The context of the handler, error details and private check status.
     * 
     * @return The success of the generation of the Byte Array.
     */
    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples, 
        char **buf, uint32_t &bufLen, 
        csm::daemon::EventContextHandlerState_sptr& ctx ) = 0;

  /**
   * @brief Decodes the user data and assembles a query to perform a private check.
   *
   * @param[in]  arguments A Buffer containing a serialized structure, API dependent.
   * @param[in]  len       The length of the argument buffer.
   * @param[out] dbPayload A Payload containing the database request.
   * @param[in]  ctx       The context of the handler, error details.
   *
   * @return The success of generating the private check (only matters for private APIs).
   */
    virtual bool RetrieveDataForPrivateCheck(
        const std::string& arguments, 
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
    { 
        return false; 
    }

  /**
   * @brief Verifies that the data is available to the user in the case of private apis.
   *
   * @param[in]  tuples The database tuples retrived from the database query.
   * @param[in]  msg    The original message that spawned the handler invocation.
   *
   * @return Returns if the data is available to the user.
   */
    virtual bool CompareDataForPrivateCheck(
        const std::vector<csm::db::DBTuple *>& tuples, 
        const csm::network::Message &msg,
        csm::daemon::EventContextHandlerState_sptr& ctx)
    {
        LOG( csmapi, trace ) << "CompareDataForPrivateCheck: Enter";
	    
        bool success = false;

        if ( tuples.size() == 1 && tuples[0]->data )
        {
	    	uint32_t userID = strtoul(tuples[0]->data[0], nullptr, 10);
            success = (userID == msg.GetUserID());
            
            // If the success failed report it.
            if (success)
            {
                ctx->SetErrorMessage("");
            }
            else
            {
                std::string error = "Owned by user id ";
                error.append(std::to_string(userID)).append(", user id ");
                error.append(std::to_string(msg.GetUserID())).append(" does not have permission to "
                    "access API;");

                ctx->AppendErrorMessage(error);
            }
        }
        else
        {
            ctx->SetErrorCode(CSMERR_DB_ERROR);
            ctx->AppendErrorMessage("Database check failed for user id " + std::to_string(msg.GetUserID()) + ";" );
        }
	    
        LOG( csmapi, trace ) << ":CompareDataForPrivateCheck: Exit";

	    return success;
    }

};

#endif

