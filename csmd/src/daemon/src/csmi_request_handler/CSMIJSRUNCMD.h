/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIJSRUNCMD.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: John Dunham
* Email: jdunham@us.ibm.com
*/
#ifndef __CSMI_JSRUN_CMD_H__
#define __CSMI_JSRUN_CMD_H__

#include "csmi_stateful_db.h"
#include "csmi_mcast/CSMIMcastJSRUN.h"

enum JSRUNCMDStates
{
    JSRUN_CMD_INIT = 0,
    JSRUN_CMD_MCAST_SPAWN,    // 1
    JSRUN_CMD_MCAST_RESPONSE, // 2
    JSRUN_CMD_FINAL,          // 3
    JSRUN_CMD_TIMEOUT         // 4

};

class CSMIJSRUNCMD_Master : public CSMIStatefulDB
{
public:
    CSMIJSRUNCMD_Master( csm::daemon::HandlerOptions& options );

    /** @brief Kills execution, JSRUNCMD needs to be privileged.
     *
     * STATE INIT.
     */
    virtual bool CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    virtual bool RetrieveDataForPrivateCheck(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;
    
    virtual bool CompareDataForPrivateCheck(
        const std::vector<csm::db::DBTuple *>& tuples,
        const csm::network::Message &msg,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    /** @brief Parses the query defined in RetrieveDataForPrivateCheck.
     * 
     * Used in the following states:
     *  -MCAST_SPAWN
     *
     * @param[in] ctx The context of the handler.
     * @param[in] tuples The tuples retrieved from the database.
     * @param[in,out] mcastProps A container for the results of the query (unused).
     *
     * @return True if the user was authorized to execute on the nodes.
     */
    static bool ParseAuthQuery( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const std::vector<csm::db::DBTuple *>& tuples,
        CSMIJSRUNCMD* mcastProps);

    /**
     * @brief Non functional query, returns nullptr.
     *
     * @return nullptr.
     */
    static inline csm::db::DBReqContent* BadQuery(
        csm::daemon::EventContextHandlerState_sptr ctx,
        CSMIJSRUNCMD* mcastProps) { return nullptr; }

    /**
     * @brief Invokes the @ref CreateByteArray.
     *
     * @return True if the byte array was successfully constructed.
     */
    static bool CreateByteArray(
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx );

    /** 
     * @brief Serializes the results for replying to the API caller.
     *
     * @param[in]  tuples The response from the database (Empty).
     * @param[out] buf The serialized message response.
     * @param[out] bufLen The length of the message buffer.
     * @param[in,out] ctx The context of the handler.
     */
    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;
};

class CSMIJSRUNCMD_Agent : public CSMIStateful
{
public:
    CSMIJSRUNCMD_Agent( csm::daemon::HandlerOptions& options );
};

#endif
