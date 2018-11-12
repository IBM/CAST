/*================================================================================

    csmi/include/csm_api_workload_manager.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_API_WORKLOAD_MANAGER_H__
#define __CSM_API_WORKLOAD_MANAGER_H__

#include <stdint.h>
#include "csmi_type_wm_funct.h"

#ifdef __cplusplus
extern "C" {    
#endif

/** @file csm_api_workload_manager.h
 *  @brief Function prototypes and structures for @ref wm_apis
 * 
 *  This contains the prototypes and struct definitions for the CSM Workload Manager
 *  APIs and any macros, constants, or global variables you will need.
 *
 *  @author John Dunham (jdunham@us.ibm.com)
 *  @author Jon Cohn (jcohn@us.ibm.com)
 *	@author Nick Buonarota (nbuonar@us.ibm.com)
 */

/** 
 * @ingroup wm_apis
 * @brief Used by the workload manager to allocate nodes for a job.
 *
 * This API performs actions on the Master and Compute Daemons.
 *
 * The allocation requires at least a job id and a list of nodes in the @p allocation struct.
 *
 * ## Master Daemon ##
 *
 * ### Critical Path ###
 * The Master Daemon is expected to do the following steps (in order):
 *  -# Insert a record for the API into the *csm_allocation* table.
 *  -# Insert the nodes into the *csm_node* table (failing if the node is not available).
 *  -# Issue a multicast to the Compute Agents specified.
 *  -# Record the starting resource usage of the Compute Agents.
 *
 * ### Failure Path ###
 * In the event that any of the nodes fail to create the allocation the following will happen:
 *  -# Revert the changes to the database and write the failure to *csm_allocation_history*.
 *  -# Execute a multicast to undo any changes on the local nodes.
 *
 * ## Compute Daemon ##
 *
 * ### Critical Path ##
 * A successful API will perform the following steps on the Compute Agents (in order):
 *  -# Log that the allocation is beginning (for BDS analytics).
 *  -# Collect the initial node resource usage.
 *  -# Create a cgroups on the compute nodes specified (if flagged).
 *  -# Register the allocation with the compute daemon.
 *  -# Export the `allocation_id`, `primary_job_id`, `secondary_job_id` and `user_name`.
 *  -# Execute the prolog (using user and system flags supplied).
 *  -# Collect any data that the user could change in the prolog (psr, power cap).
 *
 * ### Failure Path ###
 * If **ANY** nodes fail to complete the above the following is performed on the remaining nodes:
 *  -# Log that the allocation is concluding (for BDS analytics).
 *  -# Clear the cgroups.
 *  -# Execute the epilog.
 *
 * Changes to @p allocation 
 * ------------------------
 * When an allocation create is executed the following fields are populated in @p allocation:
 *          Field | Description
 *     -----------|-------------
 *  allocation_id | CSM assigned value
 *
 *
 *  ## Database Tables ##
 *                Table | Description 
 * ---------------------|--------------------------------
 *       csm_allocation | Tracks the overall allocation. 
 *  csm_allocation_node | tracks the allocation on a node.
 *
 * @since Beta 2.2 Added CGroup core isolation, control logic now the same as delete and update.
 *
 * @param[out] handle     An output pointer containing internally managed api data, destroy with 
 *                          @ref csm_api_object_destroy.
 * @param[in]  input      Specifies the parameters of the allocation being created, 
 *                          the `allocation_id` field is populated with the new allocation id
 *                          when successfully created.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below.
 *
 *
 * Error Code                   | Description
 * -----------------------------|------------------------------------------------------------
 * @ref CSMI_SUCCESS            | Successfully created the Allocation.
 * @ref CSMERR_TIMEOUT          | Multicast timed out.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 * @ref CSMERR_PAYLOAD_EMPTY    | The payload received by the agent handler was empty.
 * @ref CSMERR_MULTI_GEN_ERROR  | Generating the multicast failed.
 * @ref CSMERR_BAD_ADDR_TYPE    | Occurs if API was invoked on a compute daemon.
 * @ref CSMERR_SCRIPT_FAILURE   | A prolog script failed to execute on a compute daemon.
 * @ref CSMERR_CGROUP_FAIL      | The core isolation cgroup could not be created.
 * @ref CSMERR_CGROUP_EINVAL    | The cgroup parameter received an invalid value.
 * @ref CSMERR_CGROUP_EACCES    | The cgroup parameter received an illegal resource value.
 */
