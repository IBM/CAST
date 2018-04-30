/*================================================================================

    csmi/src/bb/include/csmi_internal_bb_funct.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file src/bb/include/csmi_internal_bb_funct.h
 * @brief A collection of serialization helper functions for @ref bb_apis.
 * 
 * If the user wants meaningful defaults for their CSM structs, it is 
 * recommended to use the struct's corresponding init function. 
 * Structs initialized through this function should generally be free'd 
 * using the corresponding free.
 */
#include "csmi_type_bb.h"
#include <stdint.h>
#include "csmi/src/common/include/csm_serialization_x_macros.h"

#ifndef _CSMI_BB_TYPE_FUNCTS_H_
#define _CSMI_BB_TYPE_FUNCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup csmi_vg_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_vg_record_t( csmi_vg_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_vg_record_t( csmi_vg_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_vg_record_t( csmi_vg_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_vg_record_t( csmi_vg_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_lv_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_lv_record_t( csmi_lv_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_lv_record_t( csmi_lv_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_lv_record_t( csmi_lv_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_lv_record_t( csmi_lv_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_bb_vg_ssd_info_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_bb_vg_ssd_info_t( csmi_bb_vg_ssd_info_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_bb_vg_ssd_info_t( csmi_bb_vg_ssd_info_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_bb_vg_ssd_info_t( csmi_bb_vg_ssd_info_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_bb_vg_ssd_info_t( csmi_bb_vg_ssd_info_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_cmd_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_cmd_input_t( csm_bb_cmd_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_cmd_input_t( csm_bb_cmd_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_cmd_input_t( csm_bb_cmd_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_cmd_input_t( csm_bb_cmd_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_cmd_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_cmd_output_t( csm_bb_cmd_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_cmd_output_t( csm_bb_cmd_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_cmd_output_t( csm_bb_cmd_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_cmd_output_t( csm_bb_cmd_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_lv_create_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_lv_create_input_t( csm_bb_lv_create_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_lv_create_input_t( csm_bb_lv_create_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_lv_create_input_t( csm_bb_lv_create_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_lv_create_input_t( csm_bb_lv_create_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_lv_delete_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_lv_delete_input_t( csm_bb_lv_delete_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_lv_delete_input_t( csm_bb_lv_delete_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_lv_delete_input_t( csm_bb_lv_delete_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_lv_delete_input_t( csm_bb_lv_delete_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_lv_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_lv_query_input_t( csm_bb_lv_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_lv_query_input_t( csm_bb_lv_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_lv_query_input_t( csm_bb_lv_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_lv_query_input_t( csm_bb_lv_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_lv_query_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_lv_query_output_t( csm_bb_lv_query_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_lv_query_output_t( csm_bb_lv_query_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_lv_query_output_t( csm_bb_lv_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_lv_query_output_t( csm_bb_lv_query_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_lv_update_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_lv_update_input_t( csm_bb_lv_update_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_lv_update_input_t( csm_bb_lv_update_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_lv_update_input_t( csm_bb_lv_update_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_lv_update_input_t( csm_bb_lv_update_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_vg_create_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_vg_create_input_t( csm_bb_vg_create_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_vg_create_input_t( csm_bb_vg_create_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_vg_create_input_t( csm_bb_vg_create_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_vg_create_input_t( csm_bb_vg_create_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_vg_delete_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_vg_delete_input_t( csm_bb_vg_delete_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_vg_delete_input_t( csm_bb_vg_delete_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_vg_delete_input_t( csm_bb_vg_delete_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_vg_delete_input_t( csm_bb_vg_delete_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_vg_delete_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_vg_delete_output_t( csm_bb_vg_delete_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_vg_delete_output_t( csm_bb_vg_delete_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_vg_delete_output_t( csm_bb_vg_delete_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_vg_delete_output_t( csm_bb_vg_delete_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_vg_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_vg_query_input_t( csm_bb_vg_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_vg_query_input_t( csm_bb_vg_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_vg_query_input_t( csm_bb_vg_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_vg_query_input_t( csm_bb_vg_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_bb_vg_query_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_bb_vg_query_output_t( csm_bb_vg_query_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_bb_vg_query_output_t( csm_bb_vg_query_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_bb_vg_query_output_t( csm_bb_vg_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_bb_vg_query_output_t( csm_bb_vg_query_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
