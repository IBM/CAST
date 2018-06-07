/*================================================================================
    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationCreate.h
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/**
 * @file CSMIAllocationCreate.h
 *
 * @author John Dunham (jdunham@us.ibm.com)
 */
    
#ifndef __CSMI_ALLOCATION_CREATE__
#define __CSMI_ALLOCATION_CREATE__

#include "csmi_stateful_db.h"
#include "csmi_mcast/CSMIMcastAllocation.h"

/**
 * @brief A stateful handler for Allocation Create on the Master Daemon.
 *
 *
 * 
 */
class CSMIAllocationCreate_Master : public CSMIStatefulDB
{
public:
    CSMIAllocationCreate_Master(csm::daemon::HandlerOptions& options) ;

    /** @brief Queries the database for the specified allocation id.
     * 
     * Invoked from STATEFUL_DB_INIT.
     *
     * @param[in]  arguments A Buffer containing a serialized structure,
     *                          @ref csm_allocation_create_input_t.
     * @param[in]  len       The length of the argument buffer.
     * @param[out] dbPayload A Payload containing the database request.
     * @param[in]  ctx       The context of the handler.
     *
     * @return True if the payload could be created.
     */
    virtual bool CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    /**
     * @brief Generates a payload to reserve the nodes for an allocation.
     *
     * Template parameter to @ref StatefulDBRecvSend.
     * Invoked in state RESERVE_NODES.
     *
     * @param[in]  tuples Should contain the allocation id.
     * @param[out] dbPayload The payload to reserve the nodes in the database.
     * @param[in]  ctx The context of the handler.
     *
     * @return True if @p dbPayload was successfully constructed.
     */
    static bool ReserveNodes(
        const std::vector<csm::db::DBTuple *>&tuples,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx);

    /**
     * @brief Generates a database payload to undo the allocation.
     * Invoked by the following states:
     *   * MCAST_SPAWN - Allocation is not placed in history.
     *   * MCAST_RESPONSE
     *   * UNDO_MCAST_RESPONSE
     *   * UPDATE_STATS
     * 
     * This will invoke fn_csm_allocation_revert in the SQL database.
     * 
     * @param[in] ctx The context of the handler.
     * @param[in] mcastProps The properties of the multicast, 
     *                  has allocation details to revert properly.
     *
     * @return A database request containing a payload to revert the allocation.
     */
    static csm::db::DBReqContent* UndoAllocationDB(
        csm::daemon::EventContextHandlerState_sptr ctx,
        CSMIMcastAllocation* mcastProps);
    
    /**
     * @brief Generates a database payload to insert aggregated statistics.
     * This is the final payload executed by the allocation create.
     * Invoked in the MCAST_RESPONSE state when all of the responses have been received.
     * Template parameter to @ref McastResponder.
     *
     * @param[in] ctx The context of the handler.
     * @param[in] mcastProps The properties of the multicast, 
     *              has the statistics to report on the allocation.
     *
     * @return A database request to insert aggregated statistics for an allocation.
     */
    static csm::db::DBReqContent* InsertStatsStatement(
        csm::daemon::EventContextHandlerState_sptr ctx,
        CSMIMcastAllocation* mcastProps);

    /**
     * @brief The terminal state for a failure to create an allocation.
     * Invoked in the UNDO_INSERT state, forces a failure exit point.
     * Template parameter to @ref StatefulDBRecvTerminal.
     *
     * @param[in]  tuples Should contain the allocation id.
     * @param[out] dbPayload The payload to reserve the nodes in the database.
     * @param[in]  ctx The context of the handler, has the multicast context object 
     *              for error reporting.
     *
     * @return False, triggers the error handler.
     */
    static bool  UndoTerminal(
        const std::vector<csm::db::DBTuple *>& tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx );

    /**
     * @brief The terminal state for the handler.
     * This is the STATEFUL_DB_RECV_DB state of @ref CSMIStatefulDB.
     * Invokes @ref CSMIAllocationCreate_Master.CreateByteArray
     *
     * @param[in]  tuples The database tuples retrieved from the database query,
     *              likely null.
     * @param[out] buf    A serialized message to send to the customer.
     * @param[out] bufLen The length of the generated buffer.
     * @param[in]  ctx    The context of the handler, error details and private check s    tatus.
     * @return True if the allocation was created successfully.
     */
    virtual bool CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;

    /**
     * @brief A success case terminal function.
     * Template parameter to @ref McastSpawner and @ref McastTerminal.
     * Invoked when the allocation create does not need to perform a multicast for
     * a valid execution.
     *
     * This implementation deliberately doesn't take a tuples object, as it is not
     * expected to required a database response to function.
     *
     * @param[out] buf    A serialized message to send to the customer.
     * @param[out] bufLen The length of @p buf.
     * @param[in] ctx
     *
     * @return True if the Handler executed successfully.
     */
    static bool CreateByteArray(
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx );


    /**
     * @brief Determines if the handler should perform a multicast.
     *
     * This is inlined due to the simplicity of the function.
     *
     * @param[in] ctx Unused.
     * @param[in] tuples Unused.
     * @param[in] mcastProps Tracks the state and type of the allocation.
     *
     * @param Return false if the multicast context was lost,
     *  the allocation is diagnostic, or the allocation is not running.
     *
     */
    static inline bool PerformMulticast(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const std::vector<csm::db::DBTuple *>& tuples,
        CSMIMcastAllocation* mcastProps) 
    {
        bool success = true;
        // Typically speaking this should be true.
        // If the state of the allocation was in diagnostics 
        // or not running, then a multicast is not needed.
        if( mcastProps )
        {
            csmi_allocation_mcast_context_t* allocation = mcastProps->GetData();
            // Return true if the allocation exists and the 
            // allocation is in a multicast state.
            success =  allocation && 
                !(allocation->type == CSM_DIAGNOSTICS ||
                  allocation->state != CSM_RUNNING);
            allocation->save_allocation = 1;
        }
        else
            success = false;

        return success;
    }

    /**
     * @brief Pass through function, may change pending changes to the stat query.
     * Catches the output from the @ref InsertStatsStatement query.
     * Returns true no, because the query returns nothing at this time.
     *
     * @return True
     */
    static  bool ParseStatsQuery(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const std::vector<csm::db::DBTuple *>& tuples,
        CSMIMcastAllocation* mcastProps);
};

/**
 * @brief A stateful handler for Allocation Create on the Compute Daemon.
 *
 */
class CSMIAllocationCreate_Agent : public CSMIStateful
{
public:
    CSMIAllocationCreate_Agent( csm::daemon::HandlerOptions& options ); 
};

#endif 
