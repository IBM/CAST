/*================================================================================
   
    csmi/include/csmi_type_diag.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_diag.h
 * @brief A collection of structs for @ref diag_apis.
 */
#ifndef _CSMI_DIAG_TYPES_H_
#define _CSMI_DIAG_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

// Begin special_preprocess directives
// Used for diag_run_query
// Include the database constants for the CSM_STATUS_MAX.
#include "csm_api_common.h"

/** @addtogroup diag_apis
 * @{
 */

// End special_preprocess directives

/** defgroup csmi_diag_run_status_t csmi_diag_run_status_t
 * @{
 */
/**
  * @brief A listing of legal flags for the diagnostic run status.
 * This enumeration supports bitwise operations.
  */
typedef enum {
   DIAG_NONE=0, ///< 0 - No flags set
   DIAG_CANCELED=1, ///< 1 - The diagnostic status is 'CANCELED'.
   DIAG_COMPLETED=2, ///< 2 - The diagnostic status is 'COMPLETED'.
   DIAG_RUNNING=4, ///< 4 - The diagnostic status is 'RUNNING'.
   DIAG_FAILED=8, ///< 8 - The diagnostic status is 'FAILED'.
   DIAG_COMPLETED_FAIL=16, ///< 16 - The diagnostic status is 'COMPLETED_FAIL'.
   DIAG_ALL=31, ///< 31 - All flags set
   csmi_diag_run_status_t_MAX=32 ///< 32 - Bounding Value
} csmi_diag_run_status_t;

#define csmi_diag_run_status_t_bit_count 5

/**
 * @brief Maps enum csmi_diag_run_status_t value to string.
 */
extern const char* csmi_diag_run_status_t_strs [];
/** @}*/


/**
 * @brief Defines a diagnostic run to query in the *csm_diag_run* and *csm_diag_run_history* tables of the CSM Database.
 */
typedef struct csmi_diag_run_t csmi_diag_run_t;
struct csmi_diag_run_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t run_id; /**< Diagnostic run id. */                      
    int64_t allocation_id; /**< Unique identifier of the  allocation that this diagnostic is being run under. */
    char* cmd_line; /**< Diagnostic command line invocation: program and arguments. */
    csm_bool inserted_ras; /**< Inserted diagnostic ras events.  */
    char diag_status[16]; /**< Diagnostic status - RUNNING, COMPLETED, CANCELED, FAILED.  @todo Post-PRPQ: enum?*/ 
    char* begin_time; /**< The start time of the diagnostic run.*/
    char* end_time; /**< The end time of the diagnostic run.  @todo Post-PRPQ: overlap with @ref history_time?*/
    char* history_time; /**< Time this entry was inserted into the history table. @todo Post-PRPQ: overlap with @ref end_time?*/      
    char* log_dir; /**< Location of diagnostic log directory. */                            
};
/**
 * @brief Defines a structure to match an entry in *csm_diag_result_history* table of the CSM Database.
 *
 * @todo Not sure if this brief is 100% correct - John Dunham(jdunham@us.ibm.com)
 */
typedef struct csmi_diag_run_query_details_result_t csmi_diag_run_query_details_result_t;
struct csmi_diag_run_query_details_result_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t run_id; /**< Diagnostic run id. */
    char status[16]; /**< Hardware status after the diagnostic finishes (unknown, failed, marginal, success). @todo Post-PRPQ: Is this correct?*/
    char* history_time; /**< The time that the Diagnostic run was placed in the history table. */
    char* test_name; /**< The name of the diagnostic test case run. */
    char* node_name; /**< The node the test case was run on.*/
    char* serial_number; /**< Serial number of the field replaceable unit (fru) that this diagnostic was run against. */
    char* begin_time; /**< The time when the task began (YYYY-MM-DD HH-MM-SS.MMMMM).*/ 
    char* end_time; /**< The time when the task ended (YYYY-MM-DD HH-MM-SS.MMMMM).*/ 
    char* log_file; /**< Location of diagnostic log file. */ 
};
/**
 * @brief An input wrapper for @ref csm_diag_run_end.
 * A configuration struct used to end a Diagnostic Run in the *csm_diag_run* table of the CSM database.
 */
typedef struct csm_diag_run_end_input_t csm_diag_run_end_input_t;
struct csm_diag_run_end_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t run_id; /**< Diagnostic run id. */
    char status[16]; /**< Diagnostic status (RUNNING,COMPLETED,CANCELED, COMPLETED_FAIL, FAILED). */
    csm_bool inserted_ras; /**< Inserted diagnostic ras events.  */
};
/**
 * @brief An input wrapper for @ref csm_diag_result_create.
 *
 * Defines a structure to match an entry in the *csm_diag_result* table of the CSM Database.
 */