int csm_allocation_create(csm_api_object **handle, csm_allocation_create_input_t *input);

/** @ingroup wm_apis
 *  @brief Used by the workload manager to delete an existing allocation.
 * 
 * This API performs actions on the Master and Compute Daemons.
 *
 * ## Master Daemon ##
 *
 *  ### Critical Path ###
 *  The Master Daemon will perform the following steps in the event of a successful delete:
 *   -# Verify the supplied allocation id is that of an active job.
 *   -# Perform a multicast to clean up the nodes participating in the allocation.
 *   -# Aggregate the resource usage and add it to the allocation history.
 *   -# Remove the active allocation from the *csm_allocation* and *csm_allocation_node* tables.
 *
 *  ### Failure Path ###
 *  In the event of a failure the Master Daemon will attempt the following:
 *   -# Flag any nodes that could not be accessed (if it failed in the multicast).
 *   -# Remove the allocation from the active table and free any "good nodes" for later.
 *
 * ## Compute Daemon ##
 *
 *  ### Critical Path ##
 *  The Compute Agent Daemon will execute the following locally in order:
 *   -# Log that the allocation is concluding (for BDS analytics).
 *   -# Export the `allocation_id`, `primary_job_id`, `secondary_job_id` and `user_name`.
 *   -# Execute the epilog using the user and system flags stored in the database.
 *   -# Deregister the allocation with the compute daemon.
 *   -# Remove the cgroups for the allocation being deleted.
 *   -# Aggregate the node resources for processing and computation.
 *
 *  ### Failure Path ###
 *  If the multicast fails the API Handler does not return to the Compute Agent.
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *          csm_allocation | Removes the active allocation.
 *  csm_allocation_history | Moves the active allocation into.
 *                csm_step | Removes any active steps from the allocation
 *        csm_step_history | Moves any active steps into.
 *     csm_allocation_node | Removes the active allocation nodes.
 *
 * @since Beta 2.2 Added CGroup core isolation, control logic now the same as create and update.
 * @since PRPQ: Input struct is now used in lieu of plain int.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with
 *                       @ref csm_api_object_destroy.
 * @param[in]  input  An input struct which specifies an allocation to be deleted.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully deleted the Allocation.
 * @ref CSMERR_TIMEOUT          | Multicast timed out.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 * @ref CSMERR_PAYLOAD_EMPTY    | The payload received by the agent handler was empty.
 * @ref CSMERR_MULTI_GEN_ERROR  | Generating the multicast failed.
 * @ref CSMERR_BAD_ADDR_TYPE    | Occurs if API was invoked on a compute daemon.
 * @ref CSMERR_SCRIPT_FAILURE   | An epilog script failed to execute on a compute daemon.
 * @ref CSMERR_CGROUP_FAIL      | The core isolation cgroup could not be modified.
 * @ref CSMERR_CGROUP_EINVAL    | The cgroup parameter received an invalid value.
 * @ref CSMERR_CGROUP_EACCES    | The cgroup parameter received an illegal resource value.
 */
int csm_allocation_delete(csm_api_object **handle, csm_allocation_delete_input_t *input);


