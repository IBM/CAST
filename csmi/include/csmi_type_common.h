/*================================================================================
   
    csmi/include/csmi_type_common.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_common.h
 * @brief A collection of structs for @ref common_apis.
 */
#ifndef _CSMI_COMMON_TYPES_H_
#define _CSMI_COMMON_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

// Begin special_preprocess directives
#include "csm_api_version.h"
#include "csm_api_consts.h"
#include "csm_api_macros.h"
#include <stdint.h>


/** @addtogroup common_apis
 * @{
 */

 /** @ingroup common_apis
  *  @brief CSM boolean variable, a third state is provided for query APIs.
  *
  *  --|--------------------------
  *  0 | FALSE ( @ref CSM_FALSE )
  *  1 | TRUE  ( @ref CSM_TRUE )
  *  2 | If set a CSM query will not filter on the field ( @ref CSM_UNDEF_BOOL ).
  */
 typedef char csm_bool;

// End special_preprocess directives

/** defgroup csmi_cmd_err_t csmi_cmd_err_t
 * @{
 */
/**
  * @brief A list of the error codes supported by csm.
 * @todo Post-PRPQ: Remove CSMERR_MSG_RETURNBUFFER_EMPTY, CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED
  */
typedef enum {
   CSMI_SUCCESS=0, ///< 0 - Indicates a success.
   CSMERR_GENERIC=1, ///< 1 - Indicates that the error was not defined.
   CSMI_HELP=2, ///< 2 - Indicates that a command line executable exited through the help functionality.
   CSMI_NOT_INITIALIZED=3, ///< 3 - Indicates user did not run @ref csm_init_lib successfully.
   CSMI_NO_RESULTS=4, ///< 4 - Indicates that a query API received no errors, but no results.
   CSMERR_TIMEOUT=5, ///< 5 - Indicates that a timeout occurred in execution of API. 
   CSMERR_MSGID_MISMATCH=6, ///< 6 - Indicates that the message id received did not match the sent id.
   CSMERR_CMD_MISMATCH=7, ///< 7 - Indicates that a @ref csm_api_object was reused. @todo reusable api objs.
   CSMERR_MISSING_PARAM=8, ///< 8 - Indicates a required parameter was not supplied.
   CSMERR_INVALID_PARAM=9, ///< 9 - Indicates a supplied parameter was illegal for the API invocation.
   CSMERR_RAS_HANDLER_ERROR=10, ///< 10 - Indicates an error occurred in a RAS Handler.
   CSMERR_CMD_UNKNOWN=11, ///< 11 - Indicates that the API was given an invalid command id.
   CSMERR_SENDRCV_ERROR=12, ///< 12 - Indicates an error occurred in the network transmission section of the API.
   CSMERR_MEM_ERROR=13, ///< 13 - Indicates that a memory allocation failed.
   CSMERR_NOTDEF=14, ///< 14 - Indicates that a RAS command could not be found in response to an event. @todo might need to be removed.
   CSMERR_PUBSUB=15, ///< 15 - Indicates that a RAS topic could not be found.
   CSMERR_PERM=16, ///< 16 - Indicates that the invoking user did not have permission for the resources requested.
   CSMERR_SCRIPT_FAILURE=17, ///< 17 - Indicates that an Allocation Create or Delete could not execute the prolog or epilog.
   CSMERR_INIT_LIB_FAILED=18, ///< 18 - Indicates that the API was unable to connect to a local daemon.
   CSMERR_TERM_LIB_FAILED=19, ///< 19 - Indicates that the API failed to close the connection to the local daemon.
   CSMERR_PAYLOAD_EMPTY=20, ///< 20 - Indicates that the Payload of a network message was empty unexpectedly.
   CSMERR_BAD_EVENT_TYPE=21, ///< 21 - Indicates that the event processed by a handler was invalid.
   CSMERR_BAD_ADDR_TYPE=22, ///< 22 - Indicates that the address of a message received by handler was of an unexpected type.
   CSMERR_CGROUP_DEL_ERROR=23, ///< 23 - Indicates an error occurred while deleting a CGroup.
   CSMERR_DAEMON_DB_ERR=24, ///< 24 - Indicates that the Daemon had trouble with a database connection.
   CSMERR_DB_ERROR=25, ///< 25 - Indicates that the state of a table was invalid.
   CSMERR_DEL_MISMATCH=26, ///< 26 - Indicates that the API failed to delete all the requested records.
   CSMERR_UPDATE_MISMATCH=27, ///< 27 - Indicates that the API failed to update all the requested records.
   CSMERR_MSG_PACK_ERROR=28, ///< 28 - Indicates something went wrong serializing a message payload.
   CSMERR_MSG_UNPACK_ERROR=29, ///< 29 - Indicates something went wrong deserializing a message payload.
   CSMERR_MSG_BUFFER_EMPTY=30, ///< 30 - Indicates the string buffer of a network message was empty unexpectedly.
   CSMERR_MSG_RETURNBUFFER_EMPTY=31, ///< 31 - Indicates the return buffer of a network message was empty unexpectedly.
   CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED=32, ///< 32 - Indicates the return buffer of a network message was unknown or corrupted.
   CSMERR_MULTI_GEN_ERROR=33, ///< 33 - Indicates that generating the multicast message failed.
   CSMERR_MULTI_RESP_ERROR=34, ///< 34 - Indicates that an error occured between the Master and Compute Daemon.
   CSMERR_EXEC_ILLEGAL=35, ///< 35 - Indicates that an API has an invalid permission level in the acl file.
   CSMERR_CGROUP_FAIL=36, ///< 36 - Indicates that cgroup failed to be modified, generic error.
   CSMERR_CONTEXT_LOST=37, ///< 37 - Indicates that the backend handler context, or its user data, was unexpectedly null.
   CSMERR_CGROUP_EINVAL=38, ///< 38 - Indicates an attempt to write an invalid value to a cgroup parameter.
   CSMERR_CGROUP_EACCES=39, ///< 39 - Indicates an attempt to write an invalid resource to a cgroup parameter. 
   CSMERR_BB_CMD_ERROR=40, ///< 40 - The generic burst buffer command error.
   CSM_STATE_JUMPED=41, ///< 41 - Indicates an allocation jumped states (legal, non error).
   CSM_SAME_STATE_TRANSITION=42, ///< 42 - Indicates an allocation update state operation transitioned to the same state.
   CSMERR_STATE_CHANGE_FAILED=43, ///< 43 - Indicates an allocation update state operation failed to be completed, usually a problem in the compute nodes.
   CSMERR_DELETE_STATE_BAD=44, ///< 44 - Indicates an allocation delete operation failed due to an invalid state.
   CSMERR_JSRUN_CMD_ERROR=45, ///< 45 - Indicates a JSRUN start failed.
   CSMERR_ALLOC_INVALID_NODES=46, ///< 46 - Indicates one or more nodes were not in the CSM database for the allocation create.
   CSMERR_ALLOC_OCCUPIED_NODES=47, ///< 47 - Indicates nodes in the list were in use by another allocation.
   CSMERR_ALLOC_UNAVAIL_NODES=48, ///< 48 - Indicates the allocation create failed due to nodes that were not avaliable.
   CSMERR_ALLOC_BAD_FLAGS=49, ///< 49 - Indicates the allocation create failed due to the prolog having bad allocation flags.
   CSMERR_ALLOC_MISSING=50, ///< 50 - Indicates the allocation delete failed due to a missing allocation.
   CSMERR_EPILOG_EPILOG_COLLISION=51, ///< 51 - Indicates an epilog collided with an epilog.
   CSMERR_EPILOG_PROLOG_COLLISION=52, ///< 52 - Indicates an epilog collided with a prolog.
   CSMERR_PROLOG_EPILOG_COLLISION=53, ///< 53 - Indicates a prolog collided with an epilog.
   CSMERR_PROLOG_PROLOG_COLLISION=54, ///< 54 - Indicates a prolog collided with a prolog.
   csmi_cmd_err_t_MAX=55 ///< 55 - Bounding Value
} csmi_cmd_err_t;



