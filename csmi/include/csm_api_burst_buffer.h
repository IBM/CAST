/*================================================================================

    csmi/include/csm_api_burst_buffer.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_API_BURST_BUFFER_H__
#define __CSM_API_BURST_BUFFER_H__

#include <stdint.h>
#include "csmi_type_bb_funct.h"

#ifdef __cplusplus
extern "C" {    
#endif


/** @file csm_api_burst_buffer.h
 *  @brief Function prototypes and structures for @ref bb_apis
 *
 *  This contains the prototypes and struct definitions for the CSM Burst
 *  Buffer APIs and any macros, constants, or global variables you will need.
 *
 *  @author Nick Buonarota (nbuonar@us.ibm.com)
 *  @author John Dunham (jdunhame@us.ibm.com)
 */

/** @ingroup bb_apis 
 * @brief Used by internal BB function to send a bb cmd from one place to another. For example, launch node to a compute node.
 * 
 * This API Performs actions on the Utility and Compute Daemon.
 *
 * ## Utility Daemon ##
 *
 * ### Critical Path ###
 * The Utility Daemon is expected to do the following steps:
 *   -# Generate a multicast targeting the nodes specified in @p input
 *   -# Aggregate the successes/failures of the command executions on the compute daemons.
 *
 *
 * ## Compute Daemon ##
 *
 * ### Critical Path ###
 * The Compute Daemon is expected to perform the following: 
 *   -# Verify that the command exists in the whitelist, fail the node if not and trigger @ref CSMERR_EXEC_ILLEGAL.
 *   -# Attempt to execute the command, capturing the output.
 *
 * ### Failure Path ###
 * In the event of the Compute Daemon failing to execute the command on the compute daemon:
 *  -# Capture the error returned by the Burst Buffer command (stored in error message).
 *  -# Return to the Utility Daemon and notify it about the failure.
 *
 * @todo Should the whitelist be defined in a config file? Currently defined in *CSMIBBCMD_Compute_Init.cc*.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct defines the command to execute, parameters and nodes.
 * @param[out] output An output containing the json output string, destroy 
 *                      with @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 *
 * @todo Make sure @ref CSMERR_SCRIPT_FAILURE works.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully executed the burst buffer command.
 * @ref CSMERR_TIMEOUT          | Multicast timed out.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 * @ref CSMERR_PAYLOAD_EMPTY    | The payload received by the agent handler was empty.
 * @ref CSMERR_BAD_ADDR_TYPE    | Occurs if API was invoked on a compute daemon.
 * @ref CSMERR_MULTI_GEN_ERROR  | Generating the multicast failed.
 * @ref CSMERR_EXEC_ILLEGAL     | Occurs if the BB command was not in the whitelist.
 * @ref CSMERR_SCRIPT_FAILURE   | Indicates an error with the bb command.
 *
 */
int csm_bb_cmd(csm_api_object **handle, csm_bb_cmd_input_t* input, csm_bb_cmd_output_t** output);