/** @ingroup wm_apis
 * @brief Used by the workload manager or a user script to get information about a specific 
 *      allocation based off a supplied id.
 *
 * This api queries the CSM database for the allocation specified through either the 
 * @p allocation_id or combination of @p primary_job_id and @p secondary_job_id 
 * (@p allocation_id is greedy and will supercede the other fields).
 *
 * If the user is using @p primary_job_id it must be non-zero, @p secondary_job_id 
 * may be zero if the allocation had no secondary job id or greater than zero, and 
 * the @p allocation should be set to 0 to prevent it from taking priority.
 *
 * The results of the query are placed in @p allocation_info, which must be
 * freed by the user through use of the @ref csm_api_object_destroy function.
 *
 * This query polls both the *csm_allocation* and *csm_allocation_history* tables in the 
 * database for the specified allocation. If the allocation is currently active the result 
 * will come from the *csm_allocation* table and the @ref csmi_allocation_history_t
 * field in the @p allocation_info struct will be a null pointer. 
 *
 * If the allocation was inactive and found in *csm_allocation_history* the 
 * @ref csmi_allocation_history_t field will be populated.
 *
 * If the allocation could not be found in either table the @p allocation_info field will be 
 * populated with a null pointer.
 *
 *  ## Database Tables ##
 *                        Table | Description 
 *       -----------------------|--------------------------------
 *               csm_allocation | Active allocations.
 *       csm_allocation_history | Inactive allocations.
 *          csm_allocation_node | Active nodes participating in the allocation.
 *  csm_allocation_node_history | Nodes that historically participated in the allocation.
 *
 * @param[out] handle           An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[in]  input            A wrapper for the allocation id or primary and secondary job ids.
 * @param[out] output An output struct to contain the results of the query. Null if the 
 *                                  query failed. Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: Input is now represented by an input struct, and output has been 
 *      wrapped with an output struct to more easily add new features.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the allocation.
 * @ref CSMI_NO_RESULTS         | Allocation was not found, but no error was detected.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_query(csm_api_object **handle,
                         csm_allocation_query_input_t *input,
                         csm_allocation_query_output_t **output);

/** @ingroup wm_apis
 * @brief Used by the workload manager or a user script to get all allocations currently considered 
 *  "active" by csm.
 *
 * This api queries the CSM Database for any currently active allocations. An active Allocation is 
 * any allocation defined by a row in the *csm_allocation* table of the database.
 *
 * The results of the query are placed in @p allocation_info, with the total number of results 
 * recorded to @p data_count. If there are no active allocations in the database  
 * @p allocation_info will be a null pointer and @p data_count will be zero.
 *
 * The user should free @p allocation_info through use of the @ref csm_api_object_destroy function.
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *          csm_allocation | Active allocations.
 *
 * @param[out] handle          An output pointer containing internally managed api data, destroy 
 *                                  with @ref csm_api_object_destroy.
 * @param[out] output          An output struct containing an array in which each entry corresponds 
 *                                  to a row in the *csm_allocation* table. Null in the event of a 
 *                                  failure. Destroy this using @ref csm_api_object_destroy.
 *
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 * 
 * @since PRPQ: output is now wrapped in a single struct.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved any allocations.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_query_active_all(csm_api_object **handle, 
   csm_allocation_query_active_all_input_t *input,
   csm_allocation_query_active_all_output_t **output);

/** @ingroup wm_apis 
 * @brief Used to retrieve detailed information about a specific allocation id.
 *
 * This api queries the CSM database for an allocation specified by @p allocation_id:
 *  -# Perform the query described in @ref csm_allocation_query to get baseline details.
 *  -# Then, assuming the allocation was found, query the *csm_step* and *csm_step_node* tables 
 *      for details regarding the steps. If the first step determined the allocation is not active
 *      the *csm_step_history* and *csm_step_node_history* tables are queried instead.
 *  -# Finally, the *csm_map_tag* table is queried. 
 *
 * 
 * After these queries have been executed the @p input struct will receive the contents 
 * of the first query and the details described in the remaining steps. For details on the 
 * contents of this struct please consult: 
 * @ref csmi_allocation_t and @ref csmi_allocation_details_t.
 *
 * If the @p allocation_id does not correspond to any allocation in the CSM Database, the return
 * pointers will be null and the @ref CSMI_NO_RESULTS error will be returned.
 * 
 * The @p allocation and @p allocation_details structs must be free'd through use of the 
 * @ref csm_api_object_destroy function.
 *
 * @todo implement *csm_map_tag* (Might not be needed, John 9/19/2017)
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *          csm_allocation | Active allocations.
 *  csm_allocation_history | Inactive allocations.
 *                csm_step | The active steps.
 *        csm_step_history | The inactive steps.
 *           csm_step_node | The active nodes (for steps).
 *   csm_step_node_history | Previously active nodes (for steps).
 *             csm_map_tag | @todo (Might not be needed, John 9/19/2017) 
 *     csm_map_tag_history | @todo (Might not be needed, John 9/19/2017)
 *
 * @param[out] handle        An output pointer containing internally managed api data, 
 *                              destroy with @ref csm_api_object_destroy.
 * @param[in]  input         Wraps the allocation id of the requested allocation.
 * @param[out] output        Wraps the output of the query, Null in the event of a failure.
 *                              Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: Input is now represented by an input struct, and output has been 
 *      wrapped with an output struct to more easily add new features.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the allocation details.
 * @ref CSMI_NO_RESULTS         | Allocation was not found, but no error was detected.
 * @ref CSMERR_INVALID_PARAM    | The allocation id was an invalid value.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_query_details(csm_api_object **handle, 
                                 csm_allocation_query_details_input_t   *input,
                                 csm_allocation_query_details_output_t **output);


/** @ingroup wm_apis
 * @brief Retrieves a collection of resources associated with the supplied allocation.
 *
 * @param[out] handle        An output pointer containing internally managed api data, 
 *                              destroy with @ref csm_api_object_destroy.
 * @param[in]  input        A configuration struct containing the allocation id (mandatory).
 * @param[out] output       An array of output structs containing the results from the SQL query.
 *                            Each entry represents a row from the database, null if none found.
 *                            Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the allocation details.
 * @ref CSMI_NO_RESULTS         | Allocation was not found, but no error was detected.
 * @ref CSMERR_INVALID_PARAM    | The allocation id was an invalid value.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_resources_query(csm_api_object **handle, 
                                            csm_allocation_resources_query_input_t * input,
                                            csm_allocation_resources_query_output_t ** output);

/** @ingroup wm_apis
 * @brief Updates an entry in the `csm_allocation_history` table as specified in @p input.
 *
 * @param[out] handle        An output pointer containing internally managed api data, 
 *                              destroy with @ref csm_api_object_destroy.
 * @param[in]  input        A configuration struct containing the allocation id ( mandatory )
 *                              and a collection of attributes to update ( one required ).
 *
 *  ## Database Tables ##
 *                  Table | Description 
 *             -----------|-------------
 * csm_allocation_history | The history table updated by this API.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the allocation details.
 * @ref CSMI_NO_RESULTS         | Allocation was not found, but no error was detected.
 * @ref CSMERR_INVALID_PARAM    | The allocation id was an invalid value.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_update_history( csm_api_object **csm_obj, 
                                    csm_allocation_update_history_input_t *input);


/** @ingroup wm_apis
 * @brief Creates a new step for an existing Allocation.
 *
 * This API performs actions on the Master and Compute Daemon.
 * @note The Compute Daemon interaction is currently not implemented.
 * 
 * For details on the settings of an allocation step please refer to 
 * @ref csm_allocation_step_begin_input_t.
 *
 *
 * ## Master Daemon ##
 *
 * ### Critical Path ###
 * The Master Daemon is expected to perform the following steps (in order):
 *      -# The step will be created in the database, inserting the step into the *csm_step*, and 
 *          *csm_step_node* tables.
 *
 * ### Failure Path ###
 * In the event of a failure the Master Daemon will attempt to do the following:
 *      -# The insertion of the step will be rolled back in the database.
 *
 * ## Compute Daemon ##
 * 
 * ### Critical Path ###
 * The Compute Daemon is expected to perform the following steps (in order):
 *      -# Export the `allocation_id` to the environment.
 *      -# Execute the prolog as a step prolog.
 *
 * ### Failure Path ###
 * In the event of a failed the Compute Daemon will attempt to do the following:
 *      -# Export the `allocation_id` to the environment.
 *      -# Execute the epilog as a step epilog.
 *      
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *                csm_step | The new step will be defined here.
 *           csm_step_node | Any nodes participating in the step are recorded here.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  step   A configuration struct containing the details of the step to create.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: Aliased struct @ref csmi_allocation_step_t to 
 *      @ref csm_allocation_step_begin_input_t.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully ended the step.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | The settings for the step may have been invalid, consult error message.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 * @ref CSMERR_TIMEOUT          | Multicast timed out
 * @ref CSMERR_PAYLOAD_EMPTY    | The payload received by the agent handler was empty.
 * @ref CSMERR_MULTI_GEN_ERROR  | Generating the multicast failed.
 * @ref CSMERR_BAD_ADDR_TYPE    | Occurs if API was invoked on a compute daemon.
 * @ref CSMERR_SCRIPT_FAILURE   | An prolog script failed to execute on a compute daemon.
 */
