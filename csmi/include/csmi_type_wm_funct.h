/*================================================================================
   
    csmi/include/csmi_type_wm_funct.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_wm_funct.h
 * @brief A collection of serialization helper functions for @ref wm_apis.
 * 
 * If the user wants meaningful defaults for their CSM structs, it is 
 * recommended to use the struct's corresponding init function. 
 * Structs initialized through this function should generally be free'd 
 * using the corresponding free.
 */
#include "csmi_type_wm.h"
#ifndef _CSMI_WM_TYPE_FUNCTS_H_
#define _CSMI_WM_TYPE_FUNCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_history_t( csmi_allocation_history_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_history_t( csmi_allocation_history_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_history_t( csmi_allocation_history_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_history_t( csmi_allocation_history_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_t( csmi_allocation_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_t( csmi_allocation_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_t( csmi_allocation_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_t( csmi_allocation_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_accounting_t( csmi_allocation_accounting_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_accounting_t( csmi_allocation_accounting_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_accounting_t( csmi_allocation_accounting_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_accounting_t( csmi_allocation_accounting_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_list_t( csmi_allocation_step_list_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_list_t( csmi_allocation_step_list_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_step_list_t( csmi_allocation_step_list_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_step_list_t( csmi_allocation_step_list_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_state_history_t( csmi_allocation_state_history_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_state_history_t( csmi_allocation_state_history_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_state_history_t( csmi_allocation_state_history_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_state_history_t( csmi_allocation_state_history_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_details_t( csmi_allocation_details_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_details_t( csmi_allocation_details_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_details_t( csmi_allocation_details_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_details_t( csmi_allocation_details_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_history_t( csmi_allocation_step_history_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_history_t( csmi_allocation_step_history_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_step_history_t( csmi_allocation_step_history_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_step_history_t( csmi_allocation_step_history_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_t( csmi_allocation_step_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_t( csmi_allocation_step_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_step_t( csmi_allocation_step_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_step_t( csmi_allocation_step_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_ssd_resources_record_t( csmi_ssd_resources_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_ssd_resources_record_t( csmi_ssd_resources_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_ssd_resources_record_t( csmi_ssd_resources_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_ssd_resources_record_t( csmi_ssd_resources_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_node_resources_record_t( csmi_node_resources_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_node_resources_record_t( csmi_node_resources_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_node_resources_record_t( csmi_node_resources_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_node_resources_record_t( csmi_node_resources_record_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_cgroup_t( csmi_cgroup_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_cgroup_t( csmi_cgroup_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_cgroup_t( csmi_cgroup_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_cgroup_t( csmi_cgroup_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_resources_record_t( csmi_allocation_resources_record_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_resources_record_t( csmi_allocation_resources_record_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_resources_record_t( csmi_allocation_resources_record_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_resources_record_t( csmi_allocation_resources_record_t *target );


 /** @see serialize_csmi_allocation_t */
#define serialize_csm_allocation_create_input_t( target, buf, buffer_len )\
    serialize_csmi_allocation_t( target, buf, buffer_len )

/** @see deserialize_csmi_allocation_t */
#define deserialize_csm_allocation_create_input_t( dest, buffer, buffer_len )\
    deserialize_csmi_allocation_t( dest, buffer, buffer_len )

/** @see free_csmi_allocation_t */
#define free_csm_allocation_create_input_t( target ) free_csmi_allocation_t(target)