/** @ingroup bb_apis
 * @brief Used by the Burst Buffer to create an entry in the *csm_lv* table of the CSM DB.
 *
 * This api creates a new entry in the CSM Database *csm_lv* table using the contents of the
 * @p input struct. For details on configuration please refer to @ref csm_bb_lv_create_input_t.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *               csm_lv | Tracks the logical volumes.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing the data to insert in the *csm_lv* table.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully created the entry in *csm_lv*.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_bb_lv_create(csm_api_object **handle, csm_bb_lv_create_input_t* input);

/** @brief Used by the BB to delete an entry in the "csm_lv" table of the CSM DB.
 *
 * This api deletes an entry in the CSM Database *csm_lv* table and record it to the 
 * *csm_lv_history* table. The entry deleted is specified by the contents of @p input.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *               csm_lv | Tracks the logical volumes.
 *       csm_lv_history | Tracks the history of the logical volumes.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing identifying information for the *csm_lv* 
 *                      entry.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully created the entry in *csm_lv*.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_bb_lv_delete(csm_api_object **handle, csm_bb_lv_delete_input_t* input);

/** @ingroup bb_apis
 * @brief Used by Burst Buffer or a system administrator to view records in the "csm_lv" table of the CSM DB.
 *
 * This api queries the CSM Database *csm_lv* table for entries matching the parameters specified 
 * in @p input. In the event of the query failing to find entries @p input will be null.
 *
 * The @p input struct should be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *               csm_lv | Tracks the logical volumes.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing a list of logical volume names.
 * @param[out] output An output struct containing a collection of entries retrieved by the query,
 *                      consult @ref csm_bb_lv_query_output_t for specifics. Destroy this using
 *                      @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully created the entry in *csm_lv*.
 * @ref CSMI_NO_RESULTS         | The logical volume could not be found, no other errors found.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_bb_lv_query(csm_api_object **handle, 
                    csm_bb_lv_query_input_t* input, 
                    csm_bb_lv_query_output_t** output);
				
/** @ingroup bb_apis 
 * @brief Used by the BB to modify an entry in the "csm_lv" table of the CSM DB.
 *
 * This api modifies an existing entry in the CSM Database *csm_lv* table using the contents of the
 * @p input struct. For details on configuration please refer to @ref csm_bb_lv_update_input_t.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *               csm_lv | Tracks the logical volumes.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing the data to modify the *csm_lv* table.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully created the entry in *csm_lv*.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_bb_lv_update(csm_api_object **handle, csm_bb_lv_update_input_t* input);

/** @ingroup bb_apis
 * @brief Used by the Burst Buffer to create an entry in the "csm_vg" table of the CSM DB.
 *
 * This api creates a new entry in the CSM Database *csm_vg* table using the contents of the
 * @p input struct. For details on configuration please refer to @ref csm_bb_vg_create_input_t
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *               csm_vg | Tracks the volume groups.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing the data to insert in the *csm_vg* table.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully created the entry in *csm_lv*.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_bb_vg_create(csm_api_object **handle, csm_bb_vg_create_input_t* input);

/** @ingroup bb_apis
 * @brief Used by Burst Buffer or a system administrator to delete records from the "csm_vg" table of the CSM DB.
 *
 * This api deletes the entries in the CSM Database *csm_vg* table and records them to the 
 * *csm_lv_history* table. The entries deleted are specified by the contents of @p input.
 *
 * This api returns the records successfully deleted by the API in the @p output struct. 
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *               csm_vg | Tracks the volume groups.
 *       csm_vg_history | Tracks the history of the volume groups.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing identifying information for the *csm_vg* 
 *                      entries to be deleted.
 * @param[out] output An output struct containing the list of successfully deleted volume groups
 *                      consult @ref csm_bb_vg_delete_output_t for specifics. Destroy this using
 *                      @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully created the entry in *csm_lv*.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_bb_vg_delete(csm_api_object **handle, 
                     csm_bb_vg_delete_input_t* input, 
                     csm_bb_vg_delete_output_t** output);

/** @ingroup bb_apis 
 * @brief Used by Burst Buffer or a system administrator to view records in the "csm_vg" table of the CSM DB.
 *
 * This api queries the CSM Database *csm_vg* table for entries matching the parameters specified 
 * in @p input. In the event of the query failing to find entries @p input will be null.
 *
 * The @p input struct should be destroyed using @ref csm_api_object_destroy.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *               csm_vg | Tracks the volume groups.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct containing a list of volume group names.
 * @param[out] output An output struct containing a collection of entries retrieved by the query,
 *                      consult @ref csm_bb_vg_query_output_t for specifics. Destroy this using
 *                      @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with 
 *      @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully created the entry in *csm_vg*.
 * @ref CSMI_NO_RESULTS         | The volume group could not be found, no other errors found.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 *
 */
int csm_bb_vg_query(csm_api_object **handle, 
                    csm_bb_vg_query_input_t* input, 
                    csm_bb_vg_query_output_t** output);

#ifdef __cplusplus
}
#endif

#endif