/**
 * @brief Maps enum csmi_cmd_err_t value to string.
 */
extern const char* csmi_cmd_err_t_strs [];
/** @}*/


/** defgroup csmi_node_type_t csmi_node_type_t
 * @{
 */
/**
  * @brief The different types of nodes supported by CSM.
 *
 * The enumerated string for an enum is stored in the database.
  */
typedef enum {
   CSM_NODE_NO_TYPE=0, ///< 0 - Denotes a node with no type.
   CSM_NODE_MANAGEMENT=1, ///< 1 - Denotes a management node.
   CSM_NODE_SERVICE=2, ///< 2 - Denotes a service node.
   CSM_NODE_LOGIN=3, ///< 3 - Denotes a login node.
   CSM_NODE_WORKLOAD_MANAGER=4, ///< 4 - Denotes a workload manager node.
   CSM_NODE_LAUNCH=5, ///< 5 - Denotes a launch node.
   CSM_NODE_COMPUTE=6, ///< 6 - Denotes a compute node.
   CSM_NODE_UTILITY=7, ///< 7 - Denotes a utility node.
   CSM_NODE_AGGREGATOR=8, ///< 8 - Denotes an aggregator node.
   csmi_node_type_t_MAX=9 ///< 9 - Bounding Value
} csmi_node_type_t;



