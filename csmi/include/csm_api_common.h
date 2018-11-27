/*================================================================================

    csmi/include/csm_api_common.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef _CSM_INIT_TERM_H
#define _CSM_INIT_TERM_H

#include "csmi_type_common.h"

/** @file csm_api_common.h
 *
 *  @brief A collection of functions and structs common to all CSM APIs.
 *
 *  Invocation of @ref csm_init_lib and @ref csm_term_lib will generally follow the following pattern:
 *      
 *      // Ensure that the initialization was a success, a more robust usage might react to errors.
 *      if ( csm_init_lib() ) return;
 *
 *      // In this example we perform a single csm_allocation_query_active_all
 *      // After invoking csm_init_lib once the user may use as many API as deemed necessary.
 *      csm_api_object *csm_obj = NULL;
 *      csmi_allocation_t** allocations = NULL;
 *      int dataCount;
 *
 *      int retCode = csm_allocation_query_active_all( &csm_obj, &allocations, &dataCount );
 *      
 *      if ( retCode == CSMI_SUCCESS ) { ...do some work here.. }
 *      else
 *      {
 *          char *errmsg = csm_api_object_errmsg_get(csm_obj);
 *          ... do something with the error message ...
 *      }
 *      // Always destroy the csm object to prevent memory leaks.
 *      csm_api_object_destroy(csm_obj);
 *
 *      // Terminate the connection when there are no further API calls planned or possible.
 *      // A more robust implementation may use the error code for reporting/logging.
 *      csm_term_lib();
 *
 */

