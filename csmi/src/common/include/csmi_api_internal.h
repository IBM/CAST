/*================================================================================

    csmi/src/common/include/csmi_api_internal.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSMI_API_INTERNAL_H
#define __CSMI_API_INTERNAL_H

#include "csmi/src/common/include/csmi_cmds.h"
#include "csmi/include/csm_api_common.h"
#include <inttypes.h>

/* FIXME THIS NEEDS DOCUMENTATION! */
typedef void (*freePrototype)(csm_api_object *);

/** @brief A structure for tracking the internal state of an API.
 * 
 */
typedef struct {
  int errcode;                   ///< The error code reported by a csmi command.
  char* errmsg;                  ///< An error message.
  uint32_t traceid;              ///< A trace id for tracing and debug purposes.

  csmi_cmd_t cmd;                ///< The enumerated identifier for the command.
  freePrototype csmi_free_func;  ///< A function pointer for a memory freeing function.

  void *ret_cdata;               ///< Returned data for an api, must be set by API.
  int ret_cdata_size;            ///< The size of the returned data.

  uint32_t errorlist_count;      ///< The number of node errors.
  csm_node_error_t** errlist;    ///< The errors reported by nodes.
} csmi_api_internal;

#ifdef __cplusplus
extern "C"
{
#endif

csm_api_object * csm_api_object_new(csmi_cmd_t cmd, freePrototype free_func);
void csm_api_object_set_retdata(csm_api_object *csm_obj, int ret_data_size, void *ret_data);
void csmi_api_handler_destroy(csmi_api_internal *csmi_hdl);

void csm_api_object_errcode_set(csm_api_object *csm_obj, int errcode);
void csm_api_object_errmsg_set(csm_api_object *csm_obj, char *msg);
void csm_api_object_errlist_set(csm_api_object *csm_obj, uint32_t n_count, csm_node_error_t** node_errors);  


/**
 * @brief Sets the trace id in the internal handle for the api object.
 *
 * @param[out] csm_obj The object to set the trace id for.
 * @param[in]  traceId The new trace id for this API object.
 */
void csm_api_object_trace_set(csm_api_object *csm_obj, uint32_t traceId);

// Helper Macros for APIs.
// ==============================================================================================
/**@brief Tests the csm_api_object and returns @ref CSMERR_MEM_ERROR if it fails. 
 * 
 * @param[out] csm_obj The @ref csm_api_object to construct.
 * @param[in]  expected_cmd The @ref csmi_cmd_t representing the object.
 * @param[in]  free_funct The @ref freePrototype for destroying the @ref csm_api_object.
 */
#define create_csm_api_object( csm_obj, expected_cmd, free_funct  )                 \
    *csm_obj = csm_api_object_new( expected_cmd, free_funct );                      \
    if (*csm_obj == NULL){                                                          \
        csmutil_logging(error, "csmi_api_object NULL\n");                           \
        return CSMERR_MEM_ERROR;                                                    \
    }

#define test_serialization( csm_obj, buffer )                               \
    if ( buffer == NULL) {                                                  \
        csmutil_logging(error, "Failed to pack arguments, make sure version is set!");                 \
        csm_api_object_errcode_set(*csm_obj, CSMERR_MSG_PACK_ERROR);        \
        csm_api_object_errmsg_set(*csm_obj,                                 \
            strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMERR_MSG_PACK_ERROR))); \
        return CSMERR_MSG_PACK_ERROR;                                       \
    } 


/** @brief Serializes an array of strings.
 *
 * @param[in]  array             The array of strings to be serialized.
 * @param[in]  array_len         The length of @p array.
 * @param[out] return_buffer     The serialized buffer.
 * @param[out] return_buffer_len The length of the buffer @p return_buffer.
 *
 * @return 0 The array was successfully serialized.
 * @return 1 The array was not successfully serialized.
 */
int csm_serialize_str_array( char** array, uint32_t array_len,
                         char** return_buffer, uint32_t* return_buffer_len );

/** @brief Deserializes an array of strings.
 *
 * @param[out] array      The array of strings that was deserialized.
 * @param[out] array_len  The length of @p array.
 * @param[in]  buffer     The buffer to deserialize.
 * @param[in]  buffer_len The length of the buffer @p return_buffer.
 *
 * @return 0 The array was successfully deserialized.
 * @return 1 The array was not successfully deserialized.
 */
int csm_deserialize_str_array( char** array[], uint32_t* array_len,
                           const char* buffer, uint32_t buffer_len );

/** @brief Serializes a single primative value.
 * @param[in]  value The value to serialize.
 * @param[out] buffer The serialized buffer.
 * @param[out] buffer_length The size of the buffer.
 */
#define csm_primative_serializer(value, buffer, buffer_len)\
    buffer = (char*)malloc(sizeof(value));   \
    buffer_len=sizeof(value);                \
    memcpy(buffer, &(value), sizeof(value));

/** @brief Deserializes a single primative value.
 * @param[in]  value The value to deserialize.
 * @param[out] buffer The serialized buffer.
 * @param[out] buffer_length The size of the buffer.
 */
#define csm_primative_deserializer(value, buffer, buffer_len)\
    memcpy( &(value), buffer, buffer_len);

// ==============================================================================================



#ifdef __cplusplus
}
#endif

#endif