typedef struct csm_diag_result_create_input_t csm_diag_result_create_input_t;
struct csm_diag_result_create_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t run_id; /**< Diagnostic run id. */
    char status[16]; /**< Hardware status after the diagnostic finishes (PASS, FAIL). @todo Post-PRPQ: Remove fixed width. */
    char* test_name; /**< The name of the diagnostic test case run. */
    char* node_name; /**< The node the test case was run on. */
    char* serial_number; /**< Serial number of the field replaceable unit (fru) that this diagnostic was run against. */
    char* begin_time; /**< The time when the task began (YYYY-MM-DD HH-MM-SS.MMMMM).*/
    char* log_file; /**< Location of diagnostic log file. */
};
/**
 * @brief An input wrapper for @ref csm_diag_run_begin.
 *
 * A configuration struct used to insert a Diagnostic Run into the *csm_diag_run* table of the CSM database.
 */
typedef struct csm_diag_run_begin_input_t csm_diag_run_begin_input_t;
struct csm_diag_run_begin_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t run_id; /**< Diagnostic run id. */
    int64_t allocation_id; /**< Unique identifier of the  allocation that this diagnostic is being run under. */
    char* cmd_line; /**< How diagnostic program was invoked: program and arguments. */
    char* log_dir; /**< Location of diagnostic log files. */
};
/**
 * @brief An input wrapper for @ref csm_diag_run_query.
 *
 * Specifies parameters used to query the *csm_diag_run* and *csm_diag_run_history* tables of the CSM Database.
 */
typedef struct csm_diag_run_query_input_t csm_diag_run_query_input_t;
struct csm_diag_run_query_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint8_t status; /**< Filter results to only include records that have a matching diagnostic status (NONE=0,CANCELED=1,COMPLETED=2,FAILED=4,RUNNING=8,ALL=15) */
    uint32_t allocation_ids_count; /**< Number of allocations to perform query on, size of @ref allocation_ids.*/
    uint32_t run_ids_count; /**< Number of run ids to perform query on, size of @ref run_ids.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    csm_bool inserted_ras; /**<Filter results to only include records that inserted RAS events or only include records that did not insert RAS events. API doesn't query on @ref CSM_UNDEF_BOOL. */
    char* begin_time_search_begin; /**< A time used to filter results of the SQL query and only include records with a begin_time at or after (ie: '>=' ) this time. */
    char* begin_time_search_end; /**< A time used to filter results of the SQL query and only include records with a begin_time at or before (ie: '<=' ) this time. */
    char* end_time_search_begin; /**< A time used to filter results of the SQL query and only include records with an end_time at or after (ie: '>=' ) this time. */
    char* end_time_search_end; /**< A time used to filter results of the SQL query and only include records with an end_time at or before (ie: '<=' ) this time. */
    int64_t* allocation_ids; /**< Pointer to an array of int64_t allocation_ids. Filter results to only include records that have a matching allocation id. API will ignore values less than 0. Size defined in @ref allocation_ids_count. */
    int64_t* run_ids; /**< Pointer to an array of int64_t run_ids. Filter results to only include records that have a matching diagnostic run id. API will ignore values less than 1. Size defined in @ref run_ids_count. */
};
/**
 * @brief A wrapper for the output of @ref csm_diag_run_query.
 *
 * These fields correspond to matching fields in the *csm_diag_run* and *csm_diag_run_history* 
 * tables of the CSM Database.
 */
typedef struct csm_diag_run_query_output_t csm_diag_run_query_output_t;
struct csm_diag_run_query_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t num_runs; /**< Number of diagnostic runs retrieved, size of @ref runs. */
    csmi_diag_run_t** runs;; /**< A listing of diagnostic runs retrieved, size defined by @ref num_runs.*/
};
/**
 * @brief An input wrapper for @ref csm_diag_run_query_details.
 */
typedef struct csm_diag_run_query_details_input_t csm_diag_run_query_details_input_t;
struct csm_diag_run_query_details_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t run_id; /**< A Diagnostic run id to search the database for. */
};
/**
 * @brief A wrapper for the output of @ref csm_diag_run_query_details.
 */
typedef struct csm_diag_run_query_details_output_t csm_diag_run_query_details_output_t;
struct csm_diag_run_query_details_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint64_t num_details; /**< The number of details objects retrieved, size of @ref details.*/
    csmi_diag_run_t* run_data; /**< The diagnostic run data retrieved from the query. */
    csmi_diag_run_query_details_result_t** details; /**< A list of details objects, size defined by @ref num_details. */
};
/** @} */

#ifdef __cplusplus
}
#endif
#endif
