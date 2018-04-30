/*================================================================================

    csmi/include/csm_api_ras.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_API_RAS_H__
#define __CSM_API_RAS_H__

#include <stdio.h>
#include "csmi_type_ras_funct.h"

#ifdef __cplusplus
extern "C" {    
#endif

/** @file csm_api_ras.h
 *  @brief Function prototypes and structures for @ref ras_apis
 *
 *  This contains the prototypes and struct definitions for the CSM RAS APIs
 *  and any macros, constants, or global variables you will need.
 *
 *	@author Nick Buonarota (nbuonar@us.ibm.com)
 *  @author Jon Cohn (jcohn@us.ibm.com)
 *	@author John Dunham (jdunham@us.ibm.com)
 */

/** @ingroup ras_apis 
 * @brief csm_ras_event_create -- create ras event with 
 *         additional key value information pairs.
 *  
 * @param handle A pointer to return a pointer to internally managed api data.
 *  @param msg_id -- identifier string for this ras event.  must
 *              be unique within the category and component
 *              the full message id in the db is: 
 *                  category/component/severity/id 
 *    subscription topics go: /csm/ras/category/component/severity 
 *  @param time_stamp -- time_stamp string of this event...  ISO 8601, including timezone...
 *  @param location_name -- location_name / origin of the message
 *  @param raw_data -- raw data to put into the event... (may be
 *                 NULL)
 *  @param kvcsv -- pointer to key value pairs, (may be NULL)...
 *                 key1=value1,key2=value2,
 * @returns -- 0 success != 0 error. 
 */
int csm_ras_event_create(csm_api_object **csm_obj,
                         const char *msg_id, 
                         const char *time_stamp,
                         const char *location_name,
                         const char *raw_data,
                         const char *kvcsv);

/** @ingroup ras_apis 
 * @brief csm_ras_subscribe -- Subscribe to a RAS topic;
 *  
 *  topics are messages without the very last part of the
 *  message id hierarchy. subscribe topics can be wild carded with
 *  the "#" characters.
 *  
 * @param handle A pointer to return a pointer to internally managed api data.
 * @param topic -- topic string of ras event to subscribe to. 
 *                 "." separators will be changed to /
 *                 characters prior to passing to the messaging
 *                 systems.
 * @maxevents -- maximum events to buffer at any one time. 
 *                
 *
 * @return 0 Success
 * @return errno Positive non-zero values correspond with errno.
 *         strerror() can be used to interpret 
 * @return TBD CSM specific errors
 */
int csm_ras_subscribe(csm_api_object **csm_obj,
                      const char *topic);

/** @ingroup ras_apis 
 * @brief csm_ras_unsubscribe Unsubscribe from a RAS topic;
 *
 * @param handle A pointer to return a pointer to internally managed api data.
 * @param topic -- topic string of ras event to subscribe to. 
 *                 "." separators will be changed to /
 *                 characters prior to passing to the messaging
 *                 systems.
 *                 "ALL" topic will unsubscribe ALL topics from
 *                 the daemon...
 *
 * @return 0 Success
 * @return errno Positive non-zero values correspond with errno.
 *         strerror() can be used to interpret 
 * @return TBD CSM specific errors
 */
int csm_ras_unsubscribe(csm_api_object **csm_obj,
                        const char *topic);


/** @ingroup ras_apis
 * @brief set the RAS subscription callback function 
 *  
 * Set the ras event callback function. This function will be 
 * called for each ras set of ras events that the ras match the 
 * topic qualifiers given when calling the csm_ras_subscribe 
 * function. 
 * 
 * 
 * @param csm_obj 
 * @param rasEventCallback 
 * 
 * @return int 
 */
typedef void (*ras_event_subscribe_callback)(csmi_ras_event_vector_t *event_vect);
int csm_ras_subscribe_callback(ras_event_subscribe_callback rasEventCallback);