int csm_allocation_step_begin(csm_api_object **handle, csm_allocation_step_begin_input_t *input);

/** @ingroup wm_apis 
 * @brief Stops an active step, removing it from the CSM Database and performing any necessary operations on the Nodes.
 *
 * This API performs actions on the Master and Compute Daemon.
 * @note The Compute Daemon interaction is currently not implemented.
 *
 * Identifying information for the step is specified in @p step_history. Consult 
 * @ref csmi_allocation_step_history_t for values that may be inserted in the *csm_step_history* 
 * table when ending the step.
 *
 * ## Master Daemon ##
 *
 * ### Critical Path ###
 * The Master Daemon is expected to perform the following steps (in order):
 *      -# Remove the records of the step from the *csm_step* and *csm_step_node* tables. This
 *          process will save the records from *csm_step* to *csm_step_history*
 *  @todo Should this move the *step_node* to *csm_step_node_history*?
 *
 * ### Failure Path ###
 * In the event of a failure the Master Daemon will attempt to do the following:
 *      -# Exit the execution returning an illustrative error.
 *
 * ## Compute Daemon ##
 * 
 * ### Critical Path ###
 * The Compute Daemon is expected to perform the following steps (in order):
 *      -# Export the `allocation_id` to the environment.
 *      -# Execute the epilog as a step epilog.
 *
 * ### Failure Path ###
 * In the event of a failure the Compute Daemon will attempt to do the following:
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *                csm_step | The record for the step is removed from this table.
 *        csm_step_history | The record removed from *csm_step* is saved here.
 *           csm_step_node | Any records containing the step id are removed.
 *
 * @param[out] handle  An output pointer containing internally managed api data, destroy 
 *                         with @ref csm_api_object_destroy.
 * @param[in]  input   A struct containing the allocation id and step id matching the step to
 *                          be deleted. The additional fields (nested struct)  will be inserted in 
 *                          *csm_step_history* when the remove operation is executed.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: Wrapped struct @ref csmi_allocation_step_history_t with
 *      @ref csm_allocation_step_end_input_t.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully ended the step.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | The settings for the step may have been invalid, consult error message.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 * @ref CSMERR_TIMEOUT          | Multicast timed out
 * @ref CSMERR_PAYLOAD_EMPTY    | The payload received by the agent handler was empty.
 * @ref CSMERR_MULTI_GEN_ERROR  | Generating the multicast failed.
 * @ref CSMERR_BAD_ADDR_TYPE    | Occurs if API was invoked on a compute daemon.
 * @ref CSMERR_SCRIPT_FAILURE   | An epilog script failed to execute on a compute daemon.
 */
