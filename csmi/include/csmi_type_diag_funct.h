/*================================================================================
   
    csmi/include/csmi_type_diag_funct.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
/** @file csmi_type_diag_funct.h
 * @brief A collection of serialization helper functions for @ref diag_apis.
 * 
 * If the user wants meaningful defaults for their CSM structs, it is 
 * recommended to use the struct's corresponding init function. 
 * Structs initialized through this function should generally be free'd 
 * using the corresponding free.
 */
#include "csmi_type_diag.h"
#ifndef _CSMI_DIAG_TYPE_FUNCTS_H_
#define _CSMI_DIAG_TYPE_FUNCTS_H_

#ifdef __cplusplus
extern "C" {
#endif

 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_diag_run_t( csmi_diag_run_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_diag_run_t( csmi_diag_run_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_diag_run_t( csmi_diag_run_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_diag_run_t( csmi_diag_run_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_diag_run_query_details_result_t( csmi_diag_run_query_details_result_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_diag_run_query_details_result_t( csmi_diag_run_query_details_result_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_diag_run_query_details_result_t( csmi_diag_run_query_details_result_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_diag_run_query_details_result_t( csmi_diag_run_query_details_result_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_diag_run_end_input_t( csm_diag_run_end_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_diag_run_end_input_t( csm_diag_run_end_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_diag_run_end_input_t( csm_diag_run_end_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_diag_run_end_input_t( csm_diag_run_end_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_diag_result_create_input_t( csm_diag_result_create_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_diag_result_create_input_t( csm_diag_result_create_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_diag_result_create_input_t( csm_diag_result_create_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_diag_result_create_input_t( csm_diag_result_create_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_diag_run_begin_input_t( csm_diag_run_begin_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_diag_run_begin_input_t( csm_diag_run_begin_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_diag_run_begin_input_t( csm_diag_run_begin_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_diag_run_begin_input_t( csm_diag_run_begin_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_diag_run_query_input_t( csm_diag_run_query_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_diag_run_query_input_t( csm_diag_run_query_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_diag_run_query_input_t( csm_diag_run_query_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_diag_run_query_input_t( csm_diag_run_query_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_diag_run_query_output_t( csm_diag_run_query_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_diag_run_query_output_t( csm_diag_run_query_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_diag_run_query_output_t( csm_diag_run_query_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_diag_run_query_output_t( csm_diag_run_query_output_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_diag_run_query_details_input_t( csm_diag_run_query_details_input_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_diag_run_query_details_input_t( csm_diag_run_query_details_input_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_diag_run_query_details_input_t( csm_diag_run_query_details_input_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_diag_run_query_details_input_t( csm_diag_run_query_details_input_t *target );


 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csm_diag_run_query_details_output_t( csm_diag_run_query_details_output_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csm_diag_run_query_details_output_t( csm_diag_run_query_details_output_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csm_diag_run_query_details_output_t( csm_diag_run_query_details_output_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csm_diag_run_query_details_output_t( csm_diag_run_query_details_output_t *target );



#ifdef __cplusplus
}
#endif
#endif
