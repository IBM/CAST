/*================================================================================
   
    csmi/src/wm/include/csmi_wm_type_internal.h

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#ifndef _CSMI_WM_TYPE_INTERNAL_H_
#define _CSMI_WM_TYPE_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "csmi/include/csmi_type_wm.h"
/**
 * @brief Defines a context object for an allocation.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The unique identifier for the allocation. */
    int64_t primary_job_id; /**< Primary job id (for LSF this will be the LSF job ID). */
    int32_t num_processors; /**< Number of processors for use in the allocation (required). */
    int32_t num_gpus; /**< Number of gpus to be used for the node (optional). */
    int32_t projected_memory; /**< Projected memory usage on the node for the allocation. */
    int32_t secondary_job_id; /**< Secondary job id (for LSF this will be the LSF job index for job arrays). */
    int32_t isolated_cores; /**< Specifies the number of cores to isolate in the system cgroup, 0 indicates no core isolation. */
    uint32_t num_nodes; /**< Number of nodes, size of @ref compute_nodes. */
    csmi_state_t state; /**< State of allocation, refer to @ref csmi_state_t for details. */
    csmi_allocation_type_t type; /**< Type of allocation, refer to @ref csmi_allocation_type_t for details. */
    int64_t* ib_rx; /**< Count of Data Octets received on all Infiniband ports. ( multiply by 4 for bytes ).*/  
    int64_t* ib_tx; /**< Count of Data Octets transmitted on all Infiniband ports. ( multiply by 4 for bytes ).*/  
    int64_t* gpfs_read; /**< Counter for number of bytes read over network via gpfs. */  
    int64_t* gpfs_write; /**< Counter for number of bytes written over network via gpfs. */  
    int64_t* energy; /**< The energy usage of the node. */ 
    int64_t* gpu_usage; /**< The gpu usage .*/
    int64_t* cpu_usage; /**< The cpu usage in nanoseconds. */
    int64_t* memory_max; /**< The maximum memory usage in bytes/ */
    int64_t* power_cap_hit; /**< The total power cap hit value for the run of the allocation. */
    int32_t* power_cap; /**< The maximum power draw for the node - measured in watts. */
    int32_t* ps_ratio; /**< The power shift ratio of the node. */
    csm_bool shared; /**< Flag for creating a shared allocation.*/
    char save_allocation; /**< Flag that specifies whether the allocation should be saved in the database. */
    char** compute_nodes; /**< List of nodes that participated in the allocation, size stored in @ref num_nodes. */
    char* user_flags; /**< User flags for the epilog/prolog. */
    char* system_flags; /**< System flags for the epilog/prolog. */
    char* user_name; /**< User name of allocation owner. */
    int64_t* gpu_energy; /**< The gpu energy .*/
} csmi_allocation_mcast_context_t;
 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_mcast_context_t( csmi_allocation_mcast_context_t *target );


