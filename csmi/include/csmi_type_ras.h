/*================================================================================
   
    csmi/include/csmi_type_ras.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_ras.h
 * @brief A collection of structs for @ref ras_apis.
 */
#ifndef _CSMI_RAS_TYPES_H_
#define _CSMI_RAS_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

// Begin special_preprocess directives
#include "csm_api_common.h"
#include "csm_api_ras_keys.h"


/** @addtogroup ras_apis
 * @{
 */

// End special_preprocess directives

/** defgroup RasSeverity RasSeverity
 * @{
 */
/**
  * @brief Severity levels for RAS events.
 * @todo Post-PRPQ: Rename to fit the new CSM standards.
  */
typedef enum {
   CSM_RAS_SEV_INFO=0, ///< 0 - Denotes an info level RAS event.
   CSM_RAS_SEV_WARN=1, ///< 1 - Denotes a warning level RAS event.
   CSM_RAS_SEV_ERROR=2, ///< 2 - Denotes an error level RAS event.
   CSM_RAS_SEV_FATAL=3, ///< 3 - Denotes a fatal level RAS event.
   RasSeverity_MAX=4 ///< 4 - Bounding Value
} RasSeverity;



/**
 * @brief Maps enum RasSeverity value to string.
 */
extern const char* RasSeverity_strs [];
/** @}*/


/**
 * @brief Represents a record in the **csm_ras_type** table of the CSM database.
 */
typedef struct csmi_ras_type_record_t csmi_ras_type_record_t;
struct csmi_ras_type_record_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t threshold_count; /**< Number of times this event has to occur during the 'threshold_period' before taking action on the RAS event. */
    int32_t threshold_period; /**< Period in seconds over which to count the 'threshold_count'. */
    csm_bool enabled; /**< (bool) Events will be processed if enabled=true and suppressed if enabled=false. */
    csm_bool visible_to_users; /**< (bool) When visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation. */
    csm_bool set_not_ready; /**< Deprecated after CSM_VERSION_0_4_1. (bool) When set_not_ready=true, resources associated with the event will be set to ready=n when the event hits threshold. */
    csm_bool set_ready; /**< Deprecated after CSM_VERSION_0_4_1. (bool) When set_ready=true, resources associated with the event will be set to ready=y when the event hits threshold. */
    char* msg_id; /**< The identifier string for this RAS event. It must be unique.  Typically it consists of three parts separated by periods "system.component.id". */
    char* control_action; /**< Name of control action script to invoke for this event. */
    char* description; /**< Description of the RAS event. */
    char* message; /**< RAS message to display to the user (pre-variable substitution). */
    csmi_ras_severity_t severity; /**< Severity of the RAS event. INFO/WARNING/FATAL. */
    csmi_node_state_t set_state; /**< replacement for 'set_ready' and 'set_not_ready' after CSM_VERSION_0_4_1. resources associated with the event will be set to this node state when the event hits threshold. */
};
/**
 * @brief An input wrapper for @ref  csm_ras_event_create.
 */
typedef struct csm_ras_event_create_input_t csm_ras_event_create_input_t;
struct csm_ras_event_create_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* msg_id; /**< A RAS message id which exists in **csm_ras_type**. */
    char* time_stamp; /**< The timestamp the RAS event was generated (YYYY-MM-DD HH:MM:SS.mmmmmm).*/ 
    char* location_name; /**< Origin of the node.*/
    char* raw_data; /**< Raw event information (Nullable).*/
    char* kvcsv; /**< Key value pairs ("k1=v1,k2=v2,..")*/
};
/**
 * @brief Represents a row in the csm_ras_event_action table.
 */
typedef struct csmi_ras_event_action_record_t csmi_ras_event_action_record_t;
struct csmi_ras_event_action_record_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t rec_id; /**< The record id of the RAS evnet. */
    int32_t count; /**< The number of times the event occured. */
    int32_t msg_id_seq; /**< The sequence number indexing the csm_ras_type_audit table.*/
    char* archive_history_time; /**< Time when the RAS event was archived to the big data store. */
    char* location_name; /**< The origin point of the RAS event.*/
    char* message; /**< RAS message text. */
    char* msg_id; /**< The type of the RAS event, for example "csm.status.up" */
    char* raw_data; /**< Event raw data. */
    char* time_stamp; /**< The time that the event was generated. */
};
/**
 * @brief Represents a row in the **csm_ras_event_action** table.
 */