/**
 * @brief Maps enum csmi_node_type_t value to string.
 */
extern const char* csmi_node_type_t_strs [];
/** @}*/


/** defgroup csmi_node_state_t csmi_node_state_t
 * @{
 */
/**
  * @brief The different states a CSM node may be in.
 *
  */
typedef enum {
   CSM_NODE_NO_DEF=0, ///< 0 - csmi_node_state_t has not been set yet or value is unknown. Node has no specified state.
   CSM_NODE_DISCOVERED=1, ///< 1 - Node was discovered by CSM's inventory.
   CSM_NODE_IN_SERVICE=2, ///< 2 - Node has been marked as in service.
   CSM_NODE_OUT_OF_SERVICE=3, ///< 3 - Node has been marked as out of service.
   CSM_NODE_SYS_ADMIN_RESERVED=4, ///< 4 - Node has been marked as reserved by the sys admin.
   CSM_NODE_SOFT_FAILURE=5, ///< 5 - Node has been placed into a soft failure.
   CSM_NODE_MAINTENANCE=6, ///< 6 - Node is in Maintenance mode.
   CSM_NODE_DATABASE_NULL=7, ///< 7 - Used to represent a NULL value for CSM Database.
   CSM_NODE_HARD_FAILURE=8, ///< 8 - Node has been placed into a hard failure.
   csmi_node_state_t_MAX=9 ///< 9 - Bounding Value
} csmi_node_state_t;



/**
 * @brief Maps enum csmi_node_state_t value to string.
 */
extern const char* csmi_node_state_t_strs [];
/** @}*/


/** defgroup csmi_ras_severity_t csmi_ras_severity_t
 * @{
 */
/**
  * @brief The severity levels for a RAS event.
 *
  */
typedef enum {
   CSM_RAS_NO_SEV=0, ///< 0 - RAS event has no specified severity.
   CSM_RAS_INFO=1, ///< 1 - severity is at the info level.
   CSM_RAS_WARNING=2, ///< 2 - severity is at the warning level.
   CSM_RAS_FATAL=3, ///< 3 - severity is at the fatal level.
   csmi_ras_severity_t_MAX=4 ///< 4 - Bounding Value
} csmi_ras_severity_t;



/**
 * @brief Maps enum csmi_ras_severity_t value to string.
 */
extern const char* csmi_ras_severity_t_strs [];
/** @}*/


/**
 * @brief A container for a CSM error, encapsulates the source error code and message.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int errcode; ///< The error code of this error message. 
    char* errmsg; ///< The error message. 
    char* source; ///< The host reporting the error.
} csm_node_error_t;
/** @} */

#ifdef __cplusplus
}
#endif
#endif