/** @ingroup ras_apis 
@brief csm_ras_event_query -- Query the 'csm_ras_event_action' database table;
  
## Database Tables ##
Table                | Description 
---------------------|------------------------------------------------------------------------------
csm_ras_event_action | 

@param handle A pointer to return a pointer to internally managed api data.
@param input A pointer to a csm_ras_event_query_input_t struct containing information to be used to construct an SQL statement to query the 'csm_ras_event' table of the CSM database.
@param output Used to contain the output parameters for this API, consult @ref csm_ras_event_query_output_t for details. 
              Null in the event of an error.
              Destroy using @ref csm_api_object_destroy
  
@returns 
Error Code                                     | Description
-----------------------------------------------|---------------------------------------------------------------------------
@ref CSMI_SUCCESS                              | API completed successfully.
 */
int csm_ras_event_query(csm_api_object **csm_obj, csm_ras_event_query_input_t* input, csm_ras_event_query_output_t* *output);

/** @ingroup ras_apis 
 * @brief Used to create a new RAS message type and insert a record for the new type into the 'csm_ras_type' table of the CSM database.
 * 
 * ## Database Tables ##
 * Table              | Description 
 * -------------------|------------------------------------------------------------------------------
 * csm_ras_type       | contains the description and details for each of the possible ras event types
 * csm_ras_type_audit | records all of the changes to the ras event types in the csm_ras_type table
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with
 *                     @ref csm_api_object_destroy.
 * @param[in]  input  Specifies a RAS message type for creation, consult 
 *                      @ref csm_ras_msg_type_create_input_t for details.
 * @param[out] output The output from the create, consult @ref csm_ras_msg_type_create_output_t
 *                      for details. Destroy with @ref csm_api_object_destroy.
 * 
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with @p handle.
 *
 * @returns An error code from the table below.
 * 
 * Error Code                                     | Description
 * -----------------------------------------------|---------------------------------------------------------------------------
 * @ref CSMI_SUCCESS                              | API completed successfully.
 * @ref CSMERR_INVALID_PARAM                      | One of the parameters in the @p input was invalid.
 * @ref CSMERR_MSG_RETURNBUFFER_EMPTY             | The Return Buffer of a csmi_sendrecv_cmd message was unexpectedly empty.
 * @ref CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED | The Return Buffer of a csmi_sendrecv_cmd message was unknown or corrupted.
 */
int csm_ras_msg_type_create(csm_api_object **csm_obj, 
							csm_ras_msg_type_create_input_t* input,
							csm_ras_msg_type_create_output_t** output);

/** @ingroup ras_apis 
 * @brief Used to delete an existing RAS message type and remove its record from the 'csm_ras_type' table of the CSM database.
 * 
 * ## Database Tables ##
 * Table              | Description 
 * -------------------|------------------------------------------------------------------------------
 * csm_ras_type       | contains the description and details for each of the possible ras event types
 * csm_ras_type_audit | records all of the changes to the ras event types in the csm_ras_type table
 * 
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with
 *                     @ref csm_api_object_destroy.
 * @param[in]  input  Specifies a RAS message type for deletion.
 * @param[out] output The output from the delete, consult @ref csm_ras_msg_type_delete_output_t 
 *                      for details. Destroy with @ref csm_api_object_destroy.
 * 
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                                     | Description
 * -----------------------------------------------|---------------------------------------------------------------------------
 * @ref CSMI_SUCCESS                              | API completed successfully.
 * @ref CSMERR_INVALID_PARAM                      | One of the parameters in the @p input was invalid.
 * @ref CSMERR_MSG_RETURNBUFFER_EMPTY             | The Return Buffer of a csmi_sendrecv_cmd message was unexpectedly empty.
 * @ref CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED | The Return Buffer of a csmi_sendrecv_cmd message was unknown or corrupted.
 */
int csm_ras_msg_type_delete(csm_api_object **csm_obj,
                            csm_ras_msg_type_delete_input_t* input,
							csm_ras_msg_type_delete_output_t** output);

