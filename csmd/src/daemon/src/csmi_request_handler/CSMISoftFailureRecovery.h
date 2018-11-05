/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMISoftFailureRecovery.h
    
    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
    
#ifndef __CSMI_SOFT_FAILURE_DELETE__
#define __CSMI_SOFT_FAILURE_DELETE__

#include "csmi_stateful_db.h"
#include "csmi_mcast/CSMIMcastSoftFailureRecovery.h"

/**
 *@brief todo
 */
class CSMISoftFailureRecovery_Master : public CSMIStatefulDB
{
public:
    CSMISoftFailureRecovery_Master(csm::daemon::HandlerOptions& options) ;

    /** @brief Queries the database for the specified allocation id.
     *
     * STATE INIT.
     */
    virtual bool CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    static bool CreateByteArray(
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx );

    /** @brief 
     *
     */
    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    static bool CreateResponsePayload(
        const std::vector<csm::db::DBTuple *>&tuples,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx);

    /** @brief Parses the query defined in CreatePayload.
     * 
     * Used in the following states:
     *  -MCAST_SPAWN
     *
     * @param[in] tuples The tuples retrieved from the database.
     * @param[in] mcastProps A container for the results of 
     *
     * @return True if the info was successfully parsed.
     */
    static bool ParseInfoQuery( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        const std::vector<csm::db::DBTuple *>& tuples,
        CSMIMcastSoftFailureRecovery* mcastProps);

    /** @brief Creates a statement to fix the nodes repaired by the process.
     *
     * @param[in] mcastProps The properties used to create the sql statement.
     *
     * @return The Request content for the SQL query.
     */
    static csm::db::DBReqContent* FixRepairedNodes( 
        csm::daemon::EventContextHandlerState_sptr ctx,
        CSMIMcastSoftFailureRecovery* mcastProps);
};

class CSMISoftFailureRecovery_Agent : public CSMIStateful
{
public:
    CSMISoftFailureRecovery_Agent( csm::daemon::HandlerOptions& options ); 
};

#endif 
