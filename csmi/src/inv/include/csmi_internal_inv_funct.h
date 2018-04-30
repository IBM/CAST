/*================================================================================

    csmi/src/inv/include/csmi_internal_inv_funct.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file src/inv/include/csmi_internal_inv_funct.h
 * @brief A collection of serialization helper functions for @ref inv_apis.
 * 
 * If the user wants meaningful defaults for their CSM structs, it is 
 * recommended to use the struct's corresponding init function. 
 * Structs initialized through this function should generally be free'd 
 * using the corresponding free.
 */
#include "csmi_type_inv.h"
#include <stdint.h>
#include "csmi/src/common/include/csm_serialization_x_macros.h"

#ifndef _CSMI_INV_TYPE_FUNCTS_H_
#define _CSMI_INV_TYPE_FUNCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup csmi_dimm_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_dimm_record_t( csmi_dimm_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_dimm_record_t( csmi_dimm_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_dimm_record_t( csmi_dimm_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_dimm_record_t( csmi_dimm_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_gpu_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_gpu_record_t( csmi_gpu_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_gpu_record_t( csmi_gpu_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_gpu_record_t( csmi_gpu_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_gpu_record_t( csmi_gpu_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_hca_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_hca_record_t( csmi_hca_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_hca_record_t( csmi_hca_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_hca_record_t( csmi_hca_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_hca_record_t( csmi_hca_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_ib_cable_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_ib_cable_record_t( csmi_ib_cable_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_ib_cable_record_t( csmi_ib_cable_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_ib_cable_record_t( csmi_ib_cable_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_ib_cable_record_t( csmi_ib_cable_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_ib_cable_history_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_node_attributes_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_attributes_record_t( csmi_node_attributes_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_node_attributes_record_t( csmi_node_attributes_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_attributes_record_t( csmi_node_attributes_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_node_attributes_record_t( csmi_node_attributes_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_node_attributes_history_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_processor_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_processor_record_t( csmi_processor_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_processor_record_t( csmi_processor_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_processor_record_t( csmi_processor_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_processor_record_t( csmi_processor_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_ssd_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_ssd_record_t( csmi_ssd_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_ssd_record_t( csmi_ssd_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_ssd_record_t( csmi_ssd_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_ssd_record_t( csmi_ssd_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_switch_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_record_t( csmi_switch_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_switch_record_t( csmi_switch_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_record_t( csmi_switch_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_switch_record_t( csmi_switch_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_switch_history_record_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_history_record_t( csmi_switch_history_record_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_switch_history_record_t( csmi_switch_history_record_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_history_record_t( csmi_switch_history_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_switch_history_record_t( csmi_switch_history_record_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_node_env_data_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_env_data_t( csmi_node_env_data_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_node_env_data_t( csmi_node_env_data_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_env_data_t( csmi_node_env_data_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_node_env_data_t( csmi_node_env_data_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_switch_env_data_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_env_data_t( csmi_switch_env_data_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_switch_env_data_t( csmi_switch_env_data_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_env_data_t( csmi_switch_env_data_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_switch_env_data_t( csmi_switch_env_data_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_fabric_topology_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_fabric_topology_t( csmi_fabric_topology_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_fabric_topology_t( csmi_fabric_topology_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_fabric_topology_t( csmi_fabric_topology_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_fabric_topology_t( csmi_fabric_topology_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_node_select_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_select_t( csmi_node_select_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_node_select_t( csmi_node_select_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_select_t( csmi_node_select_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_node_select_t( csmi_node_select_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csmi_node_details_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_details_t( csmi_node_details_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csmi_node_details_t( csmi_node_details_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_details_t( csmi_node_details_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csmi_node_details_t( csmi_node_details_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_ib_cable_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_ib_cable_query_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_ib_cable_query_history_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_ib_cable_query_history_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_ib_cable_update_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_ib_cable_update_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_query_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_query_details_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_query_details_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_query_history_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_query_history_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_update_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_node_attributes_update_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_switch_attributes_query_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_switch_attributes_query_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_switch_attributes_query_history_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_switch_attributes_query_history_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_switch_attributes_update_input_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
/** @addtogroup csm_switch_attributes_update_output_t
* @{
*/
/**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t *target, char **buf, uint32_t *buffer_len);

/** @brief Serializes the supplied structure(s) into a char buffer.
*
* @param[in]  targets    The structures to pack into the char buffer.
* @param[in]  size       The number of structures supplied.
* @param[out] buf        Contains the structure(s) as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_a_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t **targets, uint32_t size, char **buf, uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest         A collection of structs to output the contents of the buffer to.
* @param[out] num_elements The number of structs produced by the buffer.
* @param[in]  buffer       The buffer to read into the destination structs.
* @param[in]  buffer_len   The size of the buffer provided (for overflows).
*/
int deserialize_a_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t **dest[], uint32_t *num_elements, const char *buffer, uint32_t buffer_len);

/** @} */
