/*================================================================================

    csmi/include/csm_api_diagnostics.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSMAPI_DIAGNOSTICS_H__
#define __CSMAPI_DIAGNOSTICS_H__

#include <stdint.h>
#include "csmi_type_diag_funct.h"

#ifdef __cplusplus
extern "C" {    
#endif

/** @file csm_api_diagnostics.h
 *  @brief Function prototypes and structures for @ref diag_apis
 *
 *  This contains the prototypes and struct definitions for the CSM Diagnostics
 *  APIs and any macros, constants, or global variables you will need.
 *
 *  @author Nick Buonarota (nbuonar@us.ibm.com)
 *  @author John Dunham (jdunham@us.ibm.com)
 */

/** @ingroup diag_apis
 * @brief Used by an application to create an entry in the *csm_diag_results* table of the CSM Database.
 *
 *	Creates an entry in the *csm_diag_results* table of the CSM Database. The contents of this entry 
 *	are defined in @p resultData, for details on the fields please consult @ref csm_diag_result_create_input_t.
 *
 *  ## Database Tables ##
 *                Table | Description 
 *           -----------|-------------
 *     csm_diag_results | Tracks the results of active diagnostic runs.
 *
 * 
 * @param[out] handle     An output pointer containing internally managed api data, destroy with 
 *                          @ref csm_api_object_destroy.
 * @param[in] resultData  A configuration struct, contents are inserted into *csm_diag_results*.
 *                          Please refer to @ref csm_diag_result_create_input_t for more information.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: @ref csm_diag_result_create_input_t aliased to @ref csm_diag_result_create_input_t.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully added the results to *csm_diag_results*.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | The database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_diag_result_create(csm_api_object **handle, 
                            csm_diag_result_create_input_t *resultData);

/** @ingroup diag_apis
 * @brief Used by an application to record that a diagnostic run has started.
 *
 *  Creates an entry in the *csm_diag_run* table of the CSM Database. The contents of this entry
 *  are defined in @p input, for details please on the fields please consult 
 *  @ref csm_diag_run_begin_input_t.
 *
 *  ## Database Tables ##
 *  Table        | Description 
 *  -------------|-------------
 *  csm_diag_run | Tracks active diagnostic runs.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with 
 *                    @ref csm_api_object_destroy.
 * @param[in]  input  A configuration struct, contents are inserted into *csm_diag_run*.
 *                    Please refer to @ref csm_diag_run_begin_input_t for more information.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *       @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully added the results to *csm_diag_run*.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | The database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_diag_run_begin(csm_api_object **handle, csm_diag_run_begin_input_t* input);

/** @ingroup diag_apis
 * @brief Used by an application to record on CSM Database that a diagnostic run finished.
 *
 * Invoked at the end of a diagnostic run. Removes diagnostic run matching the 
 * @ref csm_diag_run_end_input_t.runId supplied in @p runEndData from the *csm_diag_run* table. The
 * removed entry is then moved to *csm_diag_run_end* table and populated with the data specified by
 * the remaining fields in @p runBeginData. 
 *
 * This will also migrate all entries in *csm_diag_result* with a matching 
 * @ref csm_diag_run_end_input_t.runId to the *csm_diag_result_history* table.
 *
 *
 *  ## Database Tables ##
 *                   Table | Description 
 *              -----------|-------------
 *         csm_diag_result | Tracks the results of active diagnostic runs.
 * csm_diag_result_history | Tracks the results of completed diagnostic runs.
 *            csm_diag_run | Tracks active diagnostic runs.
 *        csm_diag_run_end | Tracks completed diagnostic runs.
 *
 * @param[out] handle      An output pointer containing internally managed api data, destroy with 
 *                              @ref csm_api_object_destroy.
 * @param[in]  runEndData  A configuration struct, contents specify the diagnostic to end and 
 *                              any metadata about the end of that diagnostic run.Please refer to 
 *                              @ref csm_diag_run_end_input_t for more information.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @since PRPQ: @ref csm_diag_run_end_input_t aliased to @ref csm_diag_run_end_input_t.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully ended the diagnostic run.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | The database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_diag_run_end(csm_api_object **handle, csm_diag_run_end_input_t* runEndData);

/** @ingroup diag_apis
 * @brief Used by an application to retrieve information about a diagnostics run.
 *
 * Queries both the *csm_diag_run* and *csm_diag_run_history* tables in the CSM Database for any
 * entries matching the parameters supplied in @p input. The results are placed in @p output with
 * each index in the array corresponding to one of the rows in the table the size of this array 
 * is stored in @p output_count. The @p output struct must be destroyed using 
 * @ref csm_api_object_destroy.
 * 
 * In the event of an error @p output will be set to null.
 *
 * If an entry was retrieved from the history table @ref csm_diag_run_query_output_t.end_time will
 * be populated.
 *
 * @todo should this return CSMI_NO_RESULTS ?
 *
 * ## Database Tables ##
 *                  Table | Description 
 *             -----------|-------------
 *           csm_diag_run | Tracks active diagnostic runs.
 *       csm_diag_run_end | Tracks completed diagnostic runs.
 *
 * @param[out] handle  An output pointer containing internally managed api data, destroy with 
 *                         @ref csm_api_object_destroy.
 * @param[in]  input   A configuration struct, contents specify the diagnostic search for.
 *                         Please refer to @ref csm_diag_run_query_input_t for more information.
 * @param[out] output  An struct containing an array of output structs, each index represents an entry in either the
 *                         *csm_diag_run* or *csm_diag_run_end* table. Returns null if no entries
 *                         could be found. For details on the fields consult 
 *                         @ref csm_diag_run_query_output_t.
 *                         Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 *  @since PRPQ: The output struct was converted to be self contained.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the database and got results.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | The database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_diag_run_query(csm_api_object **handle, csm_diag_run_query_input_t* input, 
                       csm_diag_run_query_output_t **);

/** @ingroup diag_apis
 * @brief Used by an application to retrieve information about a diagnostics run and a diagnostics result.
 *
 * This api queries the CSM Database for a diagnostic run specified in @p input:
 *  -# Perform the query described in @ref csm_diag_run_query to get baseline details.
 *  -# Then, assuming the diagnostic was found query the *csm_diag_result* and 
 *      *csm_diag_result_history* tables for the run id specified in @p input.
 * 
 * After the queries have been executed the @p output struct will be populated with the resulting 
 * data. In the event of an error @p output will be null. If no details are found 
 * @ref csm_diag_run_query_details_output_t.num_details will be set to zero and 
 * @ref csm_diag_run_query_details_output_t.details will be set to null. For more details on this
 * struct and its contents please consult @ref csm_diag_run_query_details_output_t.
 *
 * The @p output struct must be freed trough use of the @ref csm_api_object_destroy function.
 * 
 * @todo should this return CSMI_NO_RESULTS ?
 *
 * ## Database Tables ##
 *                  Table | Description 
 *             -----------|-------------
 *           csm_diag_run | Tracks active diagnostic runs.
 *       csm_diag_run_end | Tracks completed diagnostic runs.
 *         csm_diag_result | Tracks the results of active diagnostic runs.
 * csm_diag_result_history | Tracks the results of completed diagnostic runs.
 *
 *
 * @param[out] handle  An output pointer containing internally managed api data, destroy with 
 *                          @ref csm_api_object_destroy.
 * @param[in]  input   A configuration struct, contents specify the diagnostic search for.
 *                          Please refer to @ref csm_diag_run_query_details_input_t for more information.
 * @param[out] output  A struct containing the results of the detail query. Consult 
 *                          @ref csm_diag_run_query_details_output_t for details on the contents.
 *                          Destroy this using @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully queried the database and got results.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | The database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_diag_run_query_details(csm_api_object **handle, 
    csm_diag_run_query_details_input_t *input,
    csm_diag_run_query_details_output_t ** output );

#ifdef __cplusplus
}
#endif

#endif