/** @see init_csmi_allocation_t */
#define init_csm_allocation_create_input_t( target ) init_csmi_allocation_t(target)

 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_query_details_input_t( csm_allocation_query_details_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_query_details_output_t( csm_allocation_query_details_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_update_state_input_t( csm_allocation_update_state_input_t *target );


 /** @see serialize_csmi_allocation_step_t */
#define serialize_csm_allocation_step_begin_input_t( target, buf, buffer_len )\
    serialize_csmi_allocation_step_t( target, buf, buffer_len )

/** @see deserialize_csmi_allocation_step_t */
#define deserialize_csm_allocation_step_begin_input_t( dest, buffer, buffer_len )\
    deserialize_csmi_allocation_step_t( dest, buffer, buffer_len )

/** @see free_csmi_allocation_step_t */
#define free_csm_allocation_step_begin_input_t( target ) free_csmi_allocation_step_t(target)

/** @see init_csmi_allocation_step_t */
#define init_csm_allocation_step_begin_input_t( target ) init_csmi_allocation_step_t(target)

 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_end_input_t( csm_allocation_step_end_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_end_input_t( csm_allocation_step_end_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_step_end_input_t( csm_allocation_step_end_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_step_end_input_t( csm_allocation_step_end_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_step_query_input_t( csm_allocation_step_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_step_query_output_t( csm_allocation_step_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_step_query_details_input_t( csm_allocation_step_query_details_input_t *target );


 /** @see serialize_csm_allocation_step_query_output_t */
#define serialize_csm_allocation_step_query_details_output_t( target, buf, buffer_len )\
    serialize_csm_allocation_step_query_output_t( target, buf, buffer_len )

/** @see deserialize_csm_allocation_step_query_output_t */
#define deserialize_csm_allocation_step_query_details_output_t( dest, buffer, buffer_len )\
    deserialize_csm_allocation_step_query_output_t( dest, buffer, buffer_len )

/** @see free_csm_allocation_step_query_output_t */
#define free_csm_allocation_step_query_details_output_t( target ) free_csm_allocation_step_query_output_t(target)

/** @see init_csm_allocation_step_query_output_t */
#define init_csm_allocation_step_query_details_output_t( target ) init_csm_allocation_step_query_output_t(target)

 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_step_query_active_all_input_t( csm_allocation_step_query_active_all_input_t *target );


 /** @see serialize_csm_allocation_step_query_output_t */
#define serialize_csm_allocation_step_query_active_all_output_t( target, buf, buffer_len )\
    serialize_csm_allocation_step_query_output_t( target, buf, buffer_len )

/** @see deserialize_csm_allocation_step_query_output_t */
#define deserialize_csm_allocation_step_query_active_all_output_t( dest, buffer, buffer_len )\
    deserialize_csm_allocation_step_query_output_t( dest, buffer, buffer_len )

/** @see free_csm_allocation_step_query_output_t */
#define free_csm_allocation_step_query_active_all_output_t( target ) free_csm_allocation_step_query_output_t(target)

/** @see init_csm_allocation_step_query_output_t */
#define init_csm_allocation_step_query_active_all_output_t( target ) init_csm_allocation_step_query_output_t(target)

 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_input_t( csm_node_resources_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_input_t( csm_node_resources_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_resources_query_input_t( csm_node_resources_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_resources_query_input_t( csm_node_resources_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_output_t( csm_node_resources_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_output_t( csm_node_resources_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_resources_query_output_t( csm_node_resources_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_resources_query_output_t( csm_node_resources_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_resources_query_all_input_t( csm_node_resources_query_all_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_node_resources_query_all_output_t( csm_node_resources_query_all_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_cgroup_create_input_t( csm_allocation_step_cgroup_create_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_cgroup_create_input_t( csm_allocation_step_cgroup_create_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_step_cgroup_create_input_t( csm_allocation_step_cgroup_create_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_step_cgroup_create_input_t( csm_allocation_step_cgroup_create_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_step_cgroup_delete_input_t( csm_allocation_step_cgroup_delete_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_step_cgroup_delete_input_t( csm_allocation_step_cgroup_delete_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_step_cgroup_delete_input_t( csm_allocation_step_cgroup_delete_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_step_cgroup_delete_input_t( csm_allocation_step_cgroup_delete_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_resources_query_input_t( csm_allocation_resources_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_resources_query_input_t( csm_allocation_resources_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_resources_query_input_t( csm_allocation_resources_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_resources_query_input_t( csm_allocation_resources_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_resources_query_output_t( csm_allocation_resources_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_resources_query_output_t( csm_allocation_resources_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_resources_query_output_t( csm_allocation_resources_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_resources_query_output_t( csm_allocation_resources_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_update_history_input_t( csm_allocation_update_history_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_update_history_input_t( csm_allocation_update_history_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_update_history_input_t( csm_allocation_update_history_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_update_history_input_t( csm_allocation_update_history_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_input_t( csm_allocation_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_input_t( csm_allocation_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_query_input_t( csm_allocation_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_query_input_t( csm_allocation_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_output_t( csm_allocation_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_output_t( csm_allocation_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_query_output_t( csm_allocation_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_query_output_t( csm_allocation_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_active_all_input_t( csm_allocation_query_active_all_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_active_all_input_t( csm_allocation_query_active_all_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_query_active_all_input_t( csm_allocation_query_active_all_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_query_active_all_input_t( csm_allocation_query_active_all_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_query_active_all_output_t( csm_allocation_query_active_all_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_query_active_all_output_t( csm_allocation_query_active_all_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_query_active_all_output_t( csm_allocation_query_active_all_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_query_active_all_output_t( csm_allocation_query_active_all_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_allocation_delete_input_t( csm_allocation_delete_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_allocation_delete_input_t( csm_allocation_delete_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_allocation_delete_input_t( csm_allocation_delete_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_allocation_delete_input_t( csm_allocation_delete_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_cgroup_login_input_t( csm_cgroup_login_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_cgroup_login_input_t( csm_cgroup_login_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_cgroup_login_input_t( csm_cgroup_login_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_cgroup_login_input_t( csm_cgroup_login_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_jsrun_cmd_input_t( csm_jsrun_cmd_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_jsrun_cmd_input_t( csm_jsrun_cmd_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_jsrun_cmd_input_t( csm_jsrun_cmd_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_jsrun_cmd_input_t( csm_jsrun_cmd_input_t *target );



#ifdef __cplusplus
}
#endif
#endif
