/*================================================================================
   
    csmi/include/csmi_type_inv_funct.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_inv_funct.h
 * @brief A collection of serialization helper functions for @ref inv_apis.
 * 
 * If the user wants meaningful defaults for their CSM structs, it is 
 * recommended to use the struct's corresponding init function. 
 * Structs initialized through this function should generally be free'd 
 * using the corresponding free.
 */
#include "csmi_type_inv.h"
#ifndef _CSMI_INV_TYPE_FUNCTS_H_
#define _CSMI_INV_TYPE_FUNCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_dimm_record_t( csmi_dimm_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_dimm_record_t( csmi_dimm_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_dimm_record_t( csmi_dimm_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_dimm_record_t( csmi_dimm_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_gpu_record_t( csmi_gpu_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_gpu_record_t( csmi_gpu_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_gpu_record_t( csmi_gpu_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_gpu_record_t( csmi_gpu_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_hca_record_t( csmi_hca_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_hca_record_t( csmi_hca_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_hca_record_t( csmi_hca_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_hca_record_t( csmi_hca_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_ib_cable_record_t( csmi_ib_cable_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_ib_cable_record_t( csmi_ib_cable_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_ib_cable_record_t( csmi_ib_cable_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_ib_cable_record_t( csmi_ib_cable_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_ib_cable_history_record_t( csmi_ib_cable_history_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_attributes_record_t( csmi_node_attributes_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_attributes_record_t( csmi_node_attributes_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_node_attributes_record_t( csmi_node_attributes_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_node_attributes_record_t( csmi_node_attributes_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_node_attributes_history_record_t( csmi_node_attributes_history_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_query_state_history_record_t( csmi_node_query_state_history_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_query_state_history_record_t( csmi_node_query_state_history_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_node_query_state_history_record_t( csmi_node_query_state_history_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_node_query_state_history_record_t( csmi_node_query_state_history_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_processor_record_t( csmi_processor_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_processor_record_t( csmi_processor_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_processor_record_t( csmi_processor_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_processor_record_t( csmi_processor_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_ssd_record_t( csmi_ssd_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_ssd_record_t( csmi_ssd_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_ssd_record_t( csmi_ssd_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_ssd_record_t( csmi_ssd_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_record_t( csmi_switch_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_record_t( csmi_switch_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_switch_record_t( csmi_switch_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_switch_record_t( csmi_switch_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_inventory_record_t( csmi_switch_inventory_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_inventory_record_t( csmi_switch_inventory_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_switch_inventory_record_t( csmi_switch_inventory_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_switch_inventory_record_t( csmi_switch_inventory_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_ports_record_t( csmi_switch_ports_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_ports_record_t( csmi_switch_ports_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_switch_ports_record_t( csmi_switch_ports_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_switch_ports_record_t( csmi_switch_ports_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_details_t( csmi_switch_details_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_details_t( csmi_switch_details_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_switch_details_t( csmi_switch_details_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_switch_details_t( csmi_switch_details_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_history_record_t( csmi_switch_history_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_history_record_t( csmi_switch_history_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_switch_history_record_t( csmi_switch_history_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_switch_history_record_t( csmi_switch_history_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_env_data_t( csmi_node_env_data_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_env_data_t( csmi_node_env_data_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_node_env_data_t( csmi_node_env_data_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_node_env_data_t( csmi_node_env_data_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_switch_env_data_t( csmi_switch_env_data_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_switch_env_data_t( csmi_switch_env_data_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_switch_env_data_t( csmi_switch_env_data_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_switch_env_data_t( csmi_switch_env_data_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_fabric_topology_t( csmi_fabric_topology_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_fabric_topology_t( csmi_fabric_topology_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_fabric_topology_t( csmi_fabric_topology_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_fabric_topology_t( csmi_fabric_topology_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_details_t( csmi_node_details_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_details_t( csmi_node_details_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_node_details_t( csmi_node_details_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_node_details_t( csmi_node_details_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_cluster_query_state_record_t( csmi_cluster_query_state_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_cluster_query_state_record_t( csmi_cluster_query_state_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_cluster_query_state_record_t( csmi_cluster_query_state_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_cluster_query_state_record_t( csmi_cluster_query_state_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_find_job_record_t( csmi_node_find_job_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_find_job_record_t( csmi_node_find_job_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_node_find_job_record_t( csmi_node_find_job_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_node_find_job_record_t( csmi_node_find_job_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_inventory_collection_input_t( csm_ib_cable_inventory_collection_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_inventory_collection_input_t( csm_ib_cable_inventory_collection_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_inventory_collection_input_t( csm_ib_cable_inventory_collection_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_inventory_collection_input_t( csm_ib_cable_inventory_collection_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_inventory_collection_output_t( csm_ib_cable_inventory_collection_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_inventory_collection_output_t( csm_ib_cable_inventory_collection_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_inventory_collection_output_t( csm_ib_cable_inventory_collection_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_inventory_collection_output_t( csm_ib_cable_inventory_collection_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_query_input_t( csm_ib_cable_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_query_output_t( csm_ib_cable_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_query_history_input_t( csm_ib_cable_query_history_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_query_history_output_t( csm_ib_cable_query_history_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_update_input_t( csm_ib_cable_update_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_ib_cable_update_output_t( csm_ib_cable_update_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_query_input_t( csm_node_attributes_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_query_output_t( csm_node_attributes_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_query_details_input_t( csm_node_attributes_query_details_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_query_details_output_t( csm_node_attributes_query_details_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_query_history_input_t( csm_node_attributes_query_history_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_query_history_output_t( csm_node_attributes_query_history_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_query_state_history_input_t( csm_node_query_state_history_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_query_state_history_input_t( csm_node_query_state_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_query_state_history_input_t( csm_node_query_state_history_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_query_state_history_input_t( csm_node_query_state_history_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_query_state_history_output_t( csm_node_query_state_history_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_query_state_history_output_t( csm_node_query_state_history_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_query_state_history_output_t( csm_node_query_state_history_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_query_state_history_output_t( csm_node_query_state_history_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_update_input_t( csm_node_attributes_update_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_attributes_update_output_t( csm_node_attributes_update_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_delete_input_t( csm_node_delete_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_delete_input_t( csm_node_delete_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_delete_input_t( csm_node_delete_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_delete_input_t( csm_node_delete_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_delete_output_t( csm_node_delete_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_delete_output_t( csm_node_delete_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_delete_output_t( csm_node_delete_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_delete_output_t( csm_node_delete_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_find_job_input_t( csm_node_find_job_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_find_job_input_t( csm_node_find_job_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_find_job_input_t( csm_node_find_job_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_find_job_input_t( csm_node_find_job_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_find_job_output_t( csm_node_find_job_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_find_job_output_t( csm_node_find_job_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_find_job_output_t( csm_node_find_job_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_find_job_output_t( csm_node_find_job_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_query_input_t( csm_switch_attributes_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_query_output_t( csm_switch_attributes_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_details_input_t( csm_switch_attributes_query_details_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_details_input_t( csm_switch_attributes_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_query_details_input_t( csm_switch_attributes_query_details_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_query_details_input_t( csm_switch_attributes_query_details_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_details_output_t( csm_switch_attributes_query_details_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_details_output_t( csm_switch_attributes_query_details_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_query_details_output_t( csm_switch_attributes_query_details_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_query_details_output_t( csm_switch_attributes_query_details_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_query_history_input_t( csm_switch_attributes_query_history_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_query_history_output_t( csm_switch_attributes_query_history_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_update_input_t( csm_switch_attributes_update_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_attributes_update_output_t( csm_switch_attributes_update_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_inventory_collection_input_t( csm_switch_inventory_collection_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_inventory_collection_input_t( csm_switch_inventory_collection_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_inventory_collection_input_t( csm_switch_inventory_collection_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_inventory_collection_input_t( csm_switch_inventory_collection_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_inventory_collection_output_t( csm_switch_inventory_collection_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_inventory_collection_output_t( csm_switch_inventory_collection_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_inventory_collection_output_t( csm_switch_inventory_collection_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_inventory_collection_output_t( csm_switch_inventory_collection_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_children_inventory_collection_input_t( csm_switch_children_inventory_collection_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_children_inventory_collection_input_t( csm_switch_children_inventory_collection_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_children_inventory_collection_input_t( csm_switch_children_inventory_collection_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_children_inventory_collection_input_t( csm_switch_children_inventory_collection_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_switch_children_inventory_collection_output_t( csm_switch_children_inventory_collection_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_switch_children_inventory_collection_output_t( csm_switch_children_inventory_collection_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_switch_children_inventory_collection_output_t( csm_switch_children_inventory_collection_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_switch_children_inventory_collection_output_t( csm_switch_children_inventory_collection_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_cluster_query_state_input_t( csm_cluster_query_state_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_cluster_query_state_input_t( csm_cluster_query_state_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_cluster_query_state_input_t( csm_cluster_query_state_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_cluster_query_state_input_t( csm_cluster_query_state_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_cluster_query_state_output_t( csm_cluster_query_state_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_cluster_query_state_output_t( csm_cluster_query_state_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_cluster_query_state_output_t( csm_cluster_query_state_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_cluster_query_state_output_t( csm_cluster_query_state_output_t *target );



#ifdef __cplusplus
}
#endif
#endif