#ifdef __cplusplus
extern "C" {    
#endif
//
// ===============================================================================================
/** @ingroup common_apis 
 * @brief A struct for holding the output of an api.
 * 
 * This struct is a helper to the csm apis. In the event of a successful API run this
 * struct will be populated with any data malloc'd by the api.
 *
 * If a failure occurred in the invocation of the API this struct will receive both an error code
 * and an error message which are accessible through @ref csm_api_object_errcode_get and
 * @ref csm_api_object_errmsg_get respectively.
 *
 * Regardless of the execution status or if the API returns any data through the struct it is 
 * recommended to run @ref csm_api_object_destroy on the struct to prevent memory leaks.
 *
 *  @note A @ref csm_api_object **DOES NOT** support being reused before invoking 
 *      @ref csm_api_object_destroy.
 *
 * A typical usage of the @ref csm_api_object struct will look something like this:
 *
 *      if ( csm_init_lib() ) return;
 *
 *      csm_api_object *csm_obj = NULL;
 *      csmi_allocation_t** allocations = NULL;
 *      int dataCount;
 *
 *      int retCode = csm_allocation_query_active_all( &csm_obj, &allocations, &dataCount );
 *      
 *      if ( retCode == CSMI_SUCCESS ) { ...do some work here.. }
 *      else
 *      {
 *          char *errmsg = csm_api_object_errmsg_get(csm_obj);
 *          ... do something with the error message ...
 *      }
 *      // Always destroy the object to prevent memory leaks.
 *      csm_api_object_destroy(csm_obj);
 *
 *      csm_term_lib();
 *
 */
typedef struct 
{
  void *hdl; ///< A void pointer to hold structures allocated by an api.
} csm_api_object;

/** @ingroup common_apis 
 * @brief Retrieves the error code from the supplied @ref csm_api_object.
 *
 * @param[in] csm_obj A @ref csm_api_object that has been used in an API.
 *
 * @returns @ref CSMERR_NOTDEF If @p csm_obj or @ref csm_api_object.hdl is NULL.
 * @returns The error code set by the api the @p csm_obj was used with (default is @ref CSMI_SUCCESS).
 */
int csm_api_object_errcode_get(csm_api_object *csm_obj);

/** @ingroup common_apis
 *
 * @brief Retrieves the error message from the supplied @ref csm_api_object.
 *
 * @param[in] csm_obj A @ref csm_api_object that has been used in an API.
 *
 * @returns NULL If the @p csm_obj or @ref csm_api_object.hdl is NULL. Additionally, if the 
 *                  API did not set the error message this function will return null.
 * @returns The error string set by the API.
 */
char *csm_api_object_errmsg_get(csm_api_object *csm_obj);

/** @ingroup common_apis
 * @brief Getter for the number of nodes which had errors.
 *
 * @param[in] csm_obj A @ref csm_api_object that has been used in an API.
 *
 * @return The number of nodes with errors.
 */
uint32_t csm_api_object_node_error_count_get(csm_api_object *csm_obj);

/** @ingroup common_apis
 * @brief Getter for the node error code.
 *
 * @param[in] csm_obj A @ref csm_api_object that has been used in an API.
 * @param[in] index The index of the node error in the internal array.
 *
 * @return The error code, if the error code couldn't be retrieved, -1 is returned.
 */
int csm_api_object_node_error_code_get(csm_api_object *csm_obj, uint32_t index);

/** @ingroup common_apis
 * @brief Getter for the node error source (generally a hostname).
 *
 * @param[in] csm_obj A @ref csm_api_object that has been used in an API.
 * @param[in] index The index of the node error in the internal array.
 *
 * @return The source of the error, null if not found.
 */
const char* csm_api_object_node_error_source_get(csm_api_object *csm_obj, uint32_t index);

/** @ingroup common_apis
 * @brief Getter for the node error message.
 *
 * @param[in] csm_obj A @ref csm_api_object that has been used in an API.
 * @param[in] index The index of the node error in the internal array.
 *
 * @return The error message, null if not found.
 */
const char* csm_api_object_node_error_msg_get(csm_api_object *csm_obj, uint32_t index);


/** @ingroup common_apis
 * @brief Retrieves the trace id from the supplied @ref csm_api_object.
 *
 * @param[in] csm_obj A @ref csm_api_object that has been used in an API.
 *
 * @returns The trace id of the api object.
 */
uint32_t csm_api_object_traceid_get(csm_api_object *csm_obj);

/** @ingroup common_apis
 * @brief Frees the supplied @ref csm_api_object and runs any API specified destruction operations.
 *
 * This api performs a free operation on @p csm_obj and destroys any data allocated by the API
 * this struct was used to invoke. This function **DOES NOT** null @p csm_obj. If this pointer
 * is to be reused it is recommended to NULL it.
 *
 * @param [in] csm_obj A @ref csm_api_object to destroy.
 */
void csm_api_object_destroy(csm_api_object *csm_obj);

/** @ingroup common_apis
 * @brief Invokes the destroy function for the returned data stored in @p csm_obj.
 *
 * This should only be used in cases where the @ref csm_api_object is going to be reused,
 * to free the object, invoke @ref csm_api_object_destroy.
 *
 * @param [in] csm_obj The object to free the returned data of.
 */
void csm_api_object_clear(csm_api_object *csm_obj);

// ===============================================================================================

/** @ingroup common_apis
 * @def csm_init_lib()
 * A wrapper for the @ref csm_init_lib_vers function, populating the version_id parameter with
 * the active CSM Version code specified by @ref CSM_VERSION_ID.
 */
#define csm_init_lib() csm_init_lib_vers(CSM_VERSION_ID)

/** @ingroup common_apis
 * @brief Initializes CSM API library so it may connect to the local daemon.
 * 
 * Invoke this function before running any CSM API functions. If @ref csm_init_lib_vers is not invoked
 * the APIs will be unable to communicate with the CSM Daemons. If the invoking code terminates,
 * it is recommended that @ref csm_term_lib_vers is called to prevent memory leaks and close the
 * daemon connection.
 *
 * @todo Library is not fork or thread safe yet, the caller is responsible to prevent
 *          shared use of the calls. (Is this still true, John Dunham 6/12/17).
 *
 * @param[in] version_id The version id 
 *
 * @note environment variables that impact behavior:
 *  CSM_LOGLEVEL   allows to control the amount of logging (default: info)
 *  CSM_SSOCKET    set the socket path of the daemon (default: /run/csmd.sock)
 *
 * @returns 
 *        Error Code | Description
 *        -----------|-------------
 *                 0 | Successfully initialized the library.
 *                >0 | Unable to initalize the library.
 */
int csm_init_lib_vers(int64_t version_id);


/** @ingroup common_apis
 * @brief Terminates the connection to the CSM API library.
 *
 * Disconnects the daemon and terminates the library. Must be paired with a @ref csm_init_lib 
 * function call. This function must be called to prevent memory leaks and terminate connections
 * to the daemon.
 *
 * @returns 
 *        Error Code | Description
 *        -----------|-------------
 *                 0 | Successfully terminate the library.
 *                >0 | Unable to terminate the library.
 */
int csm_term_lib();

// ===============================================================================================

/** @brief Finds @p enum_str in @p enum_strs and returns the index.
 *  
 *  @note It's recommended to use @ref csm_get_enum_from_string, as it will automatically get the 
 *      appropriate @p enum_strs array.
 *  
 *  @param[in] enum_str The string to convert to an enum.
 *  @param[in] enum_strs The stringified versions of the enum parameters, 
 *                          the last field is assumed null.
 *
 *  @return -1 The string could not be found.
 *  @return The int version of the enum member matching the string.
 */
int csm_enum_from_string(char *enum_str, const char *enum_strs[]); 



#ifdef __cplusplus
}
#endif

#endif