typedef struct csmi_ras_event_action_t csmi_ras_event_action_t;
struct csmi_ras_event_action_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t rec_id; /**< The record id of the RAS event. */
    int32_t msg_id_seq; /**< The sequence number indexing the csm_ras_type_audit table.*/
    int32_t count; /**< The number of times the event occurred. */
    char* msg_id; /**< The type of the RAS event( e.g. **csm.status.up**). */
    char* time_stamp; /**< The time that the event was generated. */
    char* location_name; /**< The origin point of the RAS event.*/
    char* message; /**< RAS message text. */
    char* raw_data; /**< Event raw data. */
    char* archive_history_time; /**< Time when the RAS event was archived to the big data store. */
};
/**
 * @brief Represents a CSM RAS event.
 *  @todo Post-PRPQ: What differentiates from @ref csmi_ras_event_action_t.
 *  reply: I think this struct represents a record from the 'csm_ras_event_action_view'.
 * which is a special thing that Nate Besaw put together in the database.
 */
typedef struct csmi_ras_event_t csmi_ras_event_t;
struct csmi_ras_event_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t min_time_in_pool; /**< The minimum time the event is available in the RAS event pool. */
    int32_t processor; /**< @todo Post-PRPQ: What is the processor? */
    int32_t count; /**< @todo Post-PRPQ: What is the count of?*/
    char* msg_id; /**< The type of the RAS event( e.g. **csm.status.up**). */
    char* suppress_ids; /**< @todo Post-PRPQ: What is suppress_ids?*/
    char* severity; /**< The severity of the RAS event. */
    char* time_stamp; /**< The time that the event was generated.*/
    char* location_name; /**< The origin point of the RAS event.*/
    char* control_action; /**< @todo Post-PRPQ: What is a control action. */
    char* message; /**< RAS message text. */
    char* raw_data; /**< Event raw data. */
    char* kvcsv; /**< kvcsv. event specific keys and values in a comma separated list*/
    char* master_time_stamp; /**< The time when the event is process by the CSM master daemon. Used for correlating node state changes with CSM master processing of RAS events.*/
    int64_t rec_id; /**< unique identifier for this specific ras event. */
};
/**
 * @brief Collects an array of RAS events.
 */
typedef struct csmi_ras_event_vector_t csmi_ras_event_vector_t;
struct csmi_ras_event_vector_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t num_ras_events; /**< The number of RAS events, size of @ref events. */
    csmi_ras_event_t** events; /**< The list of RAS events, size defined by @ref num_ras_events.*/
};
/**
 * @brief An input wrapper for @ref csm_ras_event_query.
 * At least one of the fields must be specified to use the @ref csm_ras_event_query API.
 */
typedef struct csm_ras_event_query_input_t csm_ras_event_query_input_t;
struct csm_ras_event_query_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1. */
    csmi_ras_severity_t severity; /**< Severity of the RAS event. INFO/WARNING/FATAL, optional. */
    char order_by; /**< Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY rec_id ASC NULLS LAST'. VALID VALUES: [a] = 'ORDER BY rec_id ASC NULLS LAST', [b] =  'ORDER BY rec_id DESC NULLS LAST', [c] = 'ORDER BY time_stamp ASC NULLS LAST', [d] =  'ORDER BY time_stamp DESC NULLS LAST', [e] = 'ORDER BY master_time_stamp ASC NULLS LAST', [f] =  'ORDER BY master_time_stamp DESC NULLS LAST', [g] = 'ORDER BY location_name ASC NULLS LAST', [h] =  'ORDER BY location_name DESC NULLS LAST', [i] = 'ORDER BY msg_id ASC NULLS LAST', [j] =  'ORDER BY msg_id DESC NULLS LAST', [k] = 'ORDER BY severity ASC NULLS LAST', [l] =  'ORDER BY severity DESC NULLS LAST'. */
    char* msg_id; /**< The identifier string for the RAS event (e.g. **system.component.id**), optional. */
    char* start_time_stamp; /**< Start of the time range to query for RAS Events, optional. */
    char* end_time_stamp; /**< End of the time range to query for RAS Events, optional. */
    char* location_name; /**< A location to search for RAS events, optional. */
    char* control_action; /**< Name of control action script to query for, optional. */
    char* message; /**< The message of the RAS event to query for, optional. */
    char* master_time_stamp_search_begin; /**< A time used to filter results of the SQL query and only include records with a 'master_time_stamp' at or after (ie: '>=' ) this time. optional. */
    char* master_time_stamp_search_end; /**< A time used to filter results of the SQL query and only include records with a 'master_time_stamp' at or before (ie: '<=' ) this time. optional. */
    int64_t rec_id; /**< Query by rec_id. This is a unique identifier for this specific ras event. API will ignore values less than 1. optional. */
};
/**
 *
 */