int csm_allocation_step_end(csm_api_object **handle, 
                            csm_allocation_step_end_input_t* step_history);

/** @ingroup wm_apis
 * @brief Used by a user managed allocation to get basic information about a step.
 *
 * This api queries the CSM database for the steps matching the supplied conditions. The
 * @ref csm_allocation_step_query_input_t.allocation_id of the @p input struct must be set by the user.
 * 
 * The @ref csm_allocation_step_query_input_t.step_id may be set to the id of a step to query for a
 * single step, or be set to **0** to retrieve all steps associated with the allocation. 
 *
 * The results of this query are placed in @p output, which must be free'd by the user through use 
 * of the @ref csm_api_object_destroy function.
 *
 * This query polls both the *csm_step* and *csm_step_history* tables in the database using the 
 * criteria described above. If a step is active its details will be retrieved from the *csm_step* 
 * table, and the history field of the @p output struct will be nulled. 
 *
 * If a step is inactive the results will be retrieved from the *csm_step_history* table and the 
 * history field in @p output will be populated.
 *
 * If no steps could be found in either table the @p output field will be populated with a null 
 * pointer.
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *                csm_step | Contains active step records.
 *        csm_step_history | Contains inactive step records.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing the allocation id (mandatory) and 
 *                      step id (optional).
 * @param[out] output An output struct containing an array of  @ref csmi_allocation_step_t.
 *                        If the step is in the history table the 
 *                        @ref csmi_allocation_step_t.history field will be populated.
 *                        Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: @ref csm_allocation_step_query_output_t now modularized to use
 *      @ref csmi_allocation_step_t which has a nullable reference to 
 *      @ref csmi_allocation_step_history_t. 
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the steps.
 * @ref CSMI_NO_RESULTS         | No steps found, but no error was detected.
 * @ref CSMERR_INVALID_PARAM    | The allocation id was an invalid value ( <= 0 ).
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_step_query(csm_api_object **handle, 
                              csm_allocation_step_query_input_t* input, 
                              csm_allocation_step_query_output_t **output);

/** @ingroup wm_apis
 * @brief Used by a user managed allocation to get basic information about active steps for an allocation..
 *
 * This api queries the CSM database for any active steps for the supplied allocation id. An active
 * step is any step defined in *csm_step* table of the database.
 *
 * The results of the query are placed in @p input, with the total number of results 
 * recorded to @p data_count. If there are no active steps in the database, for the allocation,
 * @p output will be a null pointer and @p data_count will be zero. The @p output struct must be 
 * free'd through use of the @ref csm_api_object_destroy function.
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *                csm_step | Contains active step records.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                        @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing the allocation id to find steps for.
 * @param[out] output An output struct containing an array of  @ref csmi_allocation_step_t.
 *                        Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: @ref csm_allocation_step_query_active_all_output_t now modularized to use
 *      @ref csmi_allocation_step_t which has a nullable reference to 
 *      @ref csmi_allocation_step_history_t. 
 *
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the steps.
 * @ref CSMI_NO_RESULTS         | No steps found, but no error was detected.
 * @ref CSMERR_INVALID_PARAM    | The allocation id was an invalid value ( <= 0 ).
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_step_query_active_all(csm_api_object **handle, 
                                         csm_allocation_step_query_active_all_input_t* input, 
                                         csm_allocation_step_query_active_all_output_t** output);

/** @ingroup wm_apis
 * @brief Used by a user managed allocation to get detailed information about a step.
 *
 * This api queries the CSM database for the steps matching the supplied conditions. The
 * @ref csm_allocation_step_query_details_input_t.allocation_id of the @p input struct must be set by 
 * the user.
 * 
 * The @ref csm_allocation_step_query_details_input_t.step_id may be set to the id of a step to query 
 * for a single step, or be set to **0** to retrieve all steps associated with the allocation. 
 *
 * @note This API, by default is privileged, only the owner of the allocation or a privileged user
 *  may retrieve detailed data about a step!
 *
 * This api queries the CSM database for detailed information about the steps associated 
 *  -# Verify that the user is allowed to access detailed information about the step(s).
 *  -# Then, a query against the *csm_step*, *csm_step_node*, *csm_step_history* and 
 *      *csm_step_node_history* tables is executed, gathering detailed information about the step(s).
 
 * After the query is completed @p output will be populated by the results of the query. 
 * The user must free @p output through use of the @ref csm_api_object_destroy function.
 *
 * In the event of no steps being found in the CSM Database, the @p output struct will be nulled.
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *                csm_step | Contains active step records.
 *           csm_step_node | Contains active step node records.
 *        csm_step_history | Contains inactive step records.
 *   csm_step_node_history | Contains inactive step node records.
 *
 * @param[out] handle       An output pointer containing internally managed api data, destroy with 
 *                            @ref csm_api_object_destroy.
 * @param[in]  input        A configuration struct containing the allocation id (mandatory) and 
 *                            step id (optional).
 * @param[out] output An output struct containing an array of  @ref csmi_allocation_step_t.
 *                        If the step is in the history table the 
 *                        @ref csmi_allocation_step_t.history field will be populated.
 *                        Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: @ref csm_allocation_step_query_details_output_t now modularized to use
 *      @ref csmi_allocation_step_t which has a nullable reference to 
 *      @ref csmi_allocation_step_history_t. 
 *
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the step details.
 * @ref CSMI_NO_RESULTS         | No steps found, but no error was detected.
 * @ref CSMERR_INVALID_PARAM    | The allocation id was an invalid value ( <= 0 ).
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_allocation_step_query_details(csm_api_object **handle, 
                                      csm_allocation_step_query_details_input_t* input, 
                                      csm_allocation_step_query_details_output_t **output);

/** @ingroup wm_apis
 * @brief Used by to modify the state field of an existing allocation.
 *
 * This is used to modify the state of an allocation to the value supplied in @p new_state. This
 * API **DOES NOT** support changing the state to @ref STAGING_IN.
 * 
 * See @ref csm_allocation_create for behavior when changing the state to @ref RUNNING and
 * @ref csm_allocation_delete for behavior when changing the state to @ref STAGING_OUT.
 *
 * @note This api will **NOT** create a new allocation in the database, just update it to @ref RUNNING!
 * @note This api will **NOT** delete an allocation from the database, just update it to @ref STAGING_OUT!
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *          csm_allocation | Active allocations.
 *
 * @since Beta 2.2 Added multicast support (Staging In -> Running, Running -> Staging Out)
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy 
 *                       with @ref csm_api_object_destroy.
 * @param[in]  input  A wrapper struct containing the allocation id to update and
 *                       the new state. For more details consult 
 *                       @ref csm_allocation_update_state_input_t 
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: Wrapped the input to @ref csm_allocation_update_state_input_t
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully updated the allocation.
 * @ref CSMERR_INVALID_PARAM    | The allocation id was an invalid value ( <= 0 ), or an illegal state was specified.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 * @ref CSMERR_PAYLOAD_EMPTY    | The payload received by the agent handler was empty.
 * @ref CSMERR_MULTI_GEN_ERROR  | Generating the multicast failed.
 * @ref CSMERR_BAD_ADDR_TYPE    | Occurs if API was invoked on a compute daemon.
 * @ref CSMERR_SCRIPT_FAILURE   | An epilog script failed to execute on a compute daemon.
 * @ref CSMERR_CGROUP_FAIL      | The core isolation cgroup could not be modified.
 * @ref CSMERR_CGROUP_EINVAL    | The cgroup parameter received an invalid value.
 * @ref CSMERR_CGROUP_EACCES    | The cgroup parameter received an illegal resource value.
 */