/**
 * @brief A payload for an allocation create/delete multicast.
 * 
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< Uniquely identify this allocation. */
    int64_t primary_job_id; /**< Primary job id (for LSF this will be the LSF job ID. */
    int32_t secondary_job_id; /**< Secondary job id (for LSF this will be the LSF job index for job arrays. */
    int32_t isolated_cores; /**< Specifies the number of cores to isolate in the system cgroup, 0 indicates no core isolation. */
    int32_t num_processors; /**< Number of processors for use in the allocation (required). */
    int32_t num_gpus; /**< Number of gpus to be used for the node (optional). */
    int32_t projected_memory; /**< Projected memory usage on the node for the allocation. */
    char create; /**< A flag indicating whether this is a create or delete payload. */
    csm_bool shared; /**< Flag for creating a shared allocation.*/
    char* user_name; /**< The user name of the invoking user. */
    char* user_flags; /**< User flags for the epilog/prolog. */
    char* system_flags; /**< System flags for the epilog/prolog. */
} csmi_allocation_mcast_payload_request_t;
 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_mcast_payload_request_t( csmi_allocation_mcast_payload_request_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_mcast_payload_request_t( csmi_allocation_mcast_payload_request_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_mcast_payload_request_t( csmi_allocation_mcast_payload_request_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_mcast_payload_request_t( csmi_allocation_mcast_payload_request_t *target );


/**
 * @brief A payload for an allocation create/delete multicast.
 * 
 * @todo Need to add more metrics data.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t energy; /**< The energy usage of the node. */ 
    int64_t pc_hit; /**< The power cap hit total. */
    int64_t gpu_usage; /**< The gpu usage.*/
    int64_t ib_rx; /**< Count of Data Octets received on all Infiniband ports. ( multiply by 4 for bytes ).*/  
    int64_t ib_tx; /**< Count of Data Octets transmitted on all Infiniband ports. ( multiply by 4 for bytes ).*/  
    int64_t gpfs_read; /**< Counter for number of bytes read over network via gpfs. */  
    int64_t gpfs_write; /**< Counter for number of bytes written over network via gpfs. */  
    int64_t cpu_usage; /**< The cpu usage of the allocation on the node. */
    int64_t memory_max; /**< The maximum memory used by the allocaftion. */
    int32_t power_cap; /**< The maximum power draw for the node - measured in watts. */
    int32_t ps_ratio; /**< The power shift ratio of the node. */
    char create; /**< A flag indicating whether this is a create or delete payload. */
    char* hostname; /**< The hostname of the node this Payload was sent to. */  
    int64_t gpu_energy; /**< The gpu energy (watts).*/
} csmi_allocation_mcast_payload_response_t;
 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_mcast_payload_response_t( csmi_allocation_mcast_payload_response_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_mcast_payload_response_t( csmi_allocation_mcast_payload_response_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_mcast_payload_response_t( csmi_allocation_mcast_payload_response_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_mcast_payload_response_t( csmi_allocation_mcast_payload_response_t *target );


/**
 * @brief A context struct for storing multicast contexts.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t step_id; /**< The identifier of the step, combined with @ref allocation_id to form a unique identifier. */
    int64_t allocation_id; /**< Allocation that the step is a member of, when combined with @ref step_id represents a unique step.*/
    uint32_t num_nodes; /**< Number of nodes, size of @ref compute_nodes.*/
    char begin; /**< A flag indicating whether this is a begin or end payload. */
    char* user_flags; /**< User prolog/epilog flags. If NULL the prolog and epilog are not to be run.*/
    char** compute_nodes; /**< The list of nodes associated with this step. Used to populate the *csm_step_node* table of the CSM DB. Size stored in @ref num_nodes.*/
} csmi_allocation_step_mcast_context_t;
 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_step_mcast_context_t( csmi_allocation_step_mcast_context_t *target );


/**
 * @brief Payload struct for step multicast operations.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t step_id; /**< The identifier of the step, combined with @ref allocation_id to form a unique identifier. */
    int64_t allocation_id; /**< Allocation that the step is a member of, when combined with @ref step_id represents a unique step.*/
    char begin; /**< A flag indicating whether this is a begin or end payload. */
    char* user_flags; /**< User prolog/epilog flags. If NULL the prolog and epilog are not to be run.*/
    char* hostname; /**< Hostname detailing the origination point of the payload. */
} csmi_allocation_step_mcast_payload_t;
 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_allocation_step_mcast_payload_t( csmi_allocation_step_mcast_payload_t *target );


/**
 * @brief Defines payload for jsrun Command multicasts.
 */
typedef struct {
    uint64_t _metadata; /** The number of fields in the struct.*/
    int64_t allocation_id; /**< The allocation id for the spawn. */
    uint32_t user_id; /**< The user id for the spawn. */
    char* kv_pairs; /**< The arguments for JSRUN execution. */
    char* hostname; /**< The hostname of the node. */
    char* jsm_path; /**< The fully qualified path to the JSM executable, if NULL ignored and the default path is used ( /opt/ibm/spectrum_mpi/jsm_pmix/bin/jsm ). */
} csmi_jsrun_cmd_payload_t;
 /**  @brief Serializes the supplied structure into a char buffer.
*
* @param[in]  target     The structure to pack into the char buffer.
* @param[out] buf        Contains the structure as char buffer.
* @param[out] buffer_len Contains the length of the buffer.
*/
int serialize_csmi_jsrun_cmd_payload_t( csmi_jsrun_cmd_payload_t *target, char **buf , uint32_t *buffer_len);

/** @brief Deserializes the supplied character buffer.
*
* @param[out] dest       A pointer to a struct to output the contents of the buffer to.
* @param[in]  buffer     The buffer to read into the destination struct.
* @param[in]  buffer_len The size of the buffer provided (for overflows).
*/
int deserialize_csmi_jsrun_cmd_payload_t( csmi_jsrun_cmd_payload_t **dest, const char *buffer, uint32_t buffer_len);

/** @brief Frees the supplied struct and its members.
*
*  @warning Don't invoke unless @p target has been initialized by the init function.
*
*  @param[in] target The struct to free.
*/
void free_csmi_jsrun_cmd_payload_t( csmi_jsrun_cmd_payload_t *target );

/** @brief Initializes the supplied struct to the default values.
*
*  @param[in,out] target The struct to initialize.
*/
void init_csmi_jsrun_cmd_payload_t( csmi_jsrun_cmd_payload_t *target );



#ifdef __cplusplus
}
#endif
#endif