typedef struct csm_ras_event_query_output_t csm_ras_event_query_output_t;
struct csm_ras_event_query_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< Number of elements in the 'results' array. */
    csmi_ras_event_t** results; /**< An array of all the records returned from the SQL query. */
};
/**
 * @brief An input wrapper for @ref csm_ras_event_query_allocation.
 */
typedef struct csm_ras_event_query_allocation_input_t csm_ras_event_query_allocation_input_t;
struct csm_ras_event_query_allocation_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation id to search for associated RAS events. */
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
};
/**
 * @brief A wrapper for the output of @ref csm_ras_event_query_allocation.
 */
typedef struct csm_ras_event_query_allocation_output_t csm_ras_event_query_allocation_output_t;
struct csm_ras_event_query_allocation_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t num_events; /**< Number of events retrieved by the query, size of @ref events. */
    csmi_ras_event_action_t** events; /**< A list of RAS events retrieved, size defined by @ref num_events. */
};
/**
 * @brief An input wrapper for @ref csm_ras_msg_type_create.
 */
typedef struct csm_ras_msg_type_create_input_t csm_ras_msg_type_create_input_t;
struct csm_ras_msg_type_create_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t threshold_count; /**< Number of times this event has to occur during the 'threshold_period' before taking action on the RAS event. */
    int32_t threshold_period; /**< Period in seconds over which to count the 'threshold_count'. */
    csm_bool visible_to_users; /**< (bool) When visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation. */
    csm_bool enabled; /**< Events will be processed if enabled=true and suppressed if enabled=false */
    csm_bool set_ready; /**< Deprecated after CSM_VERSION_0_4_1. (bool) When set_ready=true, resources associated with the event will be set to ready=y when the event hits threshold. */
    csm_bool set_not_ready; /**< Deprecated after CSM_VERSION_0_4_1. (bool) When set_not_ready=true, resources associated with the event will be set to ready=n when the event hits threshold. */
    char* control_action; /**< Name of control action script to invoke for this event. */
    char* description; /**< Description of the RAS event. */
    char* msg_id; /**< The identifier string for this RAS event. It must be unique.  Typically it consists of three parts separated by periods "system.component.id". */
    char* message; /**< RAS message to display to the user (pre-variable substitution). */
    csmi_ras_severity_t severity; /**< Severity of the RAS event. INFO/WARNING/FATAL. default to INFO */
    csmi_node_state_t set_state; /**< replacement for 'set_ready' and 'set_not_ready' after CSM_VERSION_0_4_1. resources associated with the event will be set to this node state when the event hits threshold. */
};
/**
 * @brief A wrapper for the output of @ref csm_ras_msg_type_create.
 */
typedef struct csm_ras_msg_type_create_output_t csm_ras_msg_type_create_output_t;
struct csm_ras_msg_type_create_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    csm_bool insert_successful; /**< The RAS creation was successful.*/
    char* msg_id; /**< Name of the message id that was inserted into the database. NULL if failed. */
};
/**
 * @brief An input wrapper for @ref csm_ras_msg_type_delete.
 */
typedef struct csm_ras_msg_type_delete_input_t csm_ras_msg_type_delete_input_t;
struct csm_ras_msg_type_delete_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t msg_ids_count; /**< Number of messages to delete, size of @ref msg_ids. */
    char** msg_ids; /**< A list of message ids to be deleted, must have more than one id. Size defined by @ref msg_ids_count. */
};
/**
 * @brief A wrapper for the output of @ref csm_ras_msg_type_delete.
 * @todo Post-PRPQ: Refactor this struct to use the standardized response.
 */
typedef struct csm_ras_msg_type_delete_output_t csm_ras_msg_type_delete_output_t;
struct csm_ras_msg_type_delete_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t expected_number_of_deleted_msg_ids; /**< Number of msg ids that were attempted to be deleted. */
    uint32_t not_deleted_msg_ids_count; /**< Number of messages that failed to be deleted, size of @ref not_deleted_msg_ids. */
    uint32_t deleted_msg_ids_count; /**< Number of messages that were successfully deleted, size of @ref deleted_msg_ids.*/
    char** not_deleted_msg_ids; /**< List of msg ids failed to be deleted, size defined by @ref not_deleted_msg_ids_count. */
    char** deleted_msg_ids; /**< List of msg ids have been deleted, size defined by @ref deleted_msg_ids_count.*/
};
/**
 * @brief An input wrapper for @ref csm_ras_msg_type_update.
 */