int csm_allocation_update_state(csm_api_object **handle, 
                                csm_allocation_update_state_input_t *input);      

/**
 * @ingroup wm_apis
 * 
 * @brief Used by the workload manager to get allocation relevant information on a list of nodes in order to help determine which nodes to allocate for future jobs.
 * 
 * This api queries the CSM database for job scheduler relevant information.
 * The API refines its database search through criteria in the @p input parameter. 
 * Mostly commonly used is the csm_node_resources_query_input_t#node_names field of @p input.
 * The csm_node_resources_query_output_t#results field of @ref output will be populated with the records retrieved from the SQL query.
 * The csm_node_resources_query_output_t#results_count field of @p output will contain the number of records returned from the SQL query.
 * @p output must be freed by the user through use of the @ref csm_api_object_destroy function.
 * 
 * In the event of an error @p output will be set to null.
 * 
 * The results field of @p output represent the node data most relevant to the scheduler.
 * 
 * ## Database Tables ##
 * Table    | Description 
 * ---------|------------------------------------------
 * csm_node | logical extension of the xcat node table.
 * csm_ssd  | contains information on the ssds known to the system.
 * 
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                    @ref csm_api_object_destroy.
 * @param[in]  input  A struct containing the query parameters: a list of node resources.
 * @param[out] output A collection of records corresponding to rows in the query described above.
 *                    See @ref csm_node_resources_query_output_t for details. 
 *                    Destroy this using @ref csm_api_object_destroy.
 * 
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-----------------------------------------------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the node resources.
 * @ref CSMERR_INVALID_PARAM    | One of the parameters in the @p input was invalid.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
*/
int csm_node_resources_query(csm_api_object **handle, 
                             csm_node_resources_query_input_t* input, 
                             csm_node_resources_query_output_t** output);