/** @ingroup ras_apis 
 * @brief Used to query the "csm_ras_type" table in the CSM database.
 * 
 * ## Database Tables ##
 * Table              | Description 
 * -------------------|------------------------------------------------------------------------------
 * csm_ras_type       | contains the description and details for each of the possible ras event types
 * csm_ras_type_audit | records all of the changes to the ras event types in the csm_ras_type table
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with
 *                     @ref csm_api_object_destroy.
 * @param[in]  input  Specifies the parameters to query the RAS table.
 * @param[out] output The output of the query, consult @ref csm_ras_msg_type_query_output_t for details.
 *                     Null in the event of an error.
 *                     Destroy with @ref csm_api_object_destroy.
 * 
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with @p handle.
 *
 * @returns An error code from the table below.
 * 
 * Error Code                                     | Description
 * -----------------------------------------------|-------------
 * @ref CSMI_SUCCESS                              | API completed successfully.
 * @ref CSMERR_INVALID_PARAM                      | One of the parameters in the @p input was invalid.
 * @ref CSMERR_MSG_RETURNBUFFER_EMPTY             | The Return Buffer of a csmi_sendrecv_cmd message was unexpectedly empty.
 * @ref CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED | The Return Buffer of a csmi_sendrecv_cmd message was unknown or corrupted.
 */
int csm_ras_msg_type_query(csm_api_object **csm_obj, 
                           csm_ras_msg_type_query_input_t* input,
						   csm_ras_msg_type_query_output_t* *output);

/** @ingroup ras_apis 
 * @brief Used to update an existing RAS message type record in the 'csm_ras_type' table of the CSM database.
 * 
 * ## Database Tables ##
 * Table              | Description 
 * -------------------|------------------------------------------------------------------------------
 * csm_ras_type       | contains the description and details for each of the possible ras event types
 * csm_ras_type_audit | records all of the changes to the ras event types in the csm_ras_type table
 * 
 * @param[out] handle An output pointer containing internally managed api data, destroy with
 *                     @ref csm_api_object_destroy.
 * @param[in]  input  Specifies the information to be used to update the *csm_ras_type* 
 *                     table.
 * @param[out] output The output from the update, consult @ref csm_ras_msg_type_update_output_t. 
 *                     Destroy with @ref csm_api_object_destroy.
 * 
 * @returns An error code from the table below.
 *
 * Error Code                                     | Description
 * -----------------------------------------------|-------------
 * @ref CSMI_SUCCESS                              | API completed successfully.
 * @ref CSMERR_INVALID_PARAM                      | One of the parameters in the @p input was invalid.
 * @ref CSMERR_MSG_RETURNBUFFER_EMPTY             | The Return Buffer of a csmi_sendrecv_cmd message was unexpectedly empty.
 * @ref CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED | The Return Buffer of a csmi_sendrecv_cmd message was unknown or corrupted.
 * 
 * @note There is no guarantee that these are the only return values for this API. As someone may have come in, written new code, and not updated these fields.
 * 
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with @p handle.
 * */
int csm_ras_msg_type_update(csm_api_object **handle,
							csm_ras_msg_type_update_input_t* input,
							csm_ras_msg_type_update_output_t** output);



/** @ingroup ras_apis
 * @brief Retrieves the RAS events matching the specified allocation.
 *
 * @param[out] handle An output pointer containing internally managed api data, destroy with
 *                      @ref csm_api_object_destroy.
 * @param[in]  input  Specifies the parameters to query the RAS table.
 * @param[out] output The RAS events associated with the @p input configuration. Destroy with
 *                      @ref csm_api_object_destroy.
 *
 * @note The error message may be retrieved though use of @ref csm_api_object_errmsg_get with
 *          @p handle.
 *
 * @returns An error code from the table below.
 *
 * Error Code                   | Description
 * -----------------------------|-------------
 * @ref CSMI_SUCCESS            | Successfully retrieved the results from the database
 * @ref CSMI_NO_RESULTS         | No events were retrieved, but no error was detected.
 * @ref CSMERR_DAEMON_DB_ERR    | Handler wasn't able to connect to database.
 * @ref CSMERR_DB_ERROR         | A database table was in an invalid/unexpected state.
 * @ref CSMERR_MSG_PACK_ERROR   | Unable to pack a message payload.
 * @ref CSMERR_MSG_UNPACK_ERROR | Unable to unpack a message payload.
 */
int csm_ras_event_query_allocation( csm_api_object **handle, 
                                    csm_ras_event_query_allocation_input_t* input,
                                    csm_ras_event_query_allocation_output_t** output);
#ifdef __cplusplus
}
#endif

#endif