typedef struct csm_ras_msg_type_update_input_t csm_ras_msg_type_update_input_t;
struct csm_ras_msg_type_update_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t threshold_count; /**< Number of times this event has to occur during the 'threshold_period' before taking action on the RAS event. API will ignore NULL values.*/
    int32_t threshold_period; /**< Period in seconds over which to count the 'threshold_count'. API will ignore NULL values.*/
    csm_bool visible_to_users; /**< (bool) When visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation. API will ignore @ref CSM_UNDEF_BOOL.*/
    csm_bool set_not_ready; /**< Deprecated after CSM_VERSION_0_4_1. (bool) When set_not_ready=true, resources associated with the event will be set to ready=n when the event hits threshold. API will ignore @ref CSM_UNDEF_BOOL.*/
    csm_bool set_ready; /**< Deprecated after CSM_VERSION_0_4_1. (bool)  When set_ready=true, resources associated with the event will be set to ready=y when the event hits threshold. API will ignore @ref CSM_UNDEF_BOOL.*/
    csm_bool enabled; /**< (bool)  Events will be processed if enabled=true and suppressed if enabled=false. API will ignore @ref CSM_UNDEF_BOOL.*/
    char* control_action; /**< Name of control action script to invoke for this event. API will ignore NULL values.*/
    char* description; /**< Description of the RAS event. API will ignore NULL values.*/
    char* msg_id; /**< The identifier string for this RAS event. It must be unique.  Typically it consists of three parts separated by periods "system.component.id". Can not be NULL. API will fail if NULL.*/
    char* message; /**< RAS message to display to the user (pre-variable substitution). API will ignore NULL values.*/
    csmi_ras_severity_t severity; /**< Severity of the RAS event. INFO/WARNING/FATAL. */
    csmi_node_state_t set_state; /**< replacement for 'set_ready' and 'set_not_ready' after CSM_VERSION_0_4_1. resources associated with the event will be set to this node state when the event hits threshold. */
};
/**
 * @brief A wrapper for the output of @ref csm_ras_msg_type_update.
 */
typedef struct csm_ras_msg_type_update_output_t csm_ras_msg_type_update_output_t;
struct csm_ras_msg_type_update_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    csm_bool update_successful; /**< The RAS update was successful. */
    char* msg_id; /**< Name of the message id that was updated in the database. NULL if failed. */
};
/**
 * @brief An input wrapper for @ref  csm_ras_msg_type_query.
 */
typedef struct csm_ras_msg_type_query_input_t csm_ras_msg_type_query_input_t;
struct csm_ras_msg_type_query_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int32_t limit; /**< SQL 'LIMIT' numeric value. API will ignore values less than 1.*/
    int32_t offset; /**< SQL 'OFFSET' numeric value. API will ignore values less than 1.*/
    csmi_ras_severity_t severity; /**< Severity of the RAS event. INFO/WARNING/FATAL. optional. */
    char* control_action; /**< The control action script to query on, optional. */
    char* msg_id; /**< The identifier string for this RAS event ( **system.component.id ** ), optional. */
    char* message; /**< The message of the RAS event to query for, optional. */
    uint32_t set_states_count; /**< Number of set_states being queried, size of @ref set_states. */
    char** set_states; /**< List of set_states to perform query on. Will filter results to only include specified set_states. Size defined by @ref set_states_count. */
};
/**
 * @brief A wrapper for the output of @ref csm_ras_msg_type_query.
 */
typedef struct csm_ras_msg_type_query_output_t csm_ras_msg_type_query_output_t;
struct csm_ras_msg_type_query_output_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    uint32_t results_count; /**< The number of records retrieved by the query, size of @ref results. */
    csmi_ras_type_record_t** results; /**< A list of records retrieved by the query, size defined by @ref results_count.*/
};
/**
 * @brief An input wrapper for @ref csmi_ras_subscribe.
 */
typedef struct csm_ras_subscribe_input_t csm_ras_subscribe_input_t;
struct csm_ras_subscribe_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* topic;  /**< The topic slated for subscription.*/
};
/**
 * @brief An input wrapper for @ref csmi_ras_unsubscribe.
 */
typedef struct csm_ras_unsubscribe_input_t csm_ras_unsubscribe_input_t;
struct csm_ras_unsubscribe_input_t {
    uint64_t _metadata; /** The number of fields in the struct.*/
    char* topic; /**< Topic slated to be unsubscribed from. */
};
/** @} */

#ifdef __cplusplus
}
#endif
#endif