/** @ingroup wm_apis
 * @brief Returns an array of the resources of all schedulable nodes.
 *
 * This api queries the CSM database for all schedulable nodes of type *compute* and *launch*.
 * The results of the query are placed in @p output, which must be freed by the user through 
 * use of the @ref csm_api_object_destroy function.
 *
 * In the event of an error @p output will be set to null.
 *
 * The contents of @p output represent the node data most relevant to the scheduler.
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *  -----------------------|--------------------------------
 *                csm_node | Active nodes running CSM Daemons (compute and launch).
 *                 csm_ssd | A record of ssd details for each node.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A struct containing the query parameters: a limit and offset.
 * @param[out] output A collection of records corresponding to rows in the query described above.
 *                    See @ref csm_node_resources_query_all_output_t for details.
 *                    Destroy this using @ref csm_api_object_destroy.
 *
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the node resources.
 * @ref CSMERR_INVALID_PARAM    | One of the parameters in the @p input was invalid.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_node_resources_query_all (csm_api_object **handle, 
                                  csm_node_resources_query_all_input_t* input, 
                                  csm_node_resources_query_all_output_t** output);

/** @ingroup wm_apis 
 * @brief Used by PMIx to create a cgroup for the start of a step.
 *
 *
 * @param[out] handle      An output pointer containing internally managed api data, 
 *                           destroy with @ref csm_api_object_destroy.
 * @param[in]  cgroup_args A configuration structure for a new cgroup, consult the struct 
 *                          documentation for details.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below. 
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the allocation.
 * @ref CSMERR_CGROUP_FAIL      | Th API was unable to create the cgroup.
 * @ref CSMERR_CGROUP_EINVAL    | The cgroup parameter received an invalid value.
 * @ref CSMERR_CGROUP_EACCES    | The cgroup parameter received an illegal resource value.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack payload.
 */
