/*================================================================================

    csmi/src/wm/include/csmi_internal_wm_funct.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file src/wm/include/csmi_internal_wm_funct.h
 * @brief A collection of serialization helper functions for @ref wm_apis.
 * 
 * If the user wants meaningful defaults for their CSM structs, it is 
 * recommended to use the struct's corresponding init function. 
 * Structs initialized through this function should generally be free'd 
 * using the corresponding free.
 */
#include "csmi_type_wm.h"
#include <stdint.h>
#include "csmi/src/common/include/csm_serialization_x_macros.h"

#ifndef _CSMI_WM_TYPE_FUNCTS_H_
#define _CSMI_WM_TYPE_FUNCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup csmi_allocation_history_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_history_t( csmi_allocation_history_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_history_t( csmi_allocation_history_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_history_t( csmi_allocation_history_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_history_t( csmi_allocation_history_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_t( csmi_allocation_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_t( csmi_allocation_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_t( csmi_allocation_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_t( csmi_allocation_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_mcast_context_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_mcast_payload_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_mcast_payload_t( csmi_allocation_mcast_payload_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_mcast_payload_t( csmi_allocation_mcast_payload_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_mcast_payload_t( csmi_allocation_mcast_payload_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_mcast_payload_t( csmi_allocation_mcast_payload_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_accounting_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_accounting_t( csmi_allocation_accounting_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_accounting_t( csmi_allocation_accounting_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_accounting_t( csmi_allocation_accounting_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_accounting_t( csmi_allocation_accounting_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_step_list_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_list_t( csmi_allocation_step_list_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_step_list_t( csmi_allocation_step_list_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_list_t( csmi_allocation_step_list_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_step_list_t( csmi_allocation_step_list_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_maptag_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_maptag_t( csmi_allocation_maptag_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_maptag_t( csmi_allocation_maptag_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_maptag_t( csmi_allocation_maptag_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_maptag_t( csmi_allocation_maptag_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_details_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_details_t( csmi_allocation_details_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_details_t( csmi_allocation_details_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_details_t( csmi_allocation_details_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_details_t( csmi_allocation_details_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_input_t( csm_allocation_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_query_input_t( csm_allocation_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_input_t( csm_allocation_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_query_input_t( csm_allocation_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_query_details_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_query_details_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_update_state_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_step_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_t( csmi_allocation_step_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_step_t( csmi_allocation_step_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_t( csmi_allocation_step_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_step_t( csmi_allocation_step_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_step_history_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_history_t( csmi_allocation_step_history_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_step_history_t( csmi_allocation_step_history_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_history_t( csmi_allocation_step_history_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_step_history_t( csmi_allocation_step_history_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_step_mcast_payload_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_allocation_step_mcast_context_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_query_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_query_details_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_query_details_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_details_output_t( csm_allocation_step_query_details_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_query_details_output_t( csm_allocation_step_query_details_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_details_output_t( csm_allocation_step_query_details_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_query_details_output_t( csm_allocation_step_query_details_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_query_active_all_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_query_active_all_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_active_all_output_t( csm_allocation_step_query_active_all_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_query_active_all_output_t( csm_allocation_step_query_active_all_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_active_all_output_t( csm_allocation_step_query_active_all_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_query_active_all_output_t( csm_allocation_step_query_active_all_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_node_resources_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_resources_record_t( csmi_node_resources_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_node_resources_record_t( csmi_node_resources_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_resources_record_t( csmi_node_resources_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_node_resources_record_t( csmi_node_resources_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_resources_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_input_t( csm_node_resources_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_resources_query_input_t( csm_node_resources_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_input_t( csm_node_resources_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_resources_query_input_t( csm_node_resources_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_resources_query_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_output_t( csm_node_resources_query_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_resources_query_output_t( csm_node_resources_query_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_output_t( csm_node_resources_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_resources_query_output_t( csm_node_resources_query_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_resources_query_all_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_resources_query_all_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_cgroup_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_cgroup_t( csmi_cgroup_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_cgroup_t( csmi_cgroup_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_cgroup_t( csmi_cgroup_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_cgroup_t( csmi_cgroup_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_cgroup_create_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_cgroup_create_output_t( csm_allocation_step_cgroup_create_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_cgroup_create_output_t( csm_allocation_step_cgroup_create_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_cgroup_create_output_t( csm_allocation_step_cgroup_create_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_cgroup_create_output_t( csm_allocation_step_cgroup_create_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_allocation_step_cgroup_delete_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_cgroup_delete_output_t( csm_allocation_step_cgroup_delete_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_allocation_step_cgroup_delete_output_t( csm_allocation_step_cgroup_delete_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_cgroup_delete_output_t( csm_allocation_step_cgroup_delete_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_allocation_step_cgroup_delete_output_t( csm_allocation_step_cgroup_delete_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