int csm_allocation_step_cgroup_create(
    csm_api_object **handle, 
    csm_allocation_step_cgroup_create_input_t* input);

/** @ingroup wm_apis 
 * @brief Used by PMIx to delete a cgroup for the end of a step.
 *
 * This API deletes a cgroup produced by csm_allocation_step_cgroup_create.
 *
 * @param[out] handle          An output pointer containing internally managed api data, 
 *                              destroy with @ref csm_api_object_destroy.
 * @param[in]  cgroup_args A configuration structure specifying the cgroup to remove/
 *
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below. 
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the allocation.
 * @ref CSMERR_CGROUP_FAIL      | Th API was unable to delete the cgroup.
 * @ref CSMERR_CGROUP_EINVAL    | The cgroup parameter received an invalid value.
 * @ref CSMERR_CGROUP_EACCES    | The cgroup parameter received an illegal resource value.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack payload.
 */
int csm_allocation_step_cgroup_delete(
    csm_api_object **handle, 
    csm_allocation_step_cgroup_delete_input_t* input);

/** @ingroup wm_apis
 * @brief Used to place a user into the appropriate cgroup. 
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  The details about the login, contains pid, username and allocation_id.
 *                    Destroy this using @ref csm_api_object_destroy.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully logged the user in.
 * @ref CSMERR_CGROUP_FAIL      | Generic error, cgroup not found.
 * @ref CSMERR_CGROUP_EINVAL    | The cgroup parameter received an invalid value.
 * @ref CSMERR_CGROUP_EACCES    | The cgroup parameter received an illegal resource value.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack payload.
 */
int csm_cgroup_login( csm_api_object **handle, csm_cgroup_login_input_t* input );


/** @ingroup wm_apis
 * @brief Used to start JSM through the CSM infrastructure.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  The details about the login, contains pid, username and allocation_id.
 *                    Destroy this using @ref csm_api_object_destroy.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully logged the user in.
 */
int csm_jsrun_cmd( csm_api_object **handle, csm_jsrun_cmd_input_t* input);

/** @ingroup wm_apis
 *
 * @todo DOCUMENT/FINISH -John
 */
int csm_soft_failure_recovery(csm_api_object **handle);

#ifdef __cplusplus
}
#endif

#endif

